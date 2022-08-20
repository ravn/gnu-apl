#! /usr/local/bin/apl --script
⍝ ********************************************************************
⍝   $Id: $
⍝ $desc: Library of useful apl functions $
⍝ ********************************************************************

⍝ Util
⍝ Copyright (C) 2016  Bill Daly

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

∇msg←utl∆helpFns fn;src;t
  ⍝ Display help about a function
  src←⎕cr fn
  t←(+/∧\src=' ')⌽src
  msg←(1,∧\'⍝'=1↓t[;1])⌿src
∇

∇t←utl∆numberp v
  ⍝ Is arg a number?
  →(~t←1≥≡v)/0
  t←''⍴0=1↑0⍴v
∇

∇b←utl∆integerp n
⍝ Function to test that n is an integer.
→(~b←utl∆numberp n)/0
b←n=⌊n
∇

∇b←utl∆charp arg
  ⍝ Function returns true if arg is a single character
  →(~b←0=≡arg)/0
  b←''⍴' '=1↑0⍴arg
∇

∇ t←utl∆stringp s
  ⍝ Is arg a string?
  ⍝ test for nested array
  →(~t←1=≡s)/0			
  t←''⍴' '=1↑0⍴s←,s
  
∇

∇b←utl∆numberis tst
  ⍝ Test whether a number can be obtained by executing a string
  ⍎(0=⍴tst←,tst)/'b←0 ◊ →0'
  →(~b←~∧/' '=tst)/0 		⍝ Test for a blank argument
  ⍝ Rotate spaces to right side
  tst←(+/∧\tst=' ')⌽tst
  ⍝ Test for spaces imbedded in numbers
  →(~b←(+/∧\b)=+/b←tst≠' ')/0
  b←(∧/tst∊' 1234567890-¯.')∧∨/0 1=+/tst='.'
  b←b∧∧/~(1↓tst)∊'-¯'
∇

∇New←utl∆stripArraySpaces old;b
  ⍝ Strips off leading and trailing spaces. Function operates on both
  ⍝ vectors and arrays of rank 2. See also utl∆clean.
  New←(+/∧\old=' ')⌽old
  b←⌊/+/∧\⌽New=' '
  →(V,M,E)[3⌊⍴⍴old]
  ⍝ Vector
V:
  New←New[⍳-b-⍴New]
  →0
  ⍝ Matrix
M:
  New←New[;⍳-b-1↓⍴New]
  →0
  ⍝ Error -- rank of old is too high
E:
  ⎕es 'Rank of array is too high'
∇

∇cl←utl∆clean txt;b;ix
  ⍝ Converts all whites space to spaces and then removes duplicate
  ⍝ spaces. See also utl∆stringArraySpaces.
  txt←,txt
  ix←(txt∊⎕tc,⎕av[10])/⍳⍴txt
  txt[ix]←' '
  →(0=⍴cl←(~(1⌽b)∧b←txt=' ')/txt)/0
  cl←(cl[1]=' ')↓(-cl[⍴cl]=' ')↓cl
∇

∇o←k utl∆sub d
  ⍝ Calculates subtotals for each break point in larg
  o←+\[1]d
  ⍝ Test for rank of data
  ⎕es (~(⍴⍴d) ∊ 1 2)/'RANK ERROR'
  →(V,A)[⍴⍴d]
  ⍝ Vectors
V:o←o[k]-¯1↓0,o[k]
  →0
  ⍝ Arrays (of rank 2)
A: o←o[k;]-0,[1] o[¯1↓k;]
  →0
∇

∇string←delim utl∆join vector
  ⍝ Returns a character string with delim delimiting the items
  ⍝ in vector.
  string←1↓∊,delim,(⌽1,⍴vector)⍴vector
∇

∇v←delim utl∆split string;b
 ⍝ Split a string at delim.  No recursive algorithm
 b←(string∊delim)/⍳⍴string←,string
 b←(0,b),[1.1]-/(b,1+⍴string),[1.1] 1,b+1
 v←,(⊂string) utl∆sph ¨ ⊂[2]b
∇

∇v←delim utl∆split_with_quotes txt;ix;b
  ⍝ Function to split a delimited array with quoted strings to an array.
  b←((~b∨≠\b←txt∊'"')∧delim=txt)/⍳⍴txt
  b←(0,b),[1.1]-/(b,1+⍴txt),[1.1] 1,b+1
  v←(⊂txt) utl∆sph ¨ ⊂[2]b
  v←utl∆swq_helper ¨ v
∇

∇item←string utl∆sph ix
 ⍝ Helper function for utl∆split returns an item from a character
 ⍝ vector where ix index of the delimeter in the stringstring and the
 ⍝ length of the item.
 ix←⊃ix
 string←⊃string
 ix←ix[1]+⍳ix[2]
 item←string[ix]
∇

∇item←utl∆swq_helper quoted
  ⍝ Helper function for utl∆split_with_quotes to remove quotes.
  →(0=⍴quoted)/unquoted
  →(~(quoted[1]=quoted[⍴quoted])∧quoted[1] ∊ '''"')/unquoted
  item←¯1↓1↓quoted
  →0
unquoted: item←quoted
  →0
∇

∇ix← list utl∆listSearch item;rl;ri;l
  ⍝ Search a character list for an item.
  →(1=≡list)/arr
  list←⊃list
arr:
  ⎕es(2≠⍴rl←⍴list)/'RANK ERROR'
  ri←⍴item←,item
  l←rl[2]⌈ri
  →(0=⍴ix←(((rl[1],l)↑list)∧.=l↑,item)⌿⍳rl[1])/naught
  ix←''⍴ix
  →0
naught:
  ix←1+''⍴⍴list
∇

∇ix←txt utl∆search word;⎕io;old∆io;ixx;bv
  ⍝ Search for larg in rarg.
  old∆io←⎕io
  ⎕io←0
  ixx←⍳⍴txt←,txt
  bv←(txt=1↑word←,word)∧ixx≤(⍴txt)-⍴word
  ix←bv/ixx
  ix←old∆io+(txt[ix∘.+⍳⍴word]∧.=word)/ix
∇

∇new←txt utl∆replace args;ix
  ⍝ Search for and replace an item in rarg.  Larg is a two element
  ⍝ vector where Larg[1] is the text to search for, Larg[2] is the
  ⍝ replacement text.
  ix← txt utl∆search ⊃args[1]
  new←((¯1+ix)↑txt),(,⊃args[2]),(¯1+(ix←''⍴ix)+⍴,⊃args[1])↓txt
∇


∇t←n utl∆execTime c;ts;lb;i
  ⍝ Returns the number of milliseconds a command took. larg is the
  ⍝ number of times to execute command.  If larg is missing we execute
  ⍝ once.
  →(2=⎕nc 'n')/many
  ts←⎕ts
  ⍎c
  →ed
many:
  lb←(n⍴st),ed
  i←0
  ts←⎕ts
st:
  ⍎c
  →lb[i←i+1]
ed:
  t←⎕ts
  t←(60 1000⊥t[6 7])-60 1000⊥ts[6 7]
  →0
∇

∇today←utl∆today
  ⍝ Today's date as a string
  today←'06/06/0000'⍕⎕ts[2 3 1]

∇

∇txt←utl∆lower m;ix
  ⍝ Convert text to all lower case.
  m←⎕ucs m←,m
  ix←((m≥65)∧m≤90)/⍳⍴m
  m[ix]←m[ix]+32
  txt←⎕ucs m
∇

∇txt←utl∆upper m;ix
  ⍝ Convert text to all upper case.
  m←⎕ucs m←,m
  ix←((m≥97)∧m≤122)/⍳⍴m
  m[ix]←m[ix]-32
  txt←⎕ucs m
∇

∇b←str1 utl∆stringEquals str2;l
  ⍝ Compare two strings.
  l←(⍴str1←,str1)⌈⍴str2←,str2
  b←∧/(l↑str1)=l↑str2
∇

∇txt←utl∆crWithLineNo name;l
  ⍝ Add line numbers to a character representation of a function.
  l←¯1+1↑⍴txt←⎕cr name
  txt←('     ∇',[1]'[000] '⍕⍪⍳l),txt
∇

∇clean←utl∆strip_quotes txt;bv
  ⍝ Strip quotes from the start and end of character string.
  clean←txt
  →(~1↑bv←≠\clean∊'''"')/0
  clean←(bv∧¯1⌽bv)/clean
∇

∇new←om utl∆round old
  ⍝ Round numbers based on the Order of Magnitude.  Left
  ⍝ arg is thus a power of ten where positive numbers round to the
  ⍝ left of the decimal point and negative to the right.
  ⍎(2≠⎕nc'om')/'om←0'
  om←10*om
  new←om×⌊.5+old÷om
∇

∇ar←utl∆concatColumns na
  ⍝ Function returns a 2 dimensional text array from a nested array of text.
  →(1=¯1↑⍴na)/lastCol
  ar←(⊃na[;1]),' ', utl∆concatColumns 0 1↓na
  →0
lastCol:
  ar←⊃,na
  →0
∇

∇n←utl∆convertStringToNumber s;bv;a                                       
  ⍝ Converts a vector of characters to a number.  Function
  ⍝ returns the original string when it fails in this attempt. For
  ⍝ strings multiple numbers see utl∆import∆numbers.
  →(~∧/s∊'0123456789.,-¯ ()')/fail                                      
  →(1<+/s='.')/fail                                                  
  →(0=⍴(s≠' ')/s)/fail
  a←((~∧\bv)∧⌽~∧\⌽bv←s=' ')/s                                       
  →(0≠+/a=' ')/fail
  →(∧/'-'=(' '≠a)/a)/zero
  a[(a∊'(-')/⍳⍴a←,' ',a]←'¯'                                         
  n←⍎(~a∊'),')/a                                                     
  →0
zero:				⍝ Excel sometimes uses dash for 0
  n←0
  →0
fail:                                                                
  n←s                                                                
∇

∇n←utl∆import∆numbers s;bv
 ⍝ Function to turn a column of figures (ie characters) into
 ⍝ numbers. For a single number see util∆convertStringToNumber
 ⍎(2=≡s)/'s←⊃s'
 bv←~∧/s=' '
 s[(s∊'(-')/⍳⍴s←,' ',s]←'¯'
 n←bv\⍎(~s∊'),')/s
∇


∇utl∆es msg
  ⍝ Simulate an error. Similar to ⎕es with better control of the error
  ⍝ message. Thanks JAS
  →(0=⍴msg)/0
  msg ⎕es 0 1
∇

∇b←list utl∆member item
  ⍝ Tests whether a character vector is in list, a character array,
  ⍝ or a nested list of strings.
  b←∊(1+1↑⍴list)>list utl∆listSearch item
∇

∇parsed←utl∆fileName∆parse fname;suffix
  ⍝ Function breaks a fname down into three strings:
  ⍝  1) Path to directory
  ⍝  2) root name
  ⍝  3) suffix, that is whatever trails the final '.'.
  parsed←'/' utl∆split fname
  suffix←'.' utl∆split (⍴parsed)⊃parsed
  →(one,many)[2⌊⍴suffix]
one:
  parsed←(⊂'/' utl∆join ¯1↓ parsed),⊃suffix,⊂''
  →0
many:
  parsed←(⊂'/' utl∆join ¯1↓ parsed),(⊂'.'utl∆join ¯1↓suffix),¯1↑suffix
  →0
∇

∇dir←utl∆fileName∆dirname parsed
  ⍝ Function returns the directory portion of a parsed file name
  dir←1⊃parsed
∇

∇base←utl∆fileName∆basename parsed
  ⍝ Function returns the base of the file name from a parsed file name
  base←2⊃parsed
∇

∇suffix←utl∆fileName∆suffixname parsed
  ⍝ Function returns the suffix of a parsed file name.
  suffix ← 3⊃parsed
∇

∇backup←utl∆fileName∆backupname parsed
  ⍝ Function returns a name to which a file can be backed up.
  backup←(1⊃parsed),'/',(2⊃parsed),'.bak'
∇

∇ar←utl∆concatColumns na
  ⍝ Function returns a 2 dimensional text array from a nested array of text
  →(1=¯1↑⍴na)/lastCol
  ar←(⊃na[;1]),' ', utl∆concatColumns 0 1↓na
  →0
lastCol:
  ar←⊃,na
  →0
∇
  
∇sub←breakFld utl∆breakon amts;ix
  ⍝ function to calculate subtotals for changes in breakFld
  ix←(~breakFld utl∆stringEquals ¨ 1⌽breakFld)/⍳⍴breakFld←,breakFld
  sub←ix utl∆sub amts
∇

∇ b←str utl∆stringMember list
  ⍝ Function returns true if  str is in list
  ⍎(0=⍴list)/'b←0 ◊ →0'
  b←(str utl∆stringEquals 1⊃list)∨str utl∆stringMember 1↓list
∇


∇numbered←utl∆numberedArray array;shape;level
  ⍝ Function prepends a line number on to an array
  shape←⍴array
  utl∆es ((0=level)∨(2≠⍴⍴array)∨2<level←≡array)/'Malformed array for these purposes'
  numbered←('[003] '⍕(shape[1],1)⍴⍳shape[1]),array
∇

∇ix←utl∆gradeup data;t1;base
  ⍝ Function to alphabetically grade up data
  ⍎(∧/(2=≡data),t1←utl∆stringp ¨ data)/'data←⊃data'
  utl∆es (~∧/t1)/'DATA NOT CHARACTERS'
  base←2*⍳20
  base←base[+/1,∧\base<⌈/⎕ucs ∊,data]
  ix←⍋(⊂(¯1↑⍴data)⍴base)⊥¨⊂[2]⎕ucs¨data
∇
  

∇new ← utl∆sort old;base
  ⍝ Function sorts a character array or nested character vectors
  new←old[utl∆gradeup old]
∇

∇ix←data utl∆quad bottom_right;top_left;rows;cols;all
  ⍝ Function returns the row and column indices defined by the top
  ⍝ right and bottom left indices in the right argument
  →(2≠⎕nc 'data')/syntax
  →(1≠⍴⍴bottom_right)/syntax
⍝  →(2=⍴bottom_right)/nested
  →(4≠⍴bottom_right)/syntax
  top_left←bottom_right[1 2]
  bottom_right←bottom_right[3 4]
  →step2
nested:
  top_left←1⊃bottom_right
  bottom_right←2⊃bottom_right
  →step2
step2:
  →(~utl∆numberp top_left,bottom_right)/syntax 
  rows←((top_left[1]≤all)∧bottom_right[1]≥all)/all←⍳1↑⍴data
  cols←((top_left[2]≤all)∧bottom_right[2]≥all)/all←⍳1↓⍴data
  ix←(⊂rows),⊂cols
  →0
syntax:
  utl∆es 'SYNTAX IS: data_array utl∆quad upper_left, bottom_right'
∇

∇rs←delim utl∆bifurcate txt;bv
  ⍝ Function breaks a string into two parts using delim.
  bv←∧\txt≠⍬⍴delim
  ⍎((+/bv)=⍴txt)/'rs←txt ◊ →0'
  rs←(⊂bv/txt),⊂(+/1,bv)↓txt
∇

∇t←max utl∆deal shape;count;ix;⎕rl
  ⍝ Function returns an array of random numbers (with replacement)
  ⍝ less than or equal to max.
  ⎕rl←⎕FIO[60] 8
  count←×/shape
  ix←1
  t←⍬
st:t←t,1?max
  →(count>ix←ix+1)/st
  t←shape⍴t
∇

∇ws←utl∆copiedWorkspaces;fns_split
  ⍝ Function returns a list of function name prefixes
  fns_split ← (⊂'_∆⍙') utl∆split¨ utl∆clean ¨ ⊂[2]⎕nl 3                  
  ws←(~ws utl∆stringEquals ¨ 1⌽ws)/ws←1⊃¨fns_split
∇

∇cleaned←utl∆rm_trailing_lf txt
  ⍝ Function will remove a trailing line feed from a test string.
  cleaned←(¯1×txt[⍴txt]=⎕tc[3])↓txt
∇

∇utl∆display msg
  ⍝ Function to display a message
  ⍞←⎕tc[3] utl∆join msg
∇

∇rs←utl∆rm_blanks array;shape;ix
  ⍝ Function removes '!blank:' inserted into edited arrays by
  ⍝ gnu-apl-mode library.
  shape←⍴array
  rs←,array
  rs[(∊{'!:blank' utl∆stringEquals ⍵}¨rs)/⍳1↑⍴rs]←' '
  rs←shape⍴rs
∇

∇Z←utl⍙metadata
  Z←0 2⍴⍬
  Z←Z⍪'Author'          'Bill Daly'
  Z←Z⍪'BugEmail'        'bugs@dalywebandedit.com'
  Z←Z⍪'Documentation'   'doc/apl-library.info'
  Z←Z⍪'Download'        'https://sourceforge.net/p/apl-library/code/ci/master/tree/utl.apl'
  Z←Z⍪'License'         'GPL v3.0'
  Z←Z⍪'Portability'     'L3'
  Z←Z⍪'Provides'        ''
  Z←Z⍪'Requires'        ''
  Z←Z⍪'Version'                           '0 3 1'
  Z←Z⍪'Last update'               '2022-02-07'
∇

