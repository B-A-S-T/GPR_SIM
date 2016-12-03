.data
.float 0x40000000 "1.65"
.float 0x40000001 "-0.57"
.float 0x40000002 "0.0"
.float 0x40000003 "0.80"
.float 0x40000004 "-0.92"
.float 0x40000005 "0.0"
.float 0x40000006 "0.0"
.float 0x40000007 "0.0"

.label while 0x5000000f

.text

la $0, 0x40000000
ld $f0, ($0)

la $1, 0x40000001
ld $f1, ($1)

la $2, 0x40000002
ld $f2, ($2)

la $3, 0x40000003
ld $f3, ($3)

la $4, 0x40000004
ld $f4, ($4)

la $5, 0x40000005
ld $f5, ($5)

la $25, 0x40000006
la $26, 0x40000007

li $20, 100
li $19, 0

fmult $f10, $f0, $f2
fsub $f6, $f10, $f1

fmult $f11, $f3, $f5
fsub $f7, $f11, $f4

ld $f1, ($2)

sd $f6, ($25)
ld $f2, ($25)

ld $f4, ($5)

sd $f7, ($26)
ld $f5, ($26)

addi $19, $19, 1

bne $19, $20, while



