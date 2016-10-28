.data
.label main 0x50000000
.label loop 0x50000001
.text

li $1, 0
li $2, 32

subi $2, $2, 1
nop
nop
nop
nop
nop
bge  $2, $1, loop
nop	
