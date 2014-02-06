#Remove VC90 manifest dependencies from manifest files
#The audio libraries manage to create manifest dependencies on those, and it is evil
#Kristjan Valur, 2011
from xml.dom.minidom import parseString
from xml.dom import Node
import sys


def Empty(node):
    if not node.hasChildNodes():
        return True
    node.normalize()
    children = node.childNodes
    if len(children)>1:
        return False
    
    child = children[0]
    if child.nodeType == Node.TEXT_NODE:
        v = child.data
        v = v.strip()
        if not v:
            return True
    return False
        

with file(sys.argv[1], "r") as f:
    assembly = parseString(f.read())
assert assembly.documentElement.tagName == "assembly"

#remove all vc9 assemblies
for e in assembly.getElementsByTagName("assemblyIdentity"):
    if "VC90" in e.getAttribute("name"):
        if e.parentNode:
            e.parentNode.removeChild(e)


#clean up empty nodes
for e in assembly.getElementsByTagName("dependentAssembly"):
    if Empty(e):
        if e.parentNode:
            e.parentNode.removeChild(e)

for e in assembly.getElementsByTagName("dependency"):
    if Empty(e):
        if e.parentNode:
            e.parentNode.removeChild(e)

fn = sys.argv[2] if len(sys.argv) > 2 else sys.argv[1]
with file(fn, "w") as f:
    assembly.writexml(f, encoding="utf8")


    