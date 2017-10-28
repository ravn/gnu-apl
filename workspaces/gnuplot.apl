#!apl --script

⍝⍝ This is an APL workspace which is part of GNU APL. It demonstrates how
⍝⍝ the gnuplot program (see www.gnuplot.org) can be used from GNU APL for
⍝⍝ plotting numerical data. As an example it plots the result of ⎕FFT.

⍝ set ∆DATA to a matrix of data points that shall be gnu-plotted. Every
⍝ row of ∆DATA will be one plot-line. Formatting of the different lines
⍝ (color, legend, etc.) can be controlled via ∆Titles and ∆With
⍝

SINES  ← { (,⊃⍺/⊂⍵),↑⍵←1○0,⍵÷⍨○2×⍳⍵-1 }
PULSE  ← { (⍺+⍵)↑((⍺÷2),⍵)/0 1 }
SQUARE ← { 25 51 25/1 ¯1 1 }
CONST1 ← { ⍵⍴1 }

∇Z←FFT_data Samples
⍝⍝ construct data matrix Z for plotting Samples and their FFT
⍝
⍝ The matrix has 4 rows:
⍝ Z[1] : the X axis (0, 1, 2, ... N)
⍝ Z[2] : the Samples
⍝ Z[3] : the real part of the FFT of the samples
⍝ Z[4] : the imag part of the FFT of the samples

Z←((1,⍴Samples)⍴(-⎕IO-⍳⍴Samples))⍪⍉⍪Samples
Z←Z ⍪  9○⎕FFT Samples    ⍝ FFT result (real)
Z←Z ⍪ 11○⎕FFT Samples    ⍝ FFT result (imag)
∇

∇ATTRS plot DATA;REPL;H;NX;NY;Z;Cmd;Titles;Using;With;FMT
 (Titles With)←ATTRS ◊ Titles←⊃Titles ◊ With←⊃With
 (NY NX)←⍴DATA
 Z←,⊃{⍺,"\n", ⍵}/ "
    set grid xtics ytics
    set xlabel \"X-axis\"
    set ylabel \"Y-axis\"
                  "
 Cmd←"  plot" ◊ REPL←1
LOOP: Using←"using 1:",⍕REPL+1
 Z←Z, Cmd, " \"/tmp/plot_data\" ", Using
 Z←Z , " title \"", (⊃Titles[REPL]), "\" with lines " , (⊃With[REPL]), "\n"
 REPL←REPL+1 ◊ Cmd←"replot" ◊ →(REPL<NY)/LOOP
 Z←⎕UCS Z, "pause -1\n"

  H←"w"  ⎕FIO[3] "/tmp/plot_commands"   ⍝ open /tmp/plot_commands for writing
 ⊣ Z ⎕FIO[7] H                          ⍝ write plot settings and commands
 ⊣ (⎕FIO[4] H)                          ⍝ close /tmp/plot_commands

 FMT←⊂"%3d", (,(NY-1)⌿⍉⍪", %.2f"),"\n"
 H←"w" ⎕FIO[3]  "/tmp/plot_data"        ⍝ open /tmp/plot_data for writing
 ⊣ { (FMT, ⍵) ⎕FIO[22] H}¨⊂[1]DATA      ⍝ write plot data
 ⊣(⎕FIO[4] H)                           ⍝ close /tmp/plot_data
∇
)MORE

ZZ←   ⊂10 SINES 32     ⍝ [1]
ZZ←ZZ,⊂40 PULSE 61     ⍝ [2]
ZZ←ZZ,⊂   SQUARE       ⍝ [3]
ZZ←ZZ,⊂   CONST1 101   ⍝ [4]
Q←⎕INP "EOF"
Enter:
   1 for FFT SINES
   2 for FFT PULSE
   3 for FFT SQUARE
   4 for FFT CONST1
EOF

⊣{⎕←⍵}¨Q ◊ ∆DATA←⊃ZZ[⎕]

∆Titles ← ⊂("Input") ("FFT-Real") ("FFT-Imag")
∆With   ← ⊂("lt 2 lw 2") ("lt 1") ("lt 3")

(∆Titles ∆With) plot FFT_data ∆DATA
)HOST gnuplot /tmp/plot_commands

)OFF

