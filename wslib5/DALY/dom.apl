#! /usr/local/bin/apl --script
⍝ ********************************************************************
⍝ dom.apl Partial implementation of the Document Object Model
⍝ Copyright (C) 2019 Bill Daly

⍝ This program is free software: you can redistribute it and/or modify
⍝ it under the terms of the GNU General Public License as published by
⍝ the Free Software Foundation, either version 3 of the License, or
⍝ (at your option) any later version.

⍝ This program is distributed in the hope that it will be useful,
⍝ but WITHOUT ANY WARRANTY; without even the implied warranty of
⍝ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
⍝ GNU General Public License for more details.

⍝ You should have received a copy of the GNU General Public License
⍝ along with this program.  If not, see <http://www.gnu.org/licenses/>.

⍝ ********************************************************************

)copy_once 5 DALY/utl
)copy_once 3 DALY/lex
)copy_once 3 DALY/stack

⍝ ********************************************************************
⍝
⍝		       Create objects
⍝
⍝ ********************************************************************

∇attr←parent dom∆createAttribute name;value
  ⍝ Function creates an element attribute
    →(2=⍴name)/double
  utl∆es (~utl∆stringp name)/name,' is not a valid attribute name.'
  →single
single:
  value←dom∆TRUE
  →make
double:
  utl∆es (~∧/(2=⍴name),utl∆stringp ¨ name)/'''',name,''' is not a valid attribute'
  value←2⊃name
  name←1⊃name
make:
  attr←dom∆createNode name
  attr[2]←⊂(2⊃attr) lex∆assign 'nodeType' dom∆ATTRIBUTE_NODE
  attr[2]←⊂(2⊃attr) lex∆assign 'nodeValue' value
∇

∇node← dom∆createComment txt
   node← dom∆createNode 'Comment'
   node[2]←⊂(⊃node[2]) lex∆assign 'nodeType' dom∆COMMENT_NODE
   node[2]←⊂(⊃node[2]) lex∆assign 'nodeValue' txt
∇

∇docNode← dom∆createDocument rootName;rootNode;typeNode;uri;dn
   ⍝ Function to create a document. If root name is a nested vector
   ⍝ rootName[1] is the document qualifiedName and rootName[2] is its
   ⍝ URI. Left argument docType is optional and if ommitted will be deduced.
  docNode← dom∆createNode 'Document'
  docNode←docNode dom∆node∆setNodeType dom∆DOCUMENT_NODE
  →0
∇

∇documentTypeNode← dom∆createDocumentType rootName;dt
   →((2=≡rootName)∧2=⍴rootName)/create
   rootName←' ' utl∆split rootName
 create:
   documentTypeNode←dom∆createNode ⊃rootName[1]
   dt←(⊃documentTypeNode[2]) lex∆assign 'nodeType' dom∆DOCUMENT_TYPE_NODE
   dt←dt lex∆assign (⊂'nodeValue'), ⊂rootName[1]
   →(1=⍴rootName)/end
   dt←dt lex∆assign 2↑1↓rootName,dom∆TRUE
end:
  documentTypeNode[2]←⊂dt
   →0
∇

∇elementNode← dom∆createElement name;en
   elementNode← dom∆createNode name
   en←(⊃elementNode[2]) lex∆assign 'nodeType' dom∆ELEMENT_NODE
   elementNode[2]←⊂en lex∆assign 'attributes' dom∆createNamedNodeMap
∇

∇node←dom∆createTextNode txt;attrs
  node← dom∆createNode '#text#'
  attrs←(⊃node[2]) lex∆assign 'nodeType' dom∆TEXT_NODE
  node[2]←⊂attrs lex∆assign 'nodeValue' txt
∇

∇pi←dom∆createProcessingInstruction txt;b;target;data;pn
  ⍝ Function creates processor specific instructions node
  txt←utl∆clean txt
  target←(b←∧\txt≠' ')/txt
  data←1↓(~b)/txt
  pi← dom∆createNode target
  pn←(⊃pi[2]) lex∆assign 'nodeType' dom∆PROCESSING_INSTRUCTION_NODE
  pn←pn lex∆assign 'target' target
  pi[2]←⊂pn lex∆assign 'data' data
∇

∇node← dom∆createNode name
   ⍝ Fn creates a DOM node
   node←lex∆init
   node←node lex∆assign 'nodeName' name
   node←node lex∆assign 'nodeValue' ' '
   node←node lex∆assign 'nodeType' 0
   node←(⊂0⍴0),⊂node
∇

⍝ ********************************************************************
⍝
⍝			     Node Methods
⍝
⍝ ********************************************************************

∇new←node dom∆node∆appendChild child;children
  ⍝ Function to add a child to the end of our vector
  new←node
  children←(⊃node[1]),⊂child
  new[1]←⊂children
   
∇

∇new←node dom∆node∆prependChild child; children
  ⍝ Function to add a child tot he begining of our vector
  new←node
  children←(⊂child),1⊃node
  new[1]←⊂children
∇

∇n←dom∆node∆nodeName node
   n←(⊃node[2])lex∆lookup 'nodeName'
∇

∇new←node dom∆node∆setNodeName name
  new←node[1],⊂(⊃node[2]) lex∆assign 'nodeName' name
∇

∇t←dom∆node∆nodeType node
   t←(⊃node[2]) lex∆lookup 'nodeType'
∇

∇new←node dom∆node∆setNodeType type
  new←node[1],⊂(⊃node[2]) lex∆assign 'nodeType' type
∇

∇v←dom∆node∆nodeValue node
   v←(⊃node[2])lex∆lookup 'nodeValue'
∇

∇new←node dom∆node∆setNodeValue value
  new←node[1],⊂(⊃node[2]) lex∆assign 'nodeValue' value
∇

∇o←dom∆node∆ownerDocument node
   o←(⊃node[2]) lex∆lookup 'ownerDocument'
∇

∇new←node dom∆node∆setOwenerDocument doc
  new←node[1],⊂(⊃node[2]) lex∆assign 'ownerDocument' doc
∇

∇ch←dom∆node∆children node
  ch←⊃node[1]
∇

∇b←dom∆node∆hasChildren node
  b←0≠1↑⍴1⊃node
∇

∇b←dom∆node∆hasAttributes node
  b←~lex∆isempty dom∆node∆attributes node
∇

∇new←node dom∆node∆setChildren children
  ⍝ Out with the old in with the new.  This function replaces what
  ⍝ ever children there are with an new list.
  new←(⊂children),node[2]
∇

∇attrs←dom∆node∆attributes node
  ⍝ Function returns a named node map of attributes
  attrs←(⊃node[2]) lex∆lookup 'attributes'
∇

∇new←node dom∆node∆setAttribute item;attr;cix;attr_vector
  →(dom∆attr∆predicate item)/setAttr
  item←dom∆createAttribute item
setAttr:
  attr←dom∆node∆attributes node
  attr←attr dom∆namedNodeMap∆setNamedItem item
  node[2]←⊂(2⊃node) lex∆assign 'attributes' attr
  new←node
∇

∇xml←dom∆node∆toxml node;next;nextix
  ⍝ Function returns an xml text vector for a node
  →(elm,attr,txt,cdata,ref,ent,pi,com,doc,type,frag,note)[dom∆node∆nodeType node]
elm:				⍝ Element
  xml←'<',(dom∆node∆nodeName node)
  ⍎(dom∆node∆hasAttributes node)/'xml←xml,dom∆node∆toxml ¨ dom∆namedNodeMap∆list dom∆node∆attributes node'
  ⍎(~dom∆node∆hasChildren node)/'xml←xml,''/>''◊→0'
  xml←xml,'>'
  xml←xml,∊dom∆node∆toxml ¨ dom∆node∆children node
  xml←xml,'</',(dom∆node∆nodeName node),'>'
  →0
attr:				⍝ Attribute
  →(dom∆TRUE utl∆stringEquals dom∆node∆nodeValue node)/single_attr
double_attr:
  xml←' ',(dom∆node∆nodeName node),'="',(dom∆node∆nodeValue node),'"'
  →0
single_attr:
  xml←' ',dom∆node∆nodeName node
  →0
txt:				⍝ Text
  xml←dom∆node∆nodeValue node
  →0
cdata:				⍝ CDATA
  xml←dom∆node∆nodeValue node
  →0
ref:				⍝ Entity Reference
  xml←'NOT IMPLEMENTED'
  →0
ent:				⍝ Entity
  xml←'NOT IMPLEMENTED'
  →0
pi:				⍝ Processing Instruction
  xml←'<?',(dom∆pi∆target node),' ',(dom∆pi∆data node),'?>'
  →0
com:				⍝ Comment Node
  xml←'<!--',(dom∆node∆nodeValue node),'-->'
  →0
doc:				⍝ Document node
  xml← ∊dom∆node∆toxml ¨ dom∆node∆children node
  →0
type:				⍝ Document Type node
  xml←'<!DOCTYPE ',(dom∆node∆nodeName node)
  →(~(2⊃node) lex∆haskey 'SYSTEM')/typePublic
  xml←xml,' SYSTEM ',(2⊃node)lex∆lookup 'SYSTEM'
typePublic:
  →(~(2⊃node) lex∆haskey 'PUBLIC')/typeEnd
  xml←xml,' PUBLIC ',(2⊃node)lex∆lookup 'PUBLIC'
typeEnd:
  xml←xml,'>'
  →0
frag:				⍝ Document fragment
  xml←'NOT IMPLEMENTED'
  →0
note:				⍝ Notation
  xml←'NOT IMPLEMENTED'
  →0
∇

∇child←node dom∆node∆getChild n
  ⍝ Returns the nth child of node
  child←⊃(dom∆node∆children node)[n]
∇

∇children← dom∆node∆getChildren node
  children←1⊃node
∇

∇b←dom∆node∆predicate node
  ⍝ Function tests to see if node is a dom node.  This is not conical,
  ⍝ but I can't proceed without it.
  →(~b←1=⍴⍴node)/0
  →(~b←2=⍴node)/0
  →(~b←lex∆is 2⊃node)/0
  →(~b←(2⊃node) lex∆haskey 'nodeName')
  →(~b←(2⊃node) lex∆haskey 'nodeValue')
  →(~b←(2⊃node) lex∆haskey 'nodeType')
  b←1
∇

⍝ ********************************************************************
⍝
⍝			   Element Methods
⍝
⍝ ********************************************************************

∇new←dom∆element∆childless elm
  ⍝ Method marks an element as childless ie <tag/>
  new←elm
  new[2]←⊂(2⊃new) lex∆assign 'childless' 1
∇

∇b←dom∆element∆isChildless elm
  ⍝ Method returns the childless attribute
  b←(2⊃elm) lex∆lookup 'childless'
∇


⍝ ********************************************************************
⍝
⍝			   Document methods
⍝
⍝ ********************************************************************

∇node←dom∆document∆rootElement doc;children;i;lb
  ⍝ Function returns the root element of a document
  children←dom∆node∆children doc
  i←1
  lb←((⍴children)⍴st),ed
st:
  node←⊃children[i]
  →(dom∆ELEMENT_NODE=dom∆node∆nodeType node)/0
  →lb[i←i+1]
ed:
  node←dom∆createElement 'MALFORMED DOCUMENT'
  →0
∇

∇doc←doc dom∆document∆setRootElement rootElm;children;i;lb
  ⍝ Function replaces the root element of a document. Function should
  ⍝ be called after updating or changing nodes of a document.
  i←1
  lb←((⍴children←⊃doc[1])⍴st),ed
st:
  →(~dom∆ELEMENT_NODE=dom∆node∆nodeType ⊃children[i])/next
  children[i]←⊂rootElm
next:
  →lb[i←i+1]
ed:
  doc[1]←⊂children
∇

∇type←dom∆document∆getDocumentType doc;children
  ⍝ Function returns the document type node.
  children←dom∆node∆getChildren doc
  type←(dom∆DOCUMENT_TYPE_NODE = dom∆node∆nodeType¨children)/children
∇

∇doc←doc dom∆document∆setDocumentType typeNode;children;i;lb
  ⍝ Function replaces the root element of a document. Function should
  ⍝ be called after updating or changing nodes of a document.
  i←1
  lb←((⍴children←⊃doc[1])⍴st),ed
st:
  →(~dom∆DOCUMENT_TYPE_NODE=dom∆node∆nodeType ⊃children[i])/next
  children[i]←⊂typeNode
next:
  →lb[i←i+1]
ed:
  doc[1]←⊂children
∇

∇nl←name dom∆document∆getElementsByTagName node;children;child;lb
  ⍝ Function returns a NodeList of elements with the give name
  →(name utl∆stringEquals dom∆node∆nodeName node)/ahit
  nl←⊂dom∆createNodeList
  →ch
ahit:
  nl←(⊂node),dom∆createNodeList
  →ch
ch:
  →(0=⍴children←dom∆node∆getChildren node)/0
  child←1
  lb←((⍴children)⍴st),end
st:
  nl←nl,name dom∆document∆getElementsByTagName child⊃children
  nl←(0≠∊⍴¨nl)/nl
  →lb[child←child+1]
end:
∇


⍝ ********************************************************************
⍝
⍝			  Attribute Methods
⍝
⍝ ********************************************************************

∇ b←dom∆attr∆predicate node
  →(~b←dom∆node∆predicate node)/0
  b←dom∆ATTRIBUTE_NODE = dom∆node∆nodeType node
∇

⍝ ********************************************************************
⍝
⍝			   Nodelist Methods
⍝
⍝ ********************************************************************
∇nl←dom∆createNodeList
  nl←0⍴0
∇

∇length←dom∆nodeList∆length list
  length←''⍴⍴list
∇

∇node←list dom∆nodeList∆item item
  ⍝ Returns the itemth
  ⍎(item>⍴list)/'item←0⍴0 ◊ →0'
  node←item⊃list
∇

∇new←list dom∆nodeList∆appendNode node
  ⍝ Function appends a node to a node list
  →(0≠⍴list)/append
  new←1⍴⊂node
  →0
append:
  new←list,⊂node
∇

∇ix←nodeList dom∆nodeList∆lookup name
  ⍝ Function returns the index of the given node name in a node list.
  ix←(dom∆node∆nodeName ¨ nodeList) utl∆listSearch name
∇

∇b←dom∆nodeList∆predicate list
  ⍝ Function test whether list is a nodeList
  →(~b←1=⍴⍴list)/0		⍝ Not a list
  b←∧/dom∆node∆predicate ¨ list
∇

⍝ ********************************************************************
⍝
⍝			    NamedNodeMap
⍝
⍝ ********************************************************************
∇ map←dom∆createNamedNodeMap
  map←lex∆init
∇

∇ node←map dom∆namedNodeMap∆getNamedItem name
  node←map lex∆lookup name
∇

∇ map←old dom∆namedNodeMap∆setNamedItem attr;name
  ⍝  Function to add or change an attribute
  name←dom∆node∆nodeName attr
  map←old lex∆assign name attr
∇

∇ map←old dom∆namedNodeMap∆removeNamedItem attr;name
  ⍝ Function to remove an attribute
  name←dom∆node∆nodeName attr
  map←old lex∆drop name
∇

∇item←map dom∆namedNodeMap∆item index
  ⍝ Function returns the indexth item in the map
  item←2⊃map
∇

∇list←dom∆namedNodeMap∆list map
  ⍝ Function returns the elements of the map as a list
  list←lex∆values map
∇

⍝ ********************************************************************
⍝
⍝		  Processing instructions are dom∆pi
⍝
⍝ ********************************************************************
∇target←dom∆pi∆target node
  target←(⊃node[2]) lex∆lookup 'target'
∇

∇data←dom∆pi∆data node
  data←(⊃node[2]) lex∆lookup 'data'
∇

⍝ ********************************************************************
⍝
⍝			    Parse Methods
⍝
⍝ ********************************************************************

∇doc←dom∆parse txt;nl
  ⍝ Function to parse an xml text buffer
  nl←dom∆parse∆nodeFromSource ¨ '>' dom∆split txt
  doc←dom∆parse∆foldNodeList nl
∇

∇node←dom∆parse∆nodeFromSource src;b
  ⍝ dom∆parse subroutine returns a node list from the source once it
  ⍝ has been split on '>'
  →(0=⍴src)/txtNode
  src←utl∆clean src
    b←(∧/'<!-'=3↑src),(∧/'<!D'=3↑src),(∧/'</'=2↑src),(∧/'<?'=2↑src),('<'=1↑src),1
  →b/(commentNode,doctypeNode,closeElm,proc,openElm,txtNode)
commentNode:
  node←dom∆parse∆commentNode src
  →end
doctypeNode:
  node←dom∆parse∆doctypeNode src
  →end
proc:
  node←dom∆parse∆processingInstruction src
  →end
openElm:
  node←dom∆parse∆openElm src
  →end
closeElm:
  node←dom∆parse∆closeElm src
  →end
txtNode:
  node←dom∆createTextNode src
end:
∇

∇node←dom∆parse∆commentNode source
  ⍝ Function creates a comment node from the source.
  node←dom∆createComment 3↓source
∇

∇node←dom∆parse∆doctypeNode source
  ⍝ Function creates a doctype node from source
  node←dom∆createDocumentType 10↓source ⍝ 8 == ⍴ '<!DOCTYPE '
∇

∇node←dom∆parse∆processingInstruction source
  ⍝ Function creates a processing instruction
  node←dom∆createProcessingInstruction 2↓¯1↓source
∇

∇elm←dom∆parse∆openElm source;b;name;closed;attr;ix
  ⍝ Function returns an element node from source.
  closed←'/'=¯1↑source                                                     
  name←1↓(b←∧\source≠' ')/source←(-closed)↓source                          
  elm←dom∆createElement name                                               
  ⍎closed/'elm←dom∆element∆childless elm'                                  
  →(∧/b)/ed                                                                
  attr←,'=' utl∆split_with_quotes ¨ ' ' utl∆split_with_quotes 1↓(~b)/source
  ix←1                                                                     
st:                                                                      
  →(ix>⍴attr)/ed                                                           
  elm←elm dom∆node∆setAttribute ix⊃attr                                    
  ix←ix+1                                                                  
  →st                                                                      
ed:                                                                      
∇

∇node←dom∆parse∆closeElm source
  ⍝ Function returns a place holder from the end of an element.
  node←dom∆createElement 1↓source
  node←node dom∆node∆setNodeValue 'Closing element'
  node←node dom∆node∆setNodeType dom∆special_ELEMENT_END
∇

∇doc←dom∆parse∆foldNodeList nl;curNode;nlix;nodeStack;b;docNode
  ⍝  Function traverses node list nl finding children and assigning
  ⍝  them to their parent.
  doc←dom∆createDocument '#Document'
  nodeStack← stack∆init
  nlix←1
  ⍝ First loop to find the root element
st1:
  curNode←nlix⊃nl
  ⍎(dom∆DOCUMENT_TYPE_NODE=dom∆node∆nodeType curNode)/'doc←doc dom∆node∆appendChild curNode ◊ →nxt1'
  ⍎('xml' utl∆stringEquals dom∆node∆nodeName curNode)/'doc←doc dom∆node∆appendChild curNode ◊ →nxt1'
  →(dom∆ELEMENT_NODE ≠ dom∆node∆nodeType curNode)/nxt1
  nodeStack←nodeStack stack∆push doc
  nodeStack←nodeStack stack∆push curNode
nxt1:
  nlix←nlix+1
  →(2=stack∆length nodeStack)/st2
  →st1
  ⍝ Second loop to find the children of the root element
st2:
  curNode←nlix⊃nl
  →(dom∆ELEMENT_NODE dom∆special_ELEMENT_END dom∆TEXT_NODE = dom∆node∆nodeType curNode)/elm,elmEnd,txt
  ⍝ What do I do now?
  →nxt2
elm:
  nodeStack←nodeStack dom∆parse∆foldElement curNode
  →nxt2
elmEnd:
  nodeStack←nodeStack dom∆parse∆endElement curNode
  →nxt2
txt:
  nodeStack←nodeStack dom∆parse∆foldText curNode
  →nxt2
nxt2:
  nlix←nlix+1
  →(nlix>⍴nl)/ed
  →st2
ed:
  doc←nodeStack stack∆nth stack∆length nodeStack
st3:
  →(1=stack∆length nodeStack)/0
  curNode←stack∆peek nodeStack
  nodeStack←stack∆pop nodeStack
  doc←doc dom∆node∆appendChild curNode
  →st3  
∇

∇ newStack←nodeStack dom∆parse∆foldText txt;words;tag;parent
  ⍝ Function folds a text node into the nodeStack
  words←dom∆node∆nodeValue txt
  words←'<' utl∆split utl∆clean words
  →('/'=1↑tag←,⊃¯1↑words)/endFound
  newStack←nodeStack stack∆push txt
  →0
endFound:
  tag←1↓tag 			⍝ For '/'
  txt←txt dom∆node∆setNodeValue utl∆clean 1⊃words
  parent←stack∆peek nodeStack
  ⍎(~tag utl∆stringEquals dom∆node∆nodeName parent)/'newStack←nodeStack stack∆push txt ◊ →0'
  nodeStack←stack∆pop nodeStack
  parent←parent dom∆node∆appendChild txt
  newStack←nodeStack stack∆push parent
  →0
∇

∇ newStack←nodeStack dom∆parse∆foldElement elm;parent
  ⍝ Function folds an element node into the nodelist
  newStack←nodeStack stack∆push curNode
∇

∇ nodeStack←nodeStack dom∆parse∆endElement curNode;ix;iy;child;name
  ⍝ Routine to append children on stack to the current element node.
  name←('/'=name[1])↓name←dom∆node∆nodeName curNode
  →(dom∆element∆isChildless curNode)/0
  ix←1
st1:
  →(name utl∆stringEquals dom∆node∆nodeName nodeStack stack∆nth ix)/nxt
  ix←ix+1
  →st1
nxt:
  →(ix=1)/ed
  child←stack∆peek nodeStack
  nodeStack←stack∆pop nodeStack
  ix←ix - 1
  nodeStack[ix]←⊂(ix⊃nodeStack) dom∆node∆prependChild child
  →nxt
ed:
∇

⍝ ********************************************************************
⍝
⍝				 Meta
⍝
⍝ ********************************************************************

∇Z←dom⍙metadata
  Z←0 2⍴⍬
  Z←Z⍪'Author'          'Bill Daly'
  Z←Z⍪'BugEmail'        'bugs@DalyWebAndEdit.com'
  Z←Z⍪'Documentation'   'doc/apl-library.info'
  Z←Z⍪'Download'        'https://sourceforge.net/projects/apl-library/files/latest/download?source=directory'
  Z←Z⍪'License'         'GPL'
  Z←Z⍪'Portability'     'L3'
  Z←Z⍪'Provides'        'dom'
  Z←Z⍪'Requires'        'util lex'
  Z←Z⍪'Version'                  '0 2 7'
  Z←Z⍪'Last update'         '2022-02-07'
∇

dom∆ELEMENT_NODE←1

dom∆ATTRIBUTE_NODE←2

dom∆TEXT_NODE←3

dom∆CDATA_SECTION_NODE←4

dom∆ENTITY_REFERENCE_NODE←5

dom∆ENTITY_NODE←6

dom∆PROCESSING_INSTRUCTION_NODE←7

dom∆COMMENT_NODE←8

dom∆DOCUMENT_NODE←9

dom∆DOCUMENT_TYPE_NODE←10

dom∆DOCUMENT_FRAGMENT_NODE←11

dom∆NOTATION_NODE←12

dom∆special_ELEMENT_END←50

dom∆type∆DESC←12⍴0
dom∆type∆DESC[1]←⊂'Element'
dom∆type∆DESC[2]←⊂'Attribute'
dom∆type∆DESC[3]←⊂'Text'
dom∆type∆DESC[4]←⊂'CDATA section'
dom∆type∆DESC[5]←⊂'Entity reference'
dom∆type∆DESC[6]←⊂'Entity'
dom∆type∆DESC[7]←⊂'Processing instruction'
dom∆type∆DESC[8]←⊂'Comment'
dom∆type∆DESC[9]←⊂'Document'
dom∆type∆DESC[10]←⊂'Document type'
dom∆type∆DESC[11]←⊂'Document fragment'
dom∆type∆DESC[12]←⊂'Notation'

dom∆TRUE←'True'

dom∆FALSE←'False'

dom∆defaultImplementation←'THIS WORKSPACE'

dom∆error∆NOT_FOUND←'NOT FOUND'

∇v←delim dom∆split string;b
  ⍝ Split a string at delim.  No recursive algorithm for dom parsing.
  b←(delim=string)/⍳⍴string←,string
  b←b,[1.1]-b-¯1+1↓b,1+⍴string
  v←(⊂string) dom∆sph ¨ ⊂[2]b
∇

∇item←string dom∆sph ix
  ⍝ Helper function for dom∆split returns an item from a character
  ⍝ vector where ix index of the delimeter in the stringstring and the
  ⍝ length of the item.
  ix←ix[1]+⍳ix[2]
  item←string[ix]
∇

