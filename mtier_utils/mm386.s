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
	leal	4(%esp), %ecx
	andl	$-16, %esp
	pushl	-4(%ecx)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%ecx
	subl	$112, %esp
	movl	%ecx, %ebx
	movl	$1, -52(%ebp)
	movl	$0, -48(%ebp)
	movl	$0, %eax
	movl	$0, %edx
	movl	%eax, -16(%ebp)
	movl	%edx, -12(%ebp)
	cmpl	$5, (%ebx)
	je	.L2
	movl	4(%ebx), %eax
	movl	(%eax), %edx
	movl	$.LC1, %eax
	subl	$8, %esp
	pushl	%edx
	pushl	%eax
	call	printf
	addl	$16, %esp
	movl	$0, %eax
	jmp	.L3
.L2:
	movl	4(%ebx), %eax
	addl	$4, %eax
	movl	(%eax), %eax
	subl	$12, %esp
	pushl	%eax
	call	atoi
	addl	$16, %esp
	movl	%eax, -76(%ebp)
	movl	4(%ebx), %eax
	addl	$8, %eax
	movl	(%eax), %eax
	subl	$12, %esp
	pushl	%eax
	call	atoi
	addl	$16, %esp
	movl	%eax, -72(%ebp)
	movl	4(%ebx), %eax
	addl	$12, %eax
	movl	(%eax), %eax
	subl	$12, %esp
	pushl	%eax
	call	atoi
	addl	$16, %esp
	movl	%eax, -52(%ebp)
	movl	4(%ebx), %eax
	addl	$16, %eax
	movl	(%eax), %eax
	subl	$12, %esp
	pushl	%eax
	call	atoi
	addl	$16, %esp
	movl	%eax, -48(%ebp)
	movl	$.LC2, %eax
	pushl	-72(%ebp)
	pushl	-76(%ebp)
	pushl	-76(%ebp)
	pushl	%eax
	call	printf
	addl	$16, %esp
	movl	$0, -56(%ebp)
	jmp	.L4
.L35:
	movl	-48(%ebp), %eax
	subl	$12, %esp
	pushl	%eax
	call	srand
	addl	$16, %esp
	movl	-76(%ebp), %edx
	movl	-76(%ebp), %eax
	imull	%edx, %eax
	sall	$3, %eax
	subl	$12, %esp
	pushl	%eax
	call	malloc
	addl	$16, %esp
	movl	%eax, -44(%ebp)
	movl	-76(%ebp), %edx
	movl	-76(%ebp), %eax
	imull	%edx, %eax
	sall	$3, %eax
	subl	$12, %esp
	pushl	%eax
	call	malloc
	addl	$16, %esp
	movl	%eax, -40(%ebp)
	movl	-76(%ebp), %edx
	movl	-76(%ebp), %eax
	imull	%edx, %eax
	sall	$3, %eax
	subl	$12, %esp
	pushl	%eax
	call	malloc
	addl	$16, %esp
	movl	%eax, -36(%ebp)
	cmpl	$0, -44(%ebp)
	je	.L5
	cmpl	$0, -40(%ebp)
	je	.L5
	cmpl	$0, -36(%ebp)
	jne	.L6
.L5:
	movl	stderr, %eax
	movl	%eax, %edx
	movl	$.LC3, %eax
	pushl	%edx
	pushl	$52
	pushl	$1
	pushl	%eax
	call	fwrite
	addl	$16, %esp
	movl	$-1, %eax
	jmp	.L3
.L6:
	movl	$0, -68(%ebp)
	jmp	.L7
.L10:
	movl	$0, -64(%ebp)
	jmp	.L8
.L9:
	movl	-68(%ebp), %eax
	imull	-76(%ebp), %eax
	addl	-64(%ebp), %eax
	sall	$3, %eax
	movl	%eax, %ebx
	addl	-44(%ebp), %ebx
	call	rand
	movl	%eax, -108(%ebp)
	fildl	-108(%ebp)
	fldl	.LC4
	fdivrp	%st, %st(1)
	fstpl	(%ebx)
	movl	-68(%ebp), %eax
	imull	-76(%ebp), %eax
	addl	-64(%ebp), %eax
	sall	$3, %eax
	movl	%eax, %ebx
	addl	-40(%ebp), %ebx
	call	rand
	movl	%eax, -108(%ebp)
	fildl	-108(%ebp)
	fldl	.LC4
	fdivrp	%st, %st(1)
	fstpl	(%ebx)
	movl	-68(%ebp), %eax
	imull	-76(%ebp), %eax
	addl	-64(%ebp), %eax
	sall	$3, %eax
	movl	%eax, %ecx
	addl	-36(%ebp), %ecx
	movl	$0, %eax
	movl	$0, %edx
	movl	%eax, (%ecx)
	movl	%edx, 4(%ecx)
	incl	-64(%ebp)
.L8:
	movl	-64(%ebp), %eax
	cmpl	-76(%ebp), %eax
	jl	.L9
	incl	-68(%ebp)
.L7:
	movl	-68(%ebp), %eax
	cmpl	-76(%ebp), %eax
	jl	.L10
	cmpl	$0, -52(%ebp)
	je	.L11
	movl	-76(%ebp), %eax
	imull	-76(%ebp), %eax
	leal	0(,%eax,8), %edx
	movl	-44(%ebp), %eax
	andl	$-4096, %eax
	subl	$4, %esp
	pushl	$1
	pushl	%edx
	pushl	%eax
	call	mprotect
	addl	$16, %esp
	testl	%eax, %eax
	je	.L12
	call	__errno_location
	movl	(%eax), %ecx
	movl	$.LC5, %edx
	movl	stderr, %eax
	subl	$4, %esp
	pushl	%ecx
	pushl	%edx
	pushl	%eax
	call	fprintf
	addl	$16, %esp
	movl	$-2, %eax
	jmp	.L3
.L12:
	movl	-76(%ebp), %eax
	imull	-76(%ebp), %eax
	leal	0(,%eax,8), %edx
	movl	-40(%ebp), %eax
	andl	$-4096, %eax
	subl	$4, %esp
	pushl	$1
	pushl	%edx
	pushl	%eax
	call	mprotect
	addl	$16, %esp
	testl	%eax, %eax
	je	.L11
	call	__errno_location
	movl	(%eax), %ecx
	movl	$.LC6, %edx
	movl	stderr, %eax
	subl	$4, %esp
	pushl	%ecx
	pushl	%edx
	pushl	%eax
	call	fprintf
	addl	$16, %esp
	movl	$-2, %eax
	jmp	.L3
.L11:
	cmpl	$8, -76(%ebp)
	jg	.L13
	subl	$12, %esp
	pushl	$.LC7
	call	puts
	addl	$16, %esp
	movl	$0, -68(%ebp)
	jmp	.L14
.L17:
	movl	$0, -64(%ebp)
	jmp	.L15
.L16:
	movl	-68(%ebp), %eax
	imull	-76(%ebp), %eax
	addl	-64(%ebp), %eax
	sall	$3, %eax
	addl	-44(%ebp), %eax
	movl	4(%eax), %edx
	movl	(%eax), %eax
	movl	$.LC8, %ecx
	subl	$4, %esp
	pushl	%edx
	pushl	%eax
	pushl	%ecx
	call	printf
	addl	$16, %esp
	incl	-64(%ebp)
.L15:
	movl	-64(%ebp), %eax
	cmpl	-76(%ebp), %eax
	jl	.L16
	subl	$12, %esp
	pushl	$10
	call	putchar
	addl	$16, %esp
	incl	-68(%ebp)
.L14:
	movl	-68(%ebp), %eax
	cmpl	-76(%ebp), %eax
	jl	.L17
	subl	$12, %esp
	pushl	$.LC9
	call	puts
	addl	$16, %esp
	movl	$0, -68(%ebp)
	jmp	.L18
.L21:
	movl	$0, -64(%ebp)
	jmp	.L19
.L20:
	movl	-68(%ebp), %eax
	imull	-76(%ebp), %eax
	addl	-64(%ebp), %eax
	sall	$3, %eax
	addl	-40(%ebp), %eax
	movl	4(%eax), %edx
	movl	(%eax), %eax
	movl	$.LC8, %ecx
	subl	$4, %esp
	pushl	%edx
	pushl	%eax
	pushl	%ecx
	call	printf
	addl	$16, %esp
	incl	-64(%ebp)
.L19:
	movl	-64(%ebp), %eax
	cmpl	-76(%ebp), %eax
	jl	.L20
	subl	$12, %esp
	pushl	$10
	call	putchar
	addl	$16, %esp
	incl	-68(%ebp)
.L18:
	movl	-68(%ebp), %eax
	cmpl	-76(%ebp), %eax
	jl	.L21
	subl	$12, %esp
	pushl	$.LC10
	call	puts
	addl	$16, %esp
.L13:
	subl	$8, %esp
	pushl	$0
	leal	-84(%ebp), %eax
	pushl	%eax
	call	gettimeofday
	addl	$16, %esp
	movl	$0, -68(%ebp)
	jmp	.L22
.L27:
	movl	$0, -64(%ebp)
	jmp	.L23
.L26:
	movl	$0, -60(%ebp)
	jmp	.L24
.L25:
	movl	-68(%ebp), %eax
	imull	-76(%ebp), %eax
	addl	-64(%ebp), %eax
	sall	$3, %eax
	addl	-36(%ebp), %eax
	movl	-68(%ebp), %edx
	imull	-76(%ebp), %edx
	addl	-64(%ebp), %edx
	sall	$3, %edx
	addl	-36(%ebp), %edx
	fldl	(%edx)
	movl	-68(%ebp), %edx
	imull	-76(%ebp), %edx
	addl	-60(%ebp), %edx
	sall	$3, %edx
	addl	-44(%ebp), %edx
	fldl	(%edx)
	movl	-60(%ebp), %edx
	imull	-76(%ebp), %edx
	addl	-64(%ebp), %edx
	sall	$3, %edx
	addl	-40(%ebp), %edx
	fldl	(%edx)
	fmulp	%st, %st(1)
	faddp	%st, %st(1)
	fstpl	(%eax)
	incl	-60(%ebp)
.L24:
	movl	-60(%ebp), %eax
	cmpl	-76(%ebp), %eax
	jl	.L25
	incl	-64(%ebp)
.L23:
	movl	-64(%ebp), %eax
	cmpl	-76(%ebp), %eax
	jl	.L26
	incl	-68(%ebp)
.L22:
	movl	-68(%ebp), %eax
	cmpl	-76(%ebp), %eax
	jl	.L27
	subl	$8, %esp
	pushl	$0
	leal	-92(%ebp), %eax
	pushl	%eax
	call	gettimeofday
	addl	$16, %esp
	movl	-88(%ebp), %edx
	movl	-80(%ebp), %eax
	movl	%edx, %ecx
	subl	%eax, %ecx
	movl	%ecx, %eax
	movl	%eax, -108(%ebp)
	fildl	-108(%ebp)
	fldl	.LC11
	fdivrp	%st, %st(1)
	movl	-92(%ebp), %edx
	movl	-84(%ebp), %eax
	movl	%edx, %ecx
	subl	%eax, %ecx
	movl	%ecx, %eax
	movl	%eax, -108(%ebp)
	fildl	-108(%ebp)
	faddp	%st, %st(1)
	fstpl	-24(%ebp)
	movl	$.LC12, %eax
	pushl	-20(%ebp)
	pushl	-24(%ebp)
	pushl	-56(%ebp)
	pushl	%eax
	call	printf
	addl	$16, %esp
	fldl	-16(%ebp)
	faddl	-24(%ebp)
	fstpl	-16(%ebp)
	cmpl	$8, -76(%ebp)
	jg	.L28
	subl	$12, %esp
	pushl	$.LC13
	call	puts
	addl	$16, %esp
	movl	$0, -68(%ebp)
	jmp	.L29
.L32:
	movl	$0, -64(%ebp)
	jmp	.L30
.L31:
	movl	-68(%ebp), %eax
	imull	-76(%ebp), %eax
	addl	-64(%ebp), %eax
	sall	$3, %eax
	addl	-36(%ebp), %eax
	movl	4(%eax), %edx
	movl	(%eax), %eax
	movl	$.LC8, %ecx
	subl	$4, %esp
	pushl	%edx
	pushl	%eax
	pushl	%ecx
	call	printf
	addl	$16, %esp
	incl	-64(%ebp)
.L30:
	movl	-64(%ebp), %eax
	cmpl	-76(%ebp), %eax
	jl	.L31
	subl	$12, %esp
	pushl	$10
	call	putchar
	addl	$16, %esp
	incl	-68(%ebp)
.L29:
	movl	-68(%ebp), %eax
	cmpl	-76(%ebp), %eax
	jl	.L32
.L28:
	cmpl	$0, -52(%ebp)
	je	.L33
	movl	-76(%ebp), %eax
	imull	-76(%ebp), %eax
	leal	0(,%eax,8), %edx
	movl	-44(%ebp), %eax
	andl	$-4096, %eax
	subl	$4, %esp
	pushl	$2
	pushl	%edx
	pushl	%eax
	call	mprotect
	addl	$16, %esp
	testl	%eax, %eax
	je	.L34
	call	__errno_location
	movl	(%eax), %ecx
	movl	$.LC14, %edx
	movl	stderr, %eax
	subl	$4, %esp
	pushl	%ecx
	pushl	%edx
	pushl	%eax
	call	fprintf
	addl	$16, %esp
	movl	$-3, %eax
	jmp	.L3
.L34:
	movl	-76(%ebp), %eax
	imull	-76(%ebp), %eax
	leal	0(,%eax,8), %edx
	movl	-40(%ebp), %eax
	andl	$-4096, %eax
	subl	$4, %esp
	pushl	$2
	pushl	%edx
	pushl	%eax
	call	mprotect
	addl	$16, %esp
	testl	%eax, %eax
	je	.L33
	call	__errno_location
	movl	(%eax), %ecx
	movl	$.LC15, %edx
	movl	stderr, %eax
	subl	$4, %esp
	pushl	%ecx
	pushl	%edx
	pushl	%eax
	call	fprintf
	addl	$16, %esp
	movl	$-3, %eax
	jmp	.L3
.L33:
	subl	$12, %esp
	pushl	-44(%ebp)
	call	free
	addl	$16, %esp
	subl	$12, %esp
	pushl	-40(%ebp)
	call	free
	addl	$16, %esp
	subl	$12, %esp
	pushl	-36(%ebp)
	call	free
	addl	$16, %esp
	incl	-56(%ebp)
.L4:
	movl	-56(%ebp), %eax
	cmpl	-72(%ebp), %eax
	jl	.L35
	fildl	-72(%ebp)
	fldl	-16(%ebp)
	fdivp	%st, %st(1)
	movl	$.LC16, %eax
	subl	$8, %esp
	leal	-8(%esp), %esp
	fstpl	(%esp)
	pushl	-12(%ebp)
	pushl	-16(%ebp)
	pushl	-72(%ebp)
	pushl	%eax
	call	printf
	addl	$32, %esp
	movl	$0, %eax
.L3:
	leal	-8(%ebp), %esp
	addl	$0, %esp
	popl	%ecx
	popl	%ebx
	leave
	leal	-4(%ecx), %esp
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
