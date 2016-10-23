.data
.label main 0x50000000
.label loop 0x50000001
.text

LI $1, 0
LI $2, 32

SUBI $2, $2, 1
NOP
NOP
NOP
NOP
NOP
BGE  $2, $1, loop
NOP	
