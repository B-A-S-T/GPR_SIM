.data
.label loop 0x50000005
.text

li $0, 0
li $1, 0
li $2, 10
li $3, 100
li $4, 1000
li $5, 7

subi $5, $5, 1
add $1, $1, $2
add $1, $1, $3
add $1, $1, $4
bge $5, $0, loop
nop

add $4, $0, $1

