#!/usr/bin/env python3
"""Pack the print-oriented meshes into a printer-independent 3MF plate."""
from pathlib import Path
import zipfile,xml.etree.ElementTree as ET,json
import trimesh

HERE=Path(__file__).resolve().parent
NS='http://schemas.microsoft.com/3dmanufacturing/core/2015/02'
ET.register_namespace('',NS)
def tag(n):return '{'+NS+'}'+n
model=ET.Element(tag('model'),{'unit':'millimeter','{http://www.w3.org/XML/1998/namespace}lang':'en-US'})
ET.SubElement(model,tag('metadata'),{'name':'Title'}).text='Clicker cases'
resources=ET.SubElement(model,tag('resources'))
build=ET.SubElement(model,tag('build'))
layout=[('remote_base',35,48),('remote_lid',102,48),
        ('receiver_base',159,29),('receiver_lid',199,29),
        ('keycap_previous',157,70),('keycap_next',184,70),
        ('stem_fit_coupon',172,101)]
ids={};placed=[]
for name,x,y in layout:
    mesh=trimesh.load(HERE/'stl'/f'{name}.stl',force='mesh')
    if name not in ids:
        i=len(ids)+1;ids[name]=i
        obj=ET.SubElement(resources,tag('object'),{'id':str(i),'type':'model','name':name})
        me=ET.SubElement(obj,tag('mesh'));vs=ET.SubElement(me,tag('vertices'));ts=ET.SubElement(me,tag('triangles'))
        for v in mesh.vertices:ET.SubElement(vs,tag('vertex'),dict(zip(['x','y','z'],[f'{c:.6f}' for c in v])))
        for f in mesh.faces:ET.SubElement(ts,tag('triangle'),dict(zip(['v1','v2','v3'],[str(c) for c in f])))
    ET.SubElement(build,tag('item'),{'objectid':str(ids[name]),'transform':f'1 0 0 0 1 0 0 0 1 {x} {y} 0'})
    b=mesh.bounds.copy();b[:,0]+=x;b[:,1]+=y
    assert b[0,0]>0 and b[0,1]>0 and b[1,0]<220 and b[1,1]<160
    assert abs(b[0,2])<1e-5
    for previous,other in placed:
        overlap=min(b[1,0],other[1,0])-max(b[0,0],other[0,0])>0 and min(b[1,1],other[1,1])-max(b[0,1],other[0,1])>0
        assert not overlap,(name,previous)
    placed.append((name,b))
content_types='''<?xml version="1.0" encoding="UTF-8"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/></Types>'''
rels='''<?xml version="1.0" encoding="UTF-8"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Target="/3D/3dmodel.model" Id="rel0" Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/></Relationships>'''
path=HERE/'clicker-print-plate.3mf'
with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED) as z:
    z.writestr('[Content_Types].xml',content_types)
    z.writestr('_rels/.rels',rels)
    z.writestr('3D/3dmodel.model',ET.tostring(model,encoding='utf-8',xml_declaration=True))
with zipfile.ZipFile(path) as z:assert z.testzip() is None
check=trimesh.load(path)
assert len(check.graph.nodes_geometry)==len(layout)
print('3MF checked:',len(layout),'objects,',len(ids),'unique meshes, mm units, flat on bed, no overlaps')
report=json.loads((HERE/'fit-check.json').read_text());report['print_plate']={'objects':len(layout),'minimum_bed_mm':[220,160],'units':'millimeter','overlaps':0}
(HERE/'fit-check.json').write_text(json.dumps(report,indent=2)+'\n')
