#
# This file puts APL characters on the Alt/ShiftAlt plane of your (US-)keyboard.
#
# Install (as root) with:
#
#	loadkeys keys.apl
#
#
# You may also want to:
#
#	unicode_start Unifont-APL8x16.psf
#
# to see the proper chars
#
#
keymaps 0-127
keycode   1 = Escape          
		alt	keycode   1 = Meta_Escape     
	shift	alt	keycode   1 = Meta_Escape     
keycode   2 = one              exclam           one              one             
		alt	keycode   2 = U+00A8
	shift	alt	keycode   2 = U+00A1
keycode   3 = two             
	shift	keycode   3 = at              
		alt	keycode   3 = U+00AF
	shift	alt	keycode   3 = U+20AC
keycode   4 = three           
	shift	keycode   4 = numbersign      
		alt	keycode   4 = less
	shift	alt	keycode   4 = U+00A3
keycode   5 = four            
	shift	keycode   5 = dollar          
		alt	keycode   5 = U+2264
	shift	alt	keycode   5 = U+2367
keycode   6 = five            
	shift	keycode   6 = percent         
		alt	keycode   6 = Meta_five       
	shift	alt	keycode   6 = Meta_percent    
keycode   7 = six             
	shift	keycode   7 = asciicircum     
		alt	keycode   7 = U+2265
keycode   8 = seven           
	shift	keycode   8 = ampersand       
		alt	keycode   8 = greater
	shift	alt	keycode   8 = greater
keycode   9 = eight           
	shift	keycode   9 = asterisk        
		alt	keycode   9 = U+2260
	shift	alt	keycode   9 = U+2342
keycode  10 = nine             parenleft        nine             nine            
		alt	keycode  10 = U+2228
	shift	alt	keycode  10 = U+2371
keycode  11 = zero             parenright       zero             zero            
		alt	keycode  11 = U+2227
	shift	alt	keycode  11 = U+2372
keycode  12 = minus           
	shift	keycode  12 = underscore      
		alt	keycode  12 = U+00D7
	shift	alt	keycode  12 = U+2261
keycode  13 = equal            plus             equal            equal           
		alt	keycode  13 = U+00F7
	shift	alt	keycode  13 = U+2339
keycode  14 = Delete          
		alt	keycode  14 = Meta_Delete     
	shift	alt	keycode  14 = Meta_Delete     
keycode  15 = Tab             
		alt	keycode  15 = Meta_Tab        
	shift	alt	keycode  15 = Meta_Tab        
keycode  16 = +q              
	shift	keycode  16 = +Q              
		alt	keycode  16 = question
	shift	alt	keycode  16 = U+00BF
keycode  17 = +w              
	shift	keycode  17 = +W              
		alt	keycode  17 = U+2375
	shift	alt	keycode  17 = U+233D
keycode  18 = +e              
	shift	keycode  18 = +E              
		alt	keycode  18 = U+2208
	shift	alt	keycode  18 = U+22F8
keycode  19 = +r              
	shift	keycode  19 = +R              
		alt	keycode  19 = U+2374
keycode  20 = +t              
	shift	keycode  20 = +T              
		alt	keycode  20 = U+223C
	shift	alt	keycode  20 = U+2349
keycode  21 = +y              
	shift	keycode  21 = +Y              
		alt	keycode  21 = U+2191
	shift	alt	keycode  21 = U+00A5
keycode  22 = +u              
	shift	keycode  22 = +U              
		alt	keycode  22 = U+2193
keycode  23 = +i              
	shift	keycode  23 = +I              
		alt	keycode  23 = U+2373
	shift	alt	keycode  23 = U+2378
keycode  24 = +o              
	shift	keycode  24 = +O              
		alt	keycode  24 = U+25CB
	shift	alt	keycode  24 = U+2365
keycode  25 = +p              
	shift	keycode  25 = +P              
		alt	keycode  25 = U+22C6
	shift	alt	keycode  25 = U+235F
keycode  26 = bracketleft     
	shift	keycode  26 = braceleft       
		alt	keycode  26 = U+2190
keycode  27 = bracketright    
	shift	keycode  27 = braceright      
		alt	keycode  27 = U+2192
	shift	alt	keycode  27 = U+236C
keycode  28 = Return          
		alt	keycode  28 = Meta_Control_m
	shift	alt	keycode  28 = Meta_Control_m
keycode  29 = Control         
keycode  30 = +a              
	shift	keycode  30 = +A              
		alt	keycode  30 = U+237A
	shift	alt	keycode  30 = U+2296
keycode  31 = +s              
	shift	keycode  31 = +S              
		alt	keycode  31 = U+2308
keycode  32 = +d              
	shift	keycode  32 = +D              
		alt	keycode  32 = U+2308
keycode  33 = +f              
	shift	keycode  33 = +F              
		alt	keycode  33 = underscore
	shift	alt	keycode  33 = U+236B
keycode  34 = +g              
	shift	keycode  34 = +G              
		alt	keycode  34 = U+2207
	shift	alt	keycode  34 = U+2352
keycode  35 = +h              
	shift	keycode  35 = +H              
		alt	keycode  35 = U+2206
	shift	alt	keycode  35 = U+234B
keycode  36 = +j              
	shift	keycode  36 = +J              
		alt	keycode  36 = U+2218
	shift	alt	keycode  36 = U+2364
keycode  37 = +k              
	shift	keycode  37 = +K              
		alt	keycode  37 = apostrophe
	shift	alt	keycode  37 = U+233B
keycode  38 = +l              
	shift	keycode  38 = +L              
		alt	keycode  38 = U+2395
	shift	alt	keycode  38 = U+235E
keycode  39 = semicolon        colon            semicolon        semicolon       
		alt	keycode  39 = U+22A2
keycode  40 = apostrophe      
	shift	keycode  40 = quotedbl        
		alt	keycode  40 = U+22A2
keycode  41 = grave           
	shift	keycode  41 = asciitilde      
		alt	keycode  41 = U+25CA
	shift	alt	keycode  41 = U+2368
keycode  42 = Shift           
keycode  43 = backslash       
	shift	keycode  43 = bar             
		alt	keycode  43 = U+235D
	shift	alt	keycode  43 = U+2340
keycode  44 = +z              
	shift	keycode  44 = +Z              
		alt	keycode  44 = U+2282
keycode  45 = +x              
	shift	keycode  45 = +X              
		alt	keycode  45 = U+2283
keycode  46 = +c              
	shift	keycode  46 = +C              
		alt	keycode  46 = U+2229
	shift	alt	keycode  46 = U+235D
keycode  47 = +v              
	shift	keycode  47 = +V              
		alt	keycode  47 = U+222A
keycode  48 = +b              
	shift	keycode  48 = +B              
		alt	keycode  48 = U+22A5
	shift	alt	keycode  48 = U+234E
keycode  49 = +n              
	shift	keycode  49 = +N              
		alt	keycode  49 = U+22A4
	shift	alt	keycode  49 = U+2355
keycode  50 = +m              
	shift	keycode  50 = +M              
		alt	keycode  50 = U+2223
	shift	alt	keycode  50 = U+2336
keycode  51 = comma            less             comma            comma           
		alt	keycode  51 = U+2337
	shift	alt	keycode  51 = U+236A
keycode  52 = period          
	shift	keycode  52 = greater         
		alt	keycode  52 = U+234E
	shift	alt	keycode  52 = U+2359
keycode  53 = slash           
	shift	keycode  53 = question        
		alt	keycode  53 = U+2355
	shift	alt	keycode  53 = U+233F
keycode  54 = Shift           
keycode  55 = KP_Multiply     
		altgr	keycode  55 = Hex_C           
	shift	alt	keycode  55 = Hex_C           
keycode  56 = Alt             
keycode  57 = space           
		alt	keycode  57 = Meta_space      
	shift	alt	keycode  57 = Meta_space      
keycode  58 = CtrlL_Lock      
keycode  59 = F1              
	shift	keycode  59 = F13             
		altgr	keycode  59 = Console_13      
	shift	altgr	keycode  59 = Console_25      
		alt	keycode  59 = Console_1       
	shift	alt	keycode  59 = Console_13      
keycode  60 = F2              
	shift	keycode  60 = F14             
		altgr	keycode  60 = Console_14      
	shift	altgr	keycode  60 = Console_26      
		alt	keycode  60 = Console_2       
	shift	alt	keycode  60 = Console_14      
keycode  61 = F3              
	shift	keycode  61 = F15             
		altgr	keycode  61 = Console_15      
	shift	altgr	keycode  61 = Console_27      
		alt	keycode  61 = Console_3       
	shift	alt	keycode  61 = Console_15      
keycode  62 = F4              
	shift	keycode  62 = F16             
		altgr	keycode  62 = Console_16      
	shift	altgr	keycode  62 = Console_28      
		alt	keycode  62 = Console_4       
	shift	alt	keycode  62 = Console_16      
keycode  63 = F5              
	shift	keycode  63 = F17             
		altgr	keycode  63 = Console_17      
	shift	altgr	keycode  63 = Console_29      
		alt	keycode  63 = Console_5       
	shift	alt	keycode  63 = Console_17      
keycode  64 = F6              
	shift	keycode  64 = F18             
		altgr	keycode  64 = Console_18      
	shift	altgr	keycode  64 = Console_30      
		alt	keycode  64 = Console_6       
	shift	alt	keycode  64 = Console_18      
keycode  65 = F7              
	shift	keycode  65 = F19             
		altgr	keycode  65 = Console_19      
	shift	altgr	keycode  65 = Console_31      
		alt	keycode  65 = Console_7       
	shift	alt	keycode  65 = Console_19      
keycode  66 = F8              
	shift	keycode  66 = F20             
		altgr	keycode  66 = Console_20      
	shift	altgr	keycode  66 = Console_32      
		alt	keycode  66 = Console_8       
	shift	alt	keycode  66 = Console_20      
keycode  67 = F9              
	shift	keycode  67 = F21             
		altgr	keycode  67 = Console_21      
	shift	altgr	keycode  67 = Console_33      
		alt	keycode  67 = Console_9       
	shift	alt	keycode  67 = Console_21      
keycode  68 = F10             
	shift	keycode  68 = F22             
		altgr	keycode  68 = Console_22      
	shift	altgr	keycode  68 = Console_34      
		alt	keycode  68 = Console_10      
	shift	alt	keycode  68 = Console_22      
keycode  69 = Num_Lock        
		altgr	keycode  69 = Hex_A           
	shift	alt	keycode  69 = Hex_A           
keycode  70 = Scroll_Lock     
	shift	keycode  70 = Show_Memory     
		altgr	keycode  70 = Show_Registers  
		alt	keycode  70 = Show_Registers  
keycode  71 = KP_7            
		altgr	keycode  71 = Hex_7           
		alt	keycode  71 = Ascii_7         
	shift	alt	keycode  71 = Hex_7           
keycode  72 = KP_8            
		altgr	keycode  72 = Hex_8           
		alt	keycode  72 = Ascii_8         
	shift	alt	keycode  72 = Hex_8           
keycode  73 = KP_9            
		altgr	keycode  73 = Hex_9           
		alt	keycode  73 = Ascii_9         
	shift	alt	keycode  73 = Hex_9           
keycode  74 = KP_Subtract     
		altgr	keycode  74 = Hex_D           
	shift	alt	keycode  74 = Hex_D           
keycode  75 = KP_4            
		altgr	keycode  75 = Hex_4           
		alt	keycode  75 = Ascii_4         
	shift	alt	keycode  75 = Hex_4           
keycode  76 = KP_5            
		altgr	keycode  76 = Hex_5           
		alt	keycode  76 = Ascii_5         
	shift	alt	keycode  76 = Hex_5           
keycode  77 = KP_6            
		altgr	keycode  77 = Hex_6           
		alt	keycode  77 = Ascii_6         
	shift	alt	keycode  77 = Hex_6           
keycode  78 = KP_Add          
		altgr	keycode  78 = Hex_E           
	shift	alt	keycode  78 = Hex_E           
keycode  79 = KP_1            
		altgr	keycode  79 = Hex_1           
		alt	keycode  79 = Ascii_1         
	shift	alt	keycode  79 = Hex_1           
keycode  80 = KP_2            
		altgr	keycode  80 = Hex_2           
		alt	keycode  80 = Ascii_2         
	shift	alt	keycode  80 = Hex_2           
keycode  81 = KP_3            
		altgr	keycode  81 = Hex_3           
		alt	keycode  81 = Ascii_3         
	shift	alt	keycode  81 = Hex_3           
keycode  82 = KP_0            
		altgr	keycode  82 = Hex_0           
		alt	keycode  82 = Ascii_0         
	shift	alt	keycode  82 = Hex_0           
keycode  83 = KP_Period       
keycode  84 = Last_Console     Last_Console     Last_Console    
		alt	keycode  84 = Last_Console    
keycode  85 =
keycode  86 = less             greater          bar             
		alt	keycode  86 = Meta_less       
	shift	alt	keycode  86 = Meta_greater    
keycode  87 = F11             
	shift	keycode  87 = F23             
		altgr	keycode  87 = Console_23      
	shift	altgr	keycode  87 = Console_35      
		alt	keycode  87 = Console_11      
	shift	alt	keycode  87 = Console_23      
keycode  88 = F12             
	shift	keycode  88 = F24             
		altgr	keycode  88 = Console_24      
	shift	altgr	keycode  88 = Console_36      
		alt	keycode  88 = Console_12      
	shift	alt	keycode  88 = Console_24      
keycode  89 =
keycode  90 =
keycode  91 =
keycode  92 =
keycode  93 =
keycode  94 =
keycode  95 =
keycode  96 = KP_Enter        
		altgr	keycode  96 = Hex_F           
	shift	alt	keycode  96 = Hex_F           
keycode  97 = Control         
keycode  98 = KP_Divide       
		altgr	keycode  98 = Hex_B           
	shift	alt	keycode  98 = Hex_B           
keycode  99 =
		altgr	keycode  99 = Control_backslash
	shift	altgr	keycode  99 = Control_backslash
		alt	keycode  99 = Control_backslash
	shift	alt	keycode  99 = Control_backslash
keycode 100 = Alt             
keycode 101 = Break            Break            Break           
		alt	keycode 101 = Break           
keycode 102 = Find            
keycode 103 = Up              
		alt	keycode 103 = KeyboardSignal  
keycode 104 = Prior           
	shift	keycode 104 = Scroll_Backward 
keycode 105 = Left            
		alt	keycode 105 = Decr_Console    
keycode 106 = Right           
		alt	keycode 106 = Incr_Console    
keycode 107 = Select          
keycode 108 = Down            
keycode 109 = Next            
	shift	keycode 109 = Scroll_Forward  
keycode 110 = Insert          
keycode 111 = Remove          
keycode 112 = Macro            Macro            Macro           
		alt	keycode 112 = Macro           
keycode 113 = F13              F13              F13             
		alt	keycode 113 = F13             
keycode 114 = F14              F14              F14             
		alt	keycode 114 = F14             
keycode 115 = Help             Help             Help            
		alt	keycode 115 = Help            
keycode 116 = Do               Do               Do              
		alt	keycode 116 = Do              
keycode 117 = F17              F17              F17             
		alt	keycode 117 = F17             
keycode 118 = KP_MinPlus       KP_MinPlus       KP_MinPlus      
		alt	keycode 118 = KP_MinPlus      
keycode 119 = Pause           
keycode 120 =
keycode 121 =
keycode 122 =
keycode 123 =
keycode 124 =
keycode 125 = Alt             
keycode 126 = Alt             
keycode 127 =
keycode 128 =
keycode 129 =
keycode 130 =
keycode 131 =
keycode 132 =
keycode 133 =
keycode 134 =
keycode 135 =
keycode 136 =
keycode 137 =
keycode 138 =
keycode 139 =
keycode 140 =
keycode 141 =
keycode 142 =
keycode 143 =
keycode 144 =
keycode 145 =
keycode 146 =
keycode 147 =
keycode 148 =
keycode 149 =
keycode 150 =
keycode 151 =
keycode 152 =
keycode 153 =
keycode 154 =
keycode 155 =
keycode 156 =
keycode 157 =
keycode 158 =
keycode 159 =
keycode 160 =
keycode 161 =
keycode 162 =
keycode 163 =
keycode 164 =
keycode 165 =
keycode 166 =
keycode 167 =
keycode 168 =
keycode 169 =
keycode 170 =
keycode 171 =
keycode 172 =
keycode 173 =
keycode 174 =
keycode 175 =
keycode 176 =
keycode 177 =
keycode 178 =
keycode 179 =
keycode 180 =
keycode 181 =
keycode 182 =
keycode 183 =
keycode 184 =
keycode 185 =
keycode 186 =
keycode 187 =
keycode 188 =
keycode 189 =
keycode 190 =
keycode 191 =
keycode 192 =
keycode 193 =
keycode 194 =
keycode 195 =
keycode 196 =
keycode 197 =
keycode 198 =
keycode 199 =
keycode 200 =
keycode 201 =
keycode 202 =
keycode 203 =
keycode 204 =
keycode 205 =
keycode 206 =
keycode 207 =
keycode 208 =
keycode 209 =
keycode 210 =
keycode 211 =
keycode 212 =
keycode 213 =
keycode 214 =
keycode 215 =
keycode 216 =
keycode 217 =
keycode 218 =
keycode 219 =
keycode 220 =
keycode 221 =
keycode 222 =
keycode 223 =
keycode 224 =
keycode 225 =
keycode 226 =
keycode 227 =
keycode 228 =
keycode 229 =
keycode 230 =
keycode 231 =
keycode 232 =
keycode 233 =
keycode 234 =
keycode 235 =
keycode 236 =
keycode 237 =
keycode 238 =
keycode 239 =
keycode 240 =
keycode 241 =
keycode 242 =
keycode 243 =
keycode 244 =
keycode 245 =
keycode 246 =
keycode 247 =
keycode 248 =
keycode 249 =
keycode 250 =
keycode 251 =
keycode 252 =
keycode 253 =
keycode 254 =
keycode 255 =
string F1 = "\033[[A"
string F2 = "\033[[B"
string F3 = "\033[[C"
string F4 = "\033[[D"
string F5 = "\033[[E"
string F6 = "\033[17~"
string F7 = "\033[18~"
string F8 = "\033[19~"
string F9 = "\033[20~"
string F10 = "\033[21~"
string F11 = "\033[23~"
string F12 = "\033[24~"
string F13 = "\033[25~"
string F14 = "\033[26~"
string F15 = "\033[28~"
string F16 = "\033[29~"
string F17 = "\033[31~"
string F18 = "\033[32~"
string F19 = "\033[33~"
string F20 = "\033[34~"
string Find = "\033[1~"
string Insert = "\033[2~"
string Remove = "\033[3~"
string Select = "\033[4~"
string Prior = "\033[5~"
string Next = "\033[6~"
string Macro = "\033[M"
string Pause = "\033[P"
compose '`' 'A' to '�'
compose '`' 'a' to '�'
compose '\'' 'A' to '�'
compose '\'' 'a' to '�'
compose '^' 'A' to '�'
compose '^' 'a' to '�'
compose '~' 'A' to '�'
compose '~' 'a' to '�'
compose '"' 'A' to '�'
compose '"' 'a' to '�'
compose 'O' 'A' to '�'
compose 'o' 'a' to '�'
compose '0' 'A' to '�'
compose '0' 'a' to '�'
compose 'A' 'A' to '�'
compose 'a' 'a' to '�'
compose 'A' 'E' to '�'
compose 'a' 'e' to '�'
compose ',' 'C' to '�'
compose ',' 'c' to '�'
compose '`' 'E' to '�'
compose '`' 'e' to '�'
compose '\'' 'E' to '�'
compose '\'' 'e' to '�'
compose '^' 'E' to '�'
compose '^' 'e' to '�'
compose '"' 'E' to '�'
compose '"' 'e' to '�'
compose '`' 'I' to '�'
compose '`' 'i' to '�'
compose '\'' 'I' to '�'
compose '\'' 'i' to '�'
compose '^' 'I' to '�'
compose '^' 'i' to '�'
compose '"' 'I' to '�'
compose '"' 'i' to '�'
compose '-' 'D' to '�'
compose '-' 'd' to '�'
compose '~' 'N' to '�'
compose '~' 'n' to '�'
compose '`' 'O' to '�'
compose '`' 'o' to '�'
compose '\'' 'O' to '�'
compose '\'' 'o' to '�'
compose '^' 'O' to '�'
compose '^' 'o' to '�'
compose '~' 'O' to '�'
compose '~' 'o' to '�'
compose '"' 'O' to '�'
compose '"' 'o' to '�'
compose '/' 'O' to '�'
compose '/' 'o' to '�'
compose '`' 'U' to '�'
compose '`' 'u' to '�'
compose '\'' 'U' to '�'
compose '\'' 'u' to '�'
compose '^' 'U' to '�'
compose '^' 'u' to '�'
compose '"' 'U' to '�'
compose '"' 'u' to '�'
compose '\'' 'Y' to '�'
compose '\'' 'y' to '�'
compose 'T' 'H' to '�'
compose 't' 'h' to '�'
compose 's' 's' to '�'
compose '"' 'y' to '�'
compose 's' 'z' to '�'
compose 'i' 'j' to '�'
