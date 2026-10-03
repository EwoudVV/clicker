#!/usr/bin/env python3
"""Refresh the enclosure references from the two KiCad boards."""
from pathlib import Path
import hashlib,json,re,shutil,subprocess,os
import cadquery as cq

HERE=Path(__file__).resolve().parent
ROOT=HERE.parent
REF=HERE/'reference'
REF.mkdir(exist_ok=True)
TMP=ROOT/'work/enclosure-review/step'
TMP.mkdir(parents=True,exist_ok=True)
CLI=os.environ.get('KICAD_CLI') or shutil.which('kicad-cli') or '/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli'


def read(path):
    tokens=iter(re.findall(r'"(?:[^"\\]|\\.)*"|[()]|[^\s()]+',path.read_text()))
    def atom(t):
        if t.startswith('"'):return json.loads(t)
        try:return float(t)
        except ValueError:return t
    def expr():
        result=[]
        for t in tokens:
            if t==')':return result
            result.append(expr() if t=='(' else atom(t))
        raise ValueError('Unclosed KiCad expression')
    assert next(tokens)=='('
    return expr()


def children(node,key):return [n for n in node if isinstance(n,list) and n and n[0]==key]
def child(node,key):return children(node,key)[0]

def bbox(shape):
    b=shape.BoundingBox()
    return [round(v,6) for v in [b.xmin,b.xmax,b.ymin,b.ymax,b.zmin,b.zmax]]


data={}
for name in ['remote','receiver']:
    source=ROOT/name/f'{name}.kicad_pcb'
    board=read(source)
    pts=[]
    for kind in ['gr_line','gr_arc','gr_rect']:
        for graphic in children(board,kind):
            if child(graphic,'layer')[1]!='Edge.Cuts':continue
            for key in ['start','mid','end']:
                pts.extend(n[1:3] for n in children(graphic,key))
    if not pts:raise ValueError('No board outline found')
    cx=(min(p[0] for p in pts)+max(p[0] for p in pts))/2
    cy=(min(p[1] for p in pts)+max(p[1] for p in pts))/2
    footprints=[]
    for f in children(board,'footprint'):
        props={n[1]:n[2] for n in children(f,'property')}
        pos=child(f,'at')
        footprints.append({'ref':props['Reference'],'value':props['Value'],
            'center':[round(pos[1]-cx,4),round(cy-pos[2],4)],
            'angle':pos[3] if len(pos)>3 else 0,
            'side':'back' if child(f,'layer')[1]=='B.Cu' else 'front'})
    d={'thickness':child(child(board,'general'),'thickness')[1],'footprints':footprints,
       'origin':[cx,cy],'board_sha256':hashlib.sha256(source.read_bytes()).hexdigest()}
    refs=['U1','HDR1','SW_BOOT','SW_RST']+(['J_BAT','SW_PWR','D_CHG','D_STAT'] if name=='remote' else ['D1'])
    for suffix,opts in [('board',['--board-only']),('assembly',[])]+[(r,['--no-board-body','--component-filter',r]) for r in refs]:
        target=(REF/f'{name}.step' if suffix=='assembly' else
                REF/f'{name}-board.step' if suffix=='board' else TMP/f'{name}-{suffix}.step')
        cmd=[CLI,'pcb','export','step','--force','--subst-models','--user-origin',f'{cx}x{cy}mm','-o',str(target),*opts,str(source)]
        r=subprocess.run(cmd,capture_output=True,text=True)
        if r.returncode:raise RuntimeError(r.stdout+r.stderr)
        shape=cq.importers.importStep(str(target)).val()
        if suffix=='board':d['board_bounds']=bbox(shape)
        elif suffix=='assembly':d['assembly_bounds']=bbox(shape)
        else:next(f for f in footprints if f['ref']==suffix)['bounds']=bbox(shape)
        if suffix=='SW_PWR':shutil.copyfile(target,REF/'power_switch.step')
    data[name]=d
    print(name,d['origin'],d['board_bounds'])
(HERE/'measurements.json').write_text(json.dumps(data,indent=2)+'\n')
