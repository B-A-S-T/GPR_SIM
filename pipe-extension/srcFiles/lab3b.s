.data
.label length_loop 0x50000005
.label end_length_loop 0x5000000d
.label test_loop 0x5000000f
.label is_palin 0x5000001c
.label not_palin 0x50000022
.label exit 0x50000027

.asciiz 0x40000009 "Palindrome"
.asciiz 0x400000aa "Not Palindrome"
.space string_space 1024

.text
la $0, string_space
nop
nop
syscall 0
la $4, string_space
la $10, string_space

lb $11, ($10)
nop
nop
beqz $11, end_length_loop
nop
addi $10, $10, 1
b length_loop
nop

subi $10, $10, 1
nop

bge $4, $10, is_palin
nop
lb $11, ($4)
nop
lb $12, ($10)
nop
nop
bne $11, $12, not_palin
nop

addi $4, $4, 1
subi $10, $10, 1
b test_loop
nop

la $0, 0x40000009
nop
nop
syscall 1
b exit
nop

la $0, 0x400000aa
nop
nop
syscall 1
b exit
nop

exit:
