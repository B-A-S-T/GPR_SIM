.data
.label test 0x50000008
.space string_space 1024

.text
LA $0, string_space
NOP
NOP
SYSCALL 0
LA $1, string_space
LI $5, 0
LI $6, 2
LB $3, ($1)
NOP
ADDI $3, $3, 1
ADDI $5, $5, 1
NOP
BNE $5, $6, test
NOP



