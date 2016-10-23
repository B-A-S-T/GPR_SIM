.data
.label test 0x50000008
.space string_space 1024

.text
la $0, string_space
nop
nop
syscall 0
la $1, string_space
li $5, 0
li $6, 2
lb $3, ($1)
nop
addi $3, $3, 1
addi $5, $5, 1
nop
bne $5, $6, test
nop



