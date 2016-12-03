.data
.float 0x40000000 "1.65"
.float 0x40000004 "-0.57"
.float 0x40000008 "0.0"
.float 0x4000000B "0.80"
.float 0x4000001f "-0.92"
.float 0x400000ff "0.0"
.float 0x400000bc "0.0"
.float 0x400000cb "0.0"

.label while 0x5000000f
.text

la $0, 0x40000000
ld $f0, 0($0)

la $1, 0x40000004
ld $f1, 0($1)

la $2, 0x40000008
ld $f2, 0($2)

la $3, 0x4000000B
ld $f3, 0($3)

la $4, 0x40000001f
ld $f4, 0($4)

la $5, 0x400000ff
ld $f5, 0($5)

la $25, 0x400000bc
la $26, 0x400000cb

li $20, 100
li $19, 0

fmult $f10, $f0, $f2
fsub $f6, $f10, $f1

fmult $f11, $f3, $f5
fsub $f7, $f11, $f4

ld $f1, 0($2)

sd $f6, 0($25)
ld $f2, 0($25)

ld $f4, 0($5)

sd $f7, 0($26)
ld $f5, 0($26)

addi $19, 1

bne while



