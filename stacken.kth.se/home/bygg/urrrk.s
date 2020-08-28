	.file	"urrrk.c"
	.section	".text"
	.align 4
	.global x_foo
	.type	x_foo,#function
	.proc	020
x_foo:
	!#PROLOGUE# 0
	save	%sp, -112, %sp
	!#PROLOGUE# 1
	nop
	ret
	restore
.LLfe1:
	.size	x_foo,.LLfe1-x_foo
	.align 4
	.global x_bar
	.type	x_bar,#function
	.proc	020
x_bar:
	!#PROLOGUE# 0
	save	%sp, -112, %sp
	!#PROLOGUE# 1
	nop
	ret
	restore
.LLfe2:
	.size	x_bar,.LLfe2-x_bar
	.section	".rodata"
	.align 8
.LLC0:
	.asciz	"foo"
	.align 8
.LLC1:
	.asciz	"bar"
	.align 8
.LLC2:
	.asciz	"urk"
	.align 8
.LLC3:
	.asciz	"urrrk"
	.section	".data"
	.align 4
	.type	k1,#object
	.size	k1,36
k1:
	.long	.LLC0
	.long	0
	.long	x_foo
	.long	.LLC1
	.long	0
	.long	x_bar
	.long	.LLC2
	.long	1
	.long	.LLC3
	.align 4
	.type	k2,#object
	.size	k2,36
k2:
	.long	.LLC0
	.long	0
	.long	x_foo
	.long	.LLC1
	.long	0
	.long	x_bar
	.long	.LLC2
	.long	1
	.long	.LLC3
	.section	".rodata"
	.align 8
.LLC4:
	.asciz	"short: %d\n"
	.align 8
.LLC5:
	.asciz	"int:   %d\n"
	.align 8
.LLC6:
	.asciz	"long:  %d\n"
	.align 8
.LLC7:
	.asciz	"llong: %d\n"
	.align 8
.LLC8:
	.asciz	"void*: %d\n"
	.align 8
.LLC9:
	.asciz	"func*: %d\n"
	.section	".text"
	.align 4
	.global main
	.type	main,#function
	.proc	04
main:
	!#PROLOGUE# 0
	save	%sp, -112, %sp
	!#PROLOGUE# 1
	st	%i0, [%fp+68]
	st	%i1, [%fp+72]
	sethi	%hi(.LLC4), %o0
	or	%o0, %lo(.LLC4), %o0
	mov	2, %o1
	call	printf, 0
	 nop
	sethi	%hi(.LLC5), %o0
	or	%o0, %lo(.LLC5), %o0
	mov	4, %o1
	call	printf, 0
	 nop
	sethi	%hi(.LLC6), %o0
	or	%o0, %lo(.LLC6), %o0
	mov	4, %o1
	call	printf, 0
	 nop
	sethi	%hi(.LLC7), %o0
	or	%o0, %lo(.LLC7), %o0
	mov	8, %o1
	call	printf, 0
	 nop
	sethi	%hi(.LLC8), %o0
	or	%o0, %lo(.LLC8), %o0
	mov	4, %o1
	call	printf, 0
	 nop
	sethi	%hi(.LLC9), %o0
	or	%o0, %lo(.LLC9), %o0
	mov	4, %o1
	call	printf, 0
	 nop
	mov	%o0, %i0
	nop
	ret
	restore
.LLfe3:
	.size	main,.LLfe3-main
	.ident	"GCC: (GNU) 3.2"
