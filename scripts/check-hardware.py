#!/usr/bin/env python3
import collections
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORTS = ROOT / 'verification/2026-10-03'

def read_sexpr(path):
    tokens = iter(re.findall(r'"(?:[^"\\]|\\.)*"|[()]|[^\s()]+', path.read_text()))
    def atom(t):
        if t.startswith('"'):
            return json.loads(t)
        try:
            return float(t)
        except ValueError:
            return t
    def expression():
        result = []
        for t in tokens:
            if t == ')':
                return result
            result.append(expression() if t == '(' else atom(t))
        raise ValueError('Unclosed expression')
    assert next(tokens) == '('
    return expression()

def children(node, name):
    return [n for n in node if isinstance(n, list) and n and n[0] == name]

def child(node, name):
    return children(node, name)[0]

def routes(board):
    return {child(n, 'uuid')[1]: n for n in board
            if isinstance(n, list) and n and n[0] in ('segment', 'via', 'arc')}

def positions(board):
    result = {}
    for fp in children(board, 'footprint'):
        ref = next(n[2] for n in children(fp, 'property') if n[1] == 'Reference')
        result[ref] = (child(fp, 'at'), child(fp, 'layer'))
    return result

def pad_nets(board):
    result = {}
    for fp in children(board, 'footprint'):
        ref = next(n[2] for n in children(fp, 'property') if n[1] == 'Reference')
        for pad in children(fp, 'pad'):
            nets = children(pad, 'net')
            net = nets[0][-1] if nets else ''
            result[(ref, str(pad[1]))] = net
    return result

summary = {}
for name in ('remote', 'receiver'):
    before = read_sexpr(ROOT / f'work/hardware-before/{name}.kicad_pcb')
    after = read_sexpr(ROOT / f'{name}/{name}.kicad_pcb')
    assert positions(before) == positions(after), f'{name}: components moved'
    assert routes(before) == routes(after), f'{name}: routes changed'
    expected = {}
    for net in ET.parse(REPORTS / f'{name}-netlist.xml').findall('./nets/net'):
        for node in net.findall('node'):
            expected[(node.get('ref'), node.get('pin'))] = net.get('name')
    mismatches = []
    for key, actual in pad_nets(after).items():
        wanted = expected.get(key, '')
        if wanted.startswith('unconnected-') and not actual:
            continue
        if wanted.lstrip('/') != actual.lstrip('/'):
            mismatches.append((key, actual, wanted))
    assert not mismatches, (name, mismatches)
    drc = json.loads((REPORTS / f'{name}-drc-final.json').read_text())
    assert not drc['unconnected_items'], f'{name}: unrouted items'
    assert not any(v['severity'] == 'error' for v in drc['violations'])
    summary[name] = {
        'component_positions_preserved': len(positions(after)),
        'routing_objects_preserved': len(routes(after)),
        'pad_net_mismatches': mismatches,
        'drc_errors': 0,
        'unconnected': 0,
        'warnings': len(drc['violations']),
        'parity_warnings': len(drc['schematic_parity']),
    }
print(json.dumps(summary, indent=2))
(REPORTS / 'connectivity-summary.json').write_text(json.dumps(summary, indent=2) + '\n')
