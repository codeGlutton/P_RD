"""Author editable Effekseer particles using the retained CC0 textures.

No baked animation or generated image is used. The official editor compiles
this legacy XML project into the runtime effect in the next build step.
"""
from pathlib import Path
import xml.etree.ElementTree as E
import math

source = Path(__file__).resolve().parents[1] / 'SourceArt/ThirdParty/EffekseerHoly/Effects'
project = E.Element('EffekseerProject')
root = E.SubElement(project, 'Root')
E.SubElement(root, 'Name').text = 'Root'
children = E.SubElement(root, 'Children')

def setv(n, path, value):
    for part in path.split('/'):
        c = n.find(part)
        n = c if c is not None else E.SubElement(n, part)
    n.text = str(value)

def rand(n, path, value):
    for key in ('Center', 'Max', 'Min'): setv(n, path+'/'+key, value)

def node(name, texture, life=64, delay=0, alpha=255, energy=2.0, color=(255,214,100)):
    n = E.SubElement(children, 'Node')
    setv(n, 'Name', name)
    setv(n, 'CommonValues/MaxGeneration/Value', 1)
    rand(n, 'CommonValues/Life', life)
    rand(n, 'CommonValues/GenerationTimeOffset', delay)
    setv(n, 'RendererCommonValues/ColorTexture', 'Textures/'+texture+'.png')
    setv(n, 'RendererCommonValues/AlphaBlend', 1)
    setv(n, 'RendererCommonValues/EmissiveScaling', energy)
    setv(n, 'RendererCommonValues/FadeInType', 1)
    setv(n, 'RendererCommonValues/FadeIn/Frame', 5)
    setv(n, 'RendererCommonValues/FadeOutType', 1)
    setv(n, 'RendererCommonValues/FadeOut/Frame', 24)
    for k,v in zip(('R','G','B','A'), (*color, alpha)):
        setv(n, 'DrawingValues/ColorAll/Fixed/'+k, v)
    E.SubElement(n, 'Children')
    return n

def scale(n, start, end):
    setv(n, 'ScalingValues/Type', 2)
    for stage, vals in [('Start',start),('End',end)]:
        for axis,v in zip('XYZ',vals): rand(n, 'ScalingValues/Easing/'+stage+'/'+axis,v)
    setv(n, 'ScalingValues/Easing/StartSpeed', 20)
    setv(n, 'ScalingValues/Easing/EndSpeed', -20)

def position(n, x,y):
    setv(n,'LocationValues/Fixed/Location/X',x)
    setv(n,'LocationValues/Fixed/Location/Y',y)

# A luminous interior and soft, non-rotating light around the chest mouth.
n=node('chest_inner_light','tx_glow01_128',life=68,energy=3,color=(255,235,161))
position(n,0,1.4);scale(n,(1.2,1.2,1),(5.4,4.2,1))
n=node('warm_surround','tx_glow02_128',life=68,alpha=100,energy=1.4)
position(n,0,2);scale(n,(3,3,1),(12,10,1))
# Broad radiance with distinct shafts rising from a common source.
for i,angle in enumerate([-58,-38,-21,0,19,36,55]):
    a=math.radians(angle); length=[6,8,7,9,7,8,6][i]
    n=node('light_shaft_'+str(i),'tx_glow03_128',life=62,delay=i%3,alpha=135,energy=1.8)
    position(n,-math.sin(a)*length*.22,1.2+math.cos(a)*length*.22)
    setv(n,'RotationValues/Fixed/Rotation/Z',angle)
    scale(n,(.15,1,1),(.75,length,1))
# A faint expanding envelope, never an orbiting ribbon.
n=node('outer_radiance','tx_shockring01_256',life=58,delay=4,alpha=90,energy=1.5)
position(n,0,2);scale(n,(4,4,1),(12,12,1))
n=node('ground_radiance','tx_ring01_256',life=52,delay=5,alpha=65,energy=1.2)
position(n,0,.8);scale(n,(3,.7,1),(14,3.2,1))
# Independent small sparkles lift away from the opening.
for i in range(18):
    angle=i*2.399963; x=math.sin(angle)*(1.7+(i%4)*.7)
    endy=2+(i%6)*.9
    n=node('treasure_spark_'+str(i),'tx_star01_256' if i%3==0 else 'tx_glow03_128',life=34+i%4*4,delay=4+i*1.2,energy=2.6,color=(255,231,146))
    setv(n,'LocationValues/Type',2)
    for stage,xx,yy in [('Start',x*.25,1),('End',x,endy)]:
        rand(n,'LocationValues/Easing/'+stage+'/X',xx)
        rand(n,'LocationValues/Easing/'+stage+'/Y',yy)
    s=.34 if i%3==0 else .13
    scale(n,(s,s,1),(s*.4,s*.4,1))
for k,v in [('ToolVersion','1.70e'),('Version',3),('StartFrame',0),('EndFrame',90),('IsLoop','False')]: setv(project,k,v)
E.indent(project)
path=source/'chest_radiance.efkproj'
E.ElementTree(project).write(path,encoding='utf-8',xml_declaration=True)
print(path)
