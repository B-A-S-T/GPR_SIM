.data
.label tast 0x50000003
.space string_space 1024

.text
LA $0, string_space
NOP
NOP
SYSCALL 0
LA $1, string_space
LI $5, 1
LB $3, ($1)
NOP
ADD $3, $3, $5

