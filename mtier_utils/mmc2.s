	.file	"mm.c"
	.section	.rodata
	.align 4
.LC1:
	.string	"usage: %s size iters protect seed\n"
	.align 4
.LC2:
	.string	"multiplying two %dx%d matrices (%d iterations)\n"
	.align 4
.LC3:
	.string	"one or more matrix could not be allocated. Aborting\n"
.LC5:
	.string	"failed to mprotect m1: %d\n"
.LC6:
	.string	"failed to mprotect m2: $d\n"
.LC7:
	.string	"m1:"
.LC8:
	.string	"%1.4f "
.LC9:
	.string	"\nm2:"
.LC10:
	.string	"\n"
	.align 4
.LC12:
	.string	"Iter %d: finished in %.5f seconds\n"
.LC13:
	.string	"r:"
	.align 4
.LC14:
	.string	"Failed to restore writability to m1: %d\n"
	.align 4
.LC15:
	.string	"Failed to restore writability to m2: %d\n"
	.align 4
.LC16:
	.string	"%d iterations in %.4f s (%.5f s per iter)\n"
	.text
.globl main
	.type	main, @function
main:
	pushl	%ebp
	movl	%esp, %ebp
	andl	$-16, %esp
	pushl	%ebx
	subl	$156, %esp
	movl	$1, 100(%esp)
	movl	$0, 104(%esp)
	fldz
	fstpl	136(%esp)
	cmpl	$5, 8(%ebp)
	je	.L2
	movl	12(%ebp), %eax
	movl	(%eax), %edx
	movl	$.LC1, %eax
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$0, %eax
	jmp	.L3
.L2:
	movl	12(%ebp), %eax
	addl	$4, %eax
	movl	(%eax), %eax
	movl	%eax, (%esp)
	call	atoi
	movl	%eax, 76(%esp)
	movl	12(%ebp), %eax
	addl	$8, %eax
	movl	(%eax), %eax
	movl	%eax, (%esp)
	call	atoi
	movl	%eax, 80(%esp)
	movl	12(%ebp), %eax
	addl	$12, %eax
	movl	(%eax), %eax
	movl	%eax, (%esp)
	call	atoi
	movl	%eax, 100(%esp)
	movl	12(%ebp), %eax
	addl	$16, %eax
	movl	(%eax), %eax
	movl	%eax, (%esp)
	call	atoi
	movl	%eax, 104(%esp)
	movl	$.LC2, %eax
	movl	80(%esp), %edx
	movl	%edx, 12(%esp)
	movl	76(%esp), %edx
	movl	%edx, 8(%esp)
	movl	76(%esp), %edx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$0, 96(%esp)
	jmp	.L4
.L35:
	movl	104(%esp), %eax
	movl	%eax, (%esp)
	call	srand
	movl	76(%esp), %edx
	movl	76(%esp), %eax
	imull	%edx, %eax
	sall	$3, %eax
	movl	%eax, (%esp)
	call	malloc
	movl	%eax, 108(%esp)
	movl	76(%esp), %edx
	movl	76(%esp), %eax
	imull	%edx, %eax
	sall	$3, %eax
	movl	%eax, (%esp)
	call	malloc
	movl	%eax, 112(%esp)
	movl	76(%esp), %edx
	movl	76(%esp), %eax
	imull	%edx, %eax
	sall	$3, %eax
	movl	%eax, (%esp)
	call	malloc
	movl	%eax, 116(%esp)
	cmpl	$0, 108(%esp)
	je	.L5
	cmpl	$0, 112(%esp)
	je	.L5
	cmpl	$0, 116(%esp)
	jne	.L6
.L5:
	movl	stderr, %eax
	movl	%eax, %edx
	movl	$.LC3, %eax
	movl	%edx, 12(%esp)
	movl	$52, 8(%esp)
	movl	$1, 4(%esp)
	movl	%eax, (%esp)
	call	fwrite
	movl	$-1, %eax
	jmp	.L3
.L6:
	movl	$0, 84(%esp)
	jmp	.L7
.L10:
	movl	$0, 88(%esp)
	jmp	.L8
.L9:
	movl	84(%esp), %eax
	imull	76(%esp), %eax
	addl	88(%esp), %eax
	sall	$3, %eax
	movl	%eax, %ebx
	addl	108(%esp), %ebx
	call	rand
	movl	%eax, 44(%esp)
	fildl	44(%esp)
	fldl	.LC4
	fdivrp	%st, %st(1)
	fstpl	(%ebx)
	movl	84(%esp), %eax
	imull	76(%esp), %eax
	addl	88(%esp), %eax
	sall	$3, %eax
	movl	%eax, %ebx
	addl	112(%esp), %ebx
	call	rand
	movl	%eax, 44(%esp)
	fildl	44(%esp)
	fldl	.LC4
	fdivrp	%st, %st(1)
	fstpl	(%ebx)
	movl	84(%esp), %eax
	imull	76(%esp), %eax
	addl	88(%esp), %eax
	sall	$3, %eax
	addl	116(%esp), %eax
	fldz
	fstpl	(%eax)
	incl	88(%esp)
.L8:
	movl	88(%esp), %eax
	cmpl	76(%esp), %eax
	jl	.L9
	incl	84(%esp)
.L7:
	movl	84(%esp), %eax
	cmpl	76(%esp), %eax
	jl	.L10
	cmpl	$0, 100(%esp)
	je	.L11
	movl	76(%esp), %eax
	imull	76(%esp), %eax
	leal	0(,%eax,8), %edx
	movl	108(%esp), %eax
	andl	$-4096, %eax
	movl	$1, 8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	mprotect
	testl	%eax, %eax
	je	.L12
	call	__errno_location
	movl	(%eax), %ecx
	movl	$.LC5, %edx
	movl	stderr, %eax
	movl	%ecx, 8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	fprintf
	movl	$-2, %eax
	jmp	.L3
.L12:
	movl	76(%esp), %eax
	imull	76(%esp), %eax
	leal	0(,%eax,8), %edx
	movl	112(%esp), %eax
	andl	$-4096, %eax
	movl	$1, 8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	mprotect
	testl	%eax, %eax
	je	.L11
	call	__errno_location
	movl	(%eax), %ecx
	movl	$.LC6, %edx
	movl	stderr, %eax
	movl	%ecx, 8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	fprintf
	movl	$-2, %eax
	jmp	.L3
.L11:
	cmpl	$8, 76(%esp)
	jg	.L13
	movl	$.LC7, (%esp)
	call	puts
	movl	$0, 84(%esp)
	jmp	.L14
.L17:
	movl	$0, 88(%esp)
	jmp	.L15
.L16:
	movl	84(%esp), %eax
	imull	76(%esp), %eax
	addl	88(%esp), %eax
	sall	$3, %eax
	addl	108(%esp), %eax
	fldl	(%eax)
	movl	$.LC8, %eax
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	incl	88(%esp)
.L15:
	movl	88(%esp), %eax
	cmpl	76(%esp), %eax
	jl	.L16
	movl	$10, (%esp)
	call	putchar
	incl	84(%esp)
.L14:
	movl	84(%esp), %eax
	cmpl	76(%esp), %eax
	jl	.L17
	movl	$.LC9, (%esp)
	call	puts
	movl	$0, 84(%esp)
	jmp	.L18
.L21:
	movl	$0, 88(%esp)
	jmp	.L19
.L20:
	movl	84(%esp), %eax
	imull	76(%esp), %eax
	addl	88(%esp), %eax
	sall	$3, %eax
	addl	112(%esp), %eax
	fldl	(%eax)
	movl	$.LC8, %eax
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	incl	88(%esp)
.L19:
	movl	88(%esp), %eax
	cmpl	76(%esp), %eax
	jl	.L20
	movl	$10, (%esp)
	call	putchar
	incl	84(%esp)
.L18:
	movl	84(%esp), %eax
	cmpl	76(%esp), %eax
	jl	.L21
	movl	$.LC10, (%esp)
	call	puts
.L13:
	movl	$0, 4(%esp)
	leal	68(%esp), %eax
	movl	%eax, (%esp)
	call	gettimeofday
	movl	$0, 84(%esp)
	jmp	.L22
.L27:
	movl	$0, 88(%esp)
	jmp	.L23
.L26:
	movl	$0, 92(%esp)
	jmp	.L24
.L25:
	movl	84(%esp), %eax
	imull	76(%esp), %eax
	addl	88(%esp), %eax
	sall	$3, %eax
	addl	116(%esp), %eax
	movl	84(%esp), %edx
	imull	76(%esp), %edx
	addl	88(%esp), %edx
	sall	$3, %edx
	addl	116(%esp), %edx
	fldl	(%edx)
	movl	84(%esp), %edx
	imull	76(%esp), %edx
	addl	92(%esp), %edx
	sall	$3, %edx
	addl	108(%esp), %edx
	fldl	(%edx)
	movl	92(%esp), %edx
	imull	76(%esp), %edx
	addl	88(%esp), %edx
	sall	$3, %edx
	addl	112(%esp), %edx
	fldl	(%edx)
	fmulp	%st, %st(1)
	faddp	%st, %st(1)
	fstpl	(%eax)
	incl	92(%esp)
.L24:
	movl	92(%esp), %eax
	cmpl	76(%esp), %eax
	jl	.L25
	incl	88(%esp)
.L23:
	movl	88(%esp), %eax
	cmpl	76(%esp), %eax
	jl	.L26
	incl	84(%esp)
.L22:
	movl	84(%esp), %eax
	cmpl	76(%esp), %eax
	jl	.L27
	movl	$0, 4(%esp)
	leal	60(%esp), %eax
	movl	%eax, (%esp)
	call	gettimeofday
	movl	64(%esp), %edx
	movl	72(%esp), %eax
	movl	%edx, %ecx
	subl	%eax, %ecx
	movl	%ecx, %eax
	movl	%eax, 44(%esp)
	fildl	44(%esp)
	fldl	.LC11
	fdivrp	%st, %st(1)
	movl	60(%esp), %edx
	movl	68(%esp), %eax
	movl	%edx, %ecx
	subl	%eax, %ecx
	movl	%ecx, %eax
	movl	%eax, 44(%esp)
	fildl	44(%esp)
	faddp	%st, %st(1)
	fstpl	128(%esp)
	movl	$.LC12, %eax
	fldl	128(%esp)
	fstpl	8(%esp)
	movl	96(%esp), %edx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	fldl	136(%esp)
	faddl	128(%esp)
	fstpl	136(%esp)
	cmpl	$8, 76(%esp)
	jg	.L28
	movl	$.LC13, (%esp)
	call	puts
	movl	$0, 84(%esp)
	jmp	.L29
.L32:
	movl	$0, 88(%esp)
	jmp	.L30
.L31:
	movl	84(%esp), %eax
	imull	76(%esp), %eax
	addl	88(%esp), %eax
	sall	$3, %eax
	addl	116(%esp), %eax
	fldl	(%eax)
	movl	$.LC8, %eax
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	incl	88(%esp)
.L30:
	movl	88(%esp), %eax
	cmpl	76(%esp), %eax
	jl	.L31
	movl	$10, (%esp)
	call	putchar
	incl	84(%esp)
.L29:
	movl	84(%esp), %eax
	cmpl	76(%esp), %eax
	jl	.L32
.L28:
	cmpl	$0, 100(%esp)
	je	.L33
	movl	76(%esp), %eax
	imull	76(%esp), %eax
	leal	0(,%eax,8), %edx
	movl	108(%esp), %eax
	andl	$-4096, %eax
	movl	$2, 8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	mprotect
	testl	%eax, %eax
	je	.L34
	call	__errno_location
	movl	(%eax), %ecx
	movl	$.LC14, %edx
	movl	stderr, %eax
	movl	%ecx, 8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	fprintf
	movl	$-3, %eax
	jmp	.L3
.L34:
	movl	76(%esp), %eax
	imull	76(%esp), %eax
	leal	0(,%eax,8), %edx
	movl	112(%esp), %eax
	andl	$-4096, %eax
	movl	$2, 8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	mprotect
	testl	%eax, %eax
	je	.L33
	call	__errno_location
	movl	(%eax), %ecx
	movl	$.LC15, %edx
	movl	stderr, %eax
	movl	%ecx, 8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	fprintf
	movl	$-3, %eax
	jmp	.L3
.L33:
	movl	108(%esp), %eax
	movl	%eax, (%esp)
	call	free
	movl	112(%esp), %eax
	movl	%eax, (%esp)
	call	free
	movl	116(%esp), %eax
	movl	%eax, (%esp)
	call	free
	incl	96(%esp)
.L4:
	movl	96(%esp), %eax
	cmpl	80(%esp), %eax
	jl	.L35
	fildl	80(%esp)
	fldl	136(%esp)
	fdivp	%st, %st(1)
	movl	$.LC16, %eax
	fstpl	16(%esp)
	fldl	136(%esp)
	fstpl	8(%esp)
	movl	80(%esp), %edx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$0, %eax
.L3:
	addl	$156, %esp
	popl	%ebx
	leave
	ret
	.size	main, .-main
	.section	.rodata
	.align 8
.LC4:
	.long	4290772992
	.long	1105199103
	.align 8
.LC11:
	.long	0
	.long	1093567616
	.ident	"GCC: (GNU) 4.4.7 20120313 (Red Hat 4.4.7-17)"
	.section	.note.GNU-stack,"",@progbits
