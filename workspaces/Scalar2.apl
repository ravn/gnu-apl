#! /usr/local/bin/apl --script

  ⍝ tunable parameters for this benchmark program
  ⍝
  ⍙PROFILE←4000 2000 50  ⍝ fractions of Integer, Real, and Complex numbers

)COPY 5 FILE_IO.apl

 ⍝ expressions to be benchmarked. The integer STAT is the statistics
 ⍝ as in Performance.def, i.e. 0 for F12_PLUS, 1 for F12_MINUS, and so on.
 ⍝
∇Z←MON_EXPR
  Z←⍬
  ⍝     A          OP    B          N CN              STAT
  ⍝-------------------------------------------------------
⍝ Z←Z,⊂ ""         "+"   "⍙MMix_IRC"  1 "F12_PLUS"       0
⍝ Z←Z,⊂ ""         "-"   "⍙MMix_IRC"  1 "F12_MINUS"      1
⍝ Z←Z,⊂ ""         "×"   "⍙MMix_IRC"  1 "F12_TIMES"      2
  Z←Z,⊂ ""         "÷"   "⍙MMix1_IRC" 1 "F12_DIVIDE"     3
⍝ Z←Z,⊂ ""         "∼"   "⍙MBool"     1 "F12_WITHOUT"    4
⍝ Z←Z,⊂ ""         "⌈"   "⍙MMix_IR"   1 "F12_RND_UP"     5
⍝ Z←Z,⊂ ""         "⌊"   "⍙MMix_IR"   1 "F12_RND_DN"     6
⍝ Z←Z,⊂ ""         "!"   "⍙MInt2"     1 "F12_BINOM"      7
⍝ Z←Z,⊂ ""         "⋆"   "⍙MMix_IRC"  1 "F12_POWER"      8
⍝ Z←Z,⊂ ""         "⍟"   "⍙MMix1_IRC" 1 "F12_LOGA"       9
⍝ Z←Z,⊂ ""         "○"   "⍙MMix_IRC"  1 "F12_CIRCLE"    10
⍝ Z←Z,⊂ ""         "∣"   "⍙MMix_IR"   1 "F12_STILE"     11
⍝ Z←Z,⊂ ""         "?"   "⍙MInt2"     1 "F12_ROLL"      12
∇

∇Z←DYA_EXPR
  Z←⍬
  ⍝     A            OP    B           N CN              STAT
  ⍝-------------------------------------------------------
⍝ Z←Z,⊂ "⍙MMix_IRC"  "+"   "⍙Mix_IRC"  2 "F12_PLUS"      13
⍝ Z←Z,⊂ "⍙MMix_IRC"  "-"   "⍙Mix_IRC"  2 "F12_MINUS"     14
⍝ Z←Z,⊂ "⍙MMix_IRC"  "×"   "⍙Mix1_IRC" 2 "F12_TIMES"     15
⍝ Z←Z,⊂ "⍙MMix1_IRC" "÷"   "⍙Mix1_IRC" 2 "F12_DIVIDE"    16
⍝ Z←Z,⊂ "⍙MBool"     "∧"   "⍙Bool1"    2 "F2_AND"        17
⍝ Z←Z,⊂ "⍙MInt"      "⊤∧"  "⍙Int"      2 "F2_AND_B"      18
⍝ Z←Z,⊂ "⍙MBool"     "∨"   "⍙Bool1"    2 "F2_OR"         19
⍝ Z←Z,⊂ "⍙MInt"      "⊤∨"  "⍙Int"      2 "F2_OR_B"       20
⍝ Z←Z,⊂ "⍙MBool"     "⍲"   "⍙Bool1"    2 "F2_NAND"       21
⍝ Z←Z,⊂ "⍙MInt"      "⊤⍲"  "⍙Int"      2 "F2_NAND_B"     22
⍝ Z←Z,⊂ "⍙MBool"     "⍱"   "⍙Bool1"    2 "F2_NOR"        23
⍝ Z←Z,⊂ "⍙MInt"      "⊤⍱"  "⍙Int"      2 "F2_NOR_B"      24
⍝ Z←Z,⊂ "⍙MMix_IR"   "⌈"   "⍙Mix_IR"   2 "F12_RND_UP"    25
⍝ Z←Z,⊂ "⍙MMix_IR"   "⌊"   "⍙Mix_IR"   2 "F12_RND_DN"    26
⍝ Z←Z,⊂ "⍙MMix_IRC"  "!"   "⍙Mix_IRC"  2 "F12_BINOM"     27
⍝ Z←Z,⊂ "⍙MMix_IRC"  "⋆"   "⍙Mix_IRC"  2 "F12_POWER"     28
⍝ Z←Z,⊂ "⍙MMix1_IRC" "⍟"   "⍙Mix1_IRC" 2 "F12_LOGA"      29
⍝ Z←Z,⊂ "⍙MMix_IR "  "<"   "⍙Mix_IR"   2 "F2_LESS"       30
⍝ Z←Z,⊂ "⍙MMix_IR "  "≤"   "⍙Mix_IR"   2 "F2_LEQ"        31
⍝ Z←Z,⊂ "⍙MMix_IRC"  "="   "⍙Mix_IRC"  2 "F2_EQUAL"      32
⍝ Z←Z,⊂ "⍙MInt"      "⊤="  "⍙Int"      2 "F2_EQUAL_B"    33
⍝ Z←Z,⊂ "⍙MInt2"     "≠"   "⍙Mix_IRC"  2 "F2_UNEQ"       34
⍝ Z←Z,⊂ "⍙MInt"      "⊤≠"  "⍙Int"      2 "F2_UNEQ_B"     35
⍝ Z←Z,⊂ "⍙MMix_IR"   ">"   "⍙Mix_IR"   2 "F2_GREATER"    36
⍝ Z←Z,⊂ "⍙MMix_IR"   "≥"   "⍙Mix_IR"   2 "F2_MEQ"        37
  Z←Z,⊂ "¯6"         "○"   "⍙Mix_IRC"  2 "F12_CIRCLE"    38
⍝ Z←Z,⊂ "⍙MMix_IRC"  "∣"   "⍙Mix_IRC"  2 "F12_STILE"     39
⍝ Z←Z,⊂ "1 2 3"      "⋸"   "⍙Int"      2 "F12_FIND"      40
⍝ Z←Z,⊂ "⍙MMat1_IRC" "+.×" "⍙Mat1_IRC" 3 "OPER2_INNER"   41
⍝ Z←Z,⊂ "⍙MVec1_IRC" "∘.×" "⍙Vec1_IRC" 3 "OPER2_OUTER"   42
∇

∇INIT_DATA LEN;N;Ilen;Rlen;Clen
  ⍝⍝
  ⍝⍝ setup variables used in benchmark expressions:
  ⍝⍝ ⍙Int:  ¯2 ... 9
  ⍝⍝ ⍙Int1: nonzero Int
  ⍝⍝ ⍙Real: ¯10 to 10 or so
  ⍝⍝
  (Ilen Rlen Clen)←⍙PROFILE           ⍝ relative amounts of int, real, complex
  ⍙Int  ← 10 - ? Ilen ⍴ 12            ⍝ Integers -2 ... 10
  ⍙Int1 ← Ilen ⍴ (⍙Int≠0)/⍙Int        ⍝ non-zero Integers -10 ... 10
  ⍙Int2 ← Ilen ⍴ (⍙Int>0) / ⍙Int      ⍝ positive Integers 1 ... 10
  ⍙Bool ← 2 ∣ ⍙Int                    ⍝ Booleans
  ⍙Bool1← 1 ⌽ ⍙Bool                   ⍝ also Booleans
  ⍙Real ← Rlen ⍴ ⍙Int + 3 ÷ ○1        ⍝ reals
  ⍙Real1← Rlen ⍴ (⍙Real≠0)/⍙Real      ⍝ non-zero reals
  ⍙Real2← Rlen ⍴ (⍙Real>0)/⍙Real      ⍝ positive reals 1 ... 10
  ⍙Comp ← Clen ⍴ ⍙Real + 0J1×1⌽⍙Real  ⍝ complex
  ⍙Comp1← Clen ⍴ (⍙Comp≠0)/⍙Comp      ⍝ non-zero complex

  ⍙Mix_IR   ←⍙Int,⍙Real          ◊ ⍙Mix_IR  [N?N←⍴⍙Mix_IR  ]  ← ⍙Mix_IR
  ⍙Mix_IRC  ←⍙Int,⍙Real,⍙Comp    ◊ ⍙Mix_IRC [N?N←⍴⍙Mix_IRC ]  ← ⍙Mix_IRC
  ⍙Mix1_IRC ←⍙Int1,⍙Real1,⍙Comp1 ◊ ⍙Mix1_IRC[N?N←⍴⍙Mix1_IRC]  ← ⍙Mix1_IRC

  ⍙Int      ← LEN ⍴ ⍙Int
  ⍙Int1     ← LEN ⍴ ⍙Int1
  ⍙Int2     ← LEN ⍴ ⍙Int2
  ⍙Bool     ← LEN ⍴ ⍙Bool
  ⍙Bool1    ← LEN ⍴ ⍙Bool1
  ⍙Real     ← LEN ⍴ ⍙Real
  ⍙Real1    ← LEN ⍴ ⍙Real1
  ⍙Real2    ← LEN ⍴ ⍙Real2
  ⍙Comp     ← LEN ⍴ ⍙Comp
  ⍙Comp1    ← LEN ⍴ ⍙Comp1
  ⍙Mix_IR   ← LEN ⍴ ⍙Mix_IR
  ⍙Mix_IRC  ← LEN ⍴ ⍙Mix_IRC
  ⍙Mix1_IRC ← LEN ⍴ ⍙Mix1_IRC
  ⍙Mat1_IRC ← (2⍴⌈LEN⋆0.35)⍴⍙Mix1_IRC
  ⍙Vec1_IRC ← (⌈LEN⋆0.5)⍴⍙Mix1_IRC
∇

∇Z←VL ONE_PASS EXPR;JOB;OP;SID;TH1;TH2;STAT
  ⍝
  ⍝ Run one pass (for one vector length VL)
  ⍝ return (VL CYCLES), CYCLES is CPU cycles for all VL items
  ⍝
  ⍝ VL:   length of the vector
  ⍝ EXPR: A       OP  B          CAT SIDNAME    SID
  ⍝  e.g. Mix_IRC "+" "Mix_IRC"  2   "F12_PLUS" 13
  ⍝
  JOB←⊃,/EXPR[1 2 3]
  OP←⊃EXPR[2]      ⍝ e.g. +
  SID←EXPR[6]
  SID←42   ⍝ SCALAR_AB
  INIT_DATA VL

  ⍝ save current thresholds and set current thresholds to 1
  ⍝
  TH1← 1 FIO∆set_monadic_threshold OP
  TH2← 1 FIO∆set_dyadic_threshold  OP

  FIO∆clear_statistics SID ◊ ⊣⍎JOB ◊ STAT ← FIO∆get_statistics SID

  ⍝ restore saved thresholds
  ⊣ TH1 FIO∆set_monadic_threshold OP
  ⊣ TH2 FIO∆set_dyadic_threshold  OP

 Z←STAT[3 4 5]   ⍝ count / sum / sum2
 →0
∇

∇GO CORES;EXPR;XX;DATA;CAPTION;D12;D13;D45;D46;P_pit;P_sup;S_pit;S_sup;Wpos_X;Wpos_Y;ATT;I;LEN;SUM;SUM2;Q
  EXPR←⊃(DYA_EXPR[1])
  XX← 5 10 20 50
  DATA←((⍴XX), 6)⍴0
  CAPTION←⊂'caption: Z←', (⊃,/EXPR[1 2 3]), ' on ', (⍕CORES), ' cores'
  Wpos_X←⊂'pw_pos_X: ', ⍕200+30×CORES
  Wpos_Y←⊂'pw_pos_Y: ', ⍕200+30×CORES

  I←1 ◊ ⎕SYL[26;2] ← 0   ⍝ sequential
  ⊣ 100 ONE_PASS EXPR   ⍝ cache warm-up
LSeq:
  (LEN SUM SUM2) ← XX[I] ONE_PASS EXPR
  DATA[I;1 2]←(XX[I] + 1), SUM
  →((⍴XX) ≥ I←I+1)⍴LSeq

  I←1 ◊ ⎕SYL[26;2] ← CORES   ⍝ parallel
  ⊣ 100 ONE_PASS EXPR   ⍝ cache warm-up
LPar:
  (LEN SUM SUM2) ← XX[I] ONE_PASS EXPR
  DATA[I;4 5]←XX[I], SUM
  →((⍴XX) ≥ I←I+1)⍴LPar

  ⎕SYL[26;2] ← 0   ⍝ back to sequential

  Q←1,⍪DATA[;1]
  (S_sup S_pit)←⌊DATA[;2]⌹Q
  'Sequential: Startup' (6 0⍕S_sup) 'Per-Item:' (6 0⍕S_pit)
  (P_sup P_pit)←⌊DATA[;5]⌹Q
  'Parallel:   Startup' (6 0⍕P_sup) 'Per-Item:' (6 0⍕P_pit)
  'Speedup:    ' (0 2⍕S_pit÷P_pit) ' for' CORES 'cores'

  →(P_sup < S_sup)⍴GOON   ⍝ parallel setup is faster:    nonsense
  →(P_pit ≥ S_pit)⍴GOON   ⍝ parallel per item is slower: nonsense
  'Break-even: ' (⌈(P_sup - S_sup)÷(S_pit - P_pit))

GOON:
  DATA[;3]← S_sup + S_pit×DATA[;1]   ⍝ linearized lie
  DATA[;6]← P_sup + P_pit×DATA[;1]   ⍝ linearized lie
  D12←DATA[;1] + 0J1×DATA[;2]
  D13←DATA[;1] + 0J1×DATA[;3]
  D45←DATA[;4] + 0J1×DATA[;5]
  D46←DATA[;4] + 0J1×DATA[;6]
  D12←D12[1],D12
  D13←(0J1×S_sup),D13
  D45←D45[1],D45
  D46←(0J1×P_sup),D46

⍝ DATA
  ATT←"""
pa_border_L: 60
legend_X:    80
legend_Y:    45
legend_name-1: measured cycles parallel
legend_name-2: linearized cycles parallel
legend_name-3: measured cycles sequential
legend_name-4: linearized cycles sequential
line_width-2:  2
point_size-2:  6
line_width-4:  2
point_size-4:  6
      """
  ATT←CAPTION,Wpos_X,Wpos_Y,ATT
  ⊣ ATT ⎕PLOT D45⍪ D46⍪ D12 ,[0.5] D13
∇

GO 2
)MORE
GO 3
⍝GO 4
⍝GO 5
⍝GO 6
⍝GO 7
⍝GO 8
⍝GO 9
⍝GO 10
⍝GO 11
⍝GO 12
⎕
)VARS
)OFF
