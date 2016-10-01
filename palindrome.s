.data
.label length_loop 0x50000003
.label end_length_loop 0x50000007
.label test_loop 0x50000008
.label is_palin 0x5000000f
.label not_palin 0x50000012 
.label exit 0x50000015

.asciiz 0x40000009 "Palindrome"
.asciiz 0x400000AA "Not Palindrome"
.space string_space 1024

.text
LA $0, string_space
SYSCALL 0
LA $1, string_space
LA $2, string_space

LB $3, ($2)
BEQZ $3, end_length_loop

ADDI $2, $2, 1
B length_loop

SUBI $2, $2, 1

BGE $1, $2, is_palin

LB $3, ($1)
LB $4, ($2)

BNE $3, $4, not_palin

ADDI $1, $1, 1
SUBI $2, $2, 1
B test_loop

LA $0, 0x40000009
SYSCALL 1
B exit

LA $0, 0x400000AA
SYSCALL 1
B exit


SYSCALL 9

