	.file	"xxxx.c"
	.section	".rodata"
.LLC0:
	.byte	0
	.byte	1
	.byte	2
	.byte	2
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.align 8
.LLC1:
	.asciz	"%3d: %d\n"
	.section	".text"
	.align 4
	.global x
	.type	x, #function
	.proc	020
x:
	!#PROLOGUE# 0
	save	%sp, -136, %sp
	!#PROLOGUE# 1
	st	%i0, [%fp+68]
	ld	[%fp+68], %g1
	st	%g1, [%fp-20]
	st	%g0, [%fp-24]
	ld	[%fp+68], %o5
	sethi	%hi(-65536), %g1
	and	%o5, %g1, %g1
	cmp	%g1, 0
	be	.LL2
	nop
	ld	[%fp+68], %g1
	sra	%g1, 16, %g1
	st	%g1, [%fp+68]
	ld	[%fp-24], %g1
	add	%g1, 16, %g1
	st	%g1, [%fp-24]
.LL2:
	ld	[%fp+68], %o5
	sethi	%hi(64512), %g1
	or	%g1, 768, %g1
	and	%o5, %g1, %g1
	cmp	%g1, 0
	be	.LL3
	nop
	ld	[%fp+68], %g1
	sra	%g1, 8, %g1
	st	%g1, [%fp+68]
	ld	[%fp-24], %g1
	add	%g1, 8, %g1
	st	%g1, [%fp-24]
.LL3:
	ld	[%fp+68], %g1
	and	%g1, 240, %g1
	cmp	%g1, 0
	be	.LL4
	nop
	ld	[%fp+68], %g1
	sra	%g1, 4, %g1
	st	%g1, [%fp+68]
	ld	[%fp-24], %g1
	add	%g1, 4, %g1
	st	%g1, [%fp-24]
.LL4:
	sethi	%hi(.LLC0), %g1
	or	%g1, %lo(.LLC0), %g1
	add	%fp, -40, %o5
	mov	16, %o4
	mov	%o5, %o0
	mov	%g1, %o1
	mov	%o4, %o2
	call	memcpy, 0
	 nop
	ld	[%fp+68], %o5
	add	%fp, -16, %g1
	add	%g1, %o5, %g1
	ldub	[%g1-24], %g1
	sll	%g1, 24, %g1
	sra	%g1, 24, %o5
	ld	[%fp-24], %g1
	add	%g1, %o5, %g1
	st	%g1, [%fp-24]
	sethi	%hi(.LLC1), %g1
	or	%g1, %lo(.LLC1), %o0
	ld	[%fp-20], %o1
	ld	[%fp-24], %o2
	call	printf, 0
	 nop
	nop
	ret
	restore
	.size	x, .-x
	.align 4
	.global main
	.type	main, #function
	.proc	04
main:
	!#PROLOGUE# 0
	save	%sp, -120, %sp
	!#PROLOGUE# 1
	st	%i0, [%fp+68]
	st	%i1, [%fp+72]
	st	%g0, [%fp-20]
.LL6:
	ld	[%fp-20], %g1
	cmp	%g1, 39
	ble	.LL9
	nop
	b	.LL7
	 nop
.LL9:
	ld	[%fp-20], %o0
	call	x, 0
	 nop
	ld	[%fp-20], %g1
	add	%g1, 1, %g1
	st	%g1, [%fp-20]
	b	.LL6
	 nop
.LL7:
	sethi	%hi(4096), %g1
	or	%g1, 615, %o0
	call	x, 0
	 nop
	sethi	%hi(1233920), %g1
	or	%g1, 647, %o0
	call	x, 0
	 nop
	sethi	%hi(3428352), %g1
	or	%g1, 1, %o0
	call	x, 0
	 nop
	mov	%g1, %i0
	ret
	restore
	.size	main, .-main
	.ident	"GCC: (GNU) 3.3.1"
