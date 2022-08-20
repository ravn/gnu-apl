#! /usr/local/bin/apl --script
 ⍝ ********************************************************************
 ⍝   $Id: $
 ⍝ $desc: Workspace with a simple prompting function $
 ⍝ ********************************************************************
⍝ Copyright (C)2016 2017 Bill Daly

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

)copy_once 5 DALY/utl

∇resp←editcheck prompt pr;test;sink;ans;msg
  ⍝ Subroutine to prompt for user input.
  ⍎(3≠⎕nc 'editcheck')/'editcheck←''prompt∆doesnothing'''
  st:
   sink←⍞←pr
   ans←⍞
   ans←(~∧\ans=' ')/ans
  →(utl∆numberp test←prompt∆keys ans)/kw
  →(0=⍴msg←⍎editcheck,' ans')/ok
  ⍞←msg,⎕tc[3]
  →st
ok:
   resp←ans
   →0
 kw:
   resp←test
   →0
∇

∇msg←prompt∆doesnothing ans
  ⍝ Edit check that does nothing.  A real edit check would return an
  ⍝ error message.
  msg←⍬
∇

∇o←prompt∆keys i;k
   ⍝ Function to return a keyword number if i is a keyword.
   k←((4 4 ⍴ 'quittop doneback')∧.=4↑o←,i)/⍳4
   →((4<⍴i)∨0=⍴k)/0
   o←k-⎕io
   →0
∇

∇Z←prompt⍙metadata
  Z←0 2⍴⍬
  Z←Z⍪'Author'          'Bill Daly'
  Z←Z⍪'BugEmail'        'bugs@dalywebandedit.com'
  Z←Z⍪'Documentation'   'doc/apl-library.info'
  Z←Z⍪'Download'        'https://sourceforge.net/p/apl-library/code/ci/master/tree/html.apl'
  Z←Z⍪'License'         'GPL v3.0'
  Z←Z⍪'Portability'     'L1'
  Z←Z⍪'Provides'        'Top/Quit/Done design pattern for user input.'
  Z←Z⍪'Requires'        ''
  Z←Z⍪'Version'                  '0 1 3'
  Z←Z⍪'Last update'         '2019-07-01'
∇

