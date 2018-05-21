	.file	"stream.c"
	.comm	a,4,4
	.comm	b,4,4
	.comm	c,4,4
	.local	avgtime
	.comm	avgtime,32,32
	.local	maxtime
	.comm	maxtime,32,32
	.data
	.align 32
	.type	mintime, @object
	.size	mintime, 32
mintime:
	.long	3758096384
	.long	1206910975
	.long	3758096384
	.long	1206910975
	.long	3758096384
	.long	1206910975
	.long	3758096384
	.long	1206910975
	.section	.rodata
.LC0:
	.string	"Copy:      "
.LC1:
	.string	"Scale:     "
.LC2:
	.string	"Add:       "
.LC3:
	.string	"Triad:     "
	.data
	.align 4
	.type	label, @object
	.size	label, 16
label:
	.long	.LC0
	.long	.LC1
	.long	.LC2
	.long	.LC3
	.align 32
	.type	bytes, @object
	.size	bytes, 32
bytes:
	.long	0
	.long	1104053376
	.long	0
	.long	1104053376
	.long	0
	.long	1104601952
	.long	0
	.long	1104601952
	.section	.rodata
.LC4:
	.string	"malloc: "
	.align 4
.LC5:
	.string	"mtier: problem allocating array *sad trombone *"
	.align 4
.LC6:
	.string	"done. a=0x%lx b=0x%lx c=0x%lx\n"
	.align 4
.LC7:
	.string	"-------------------------------------------------------------"
	.align 4
.LC8:
	.string	"STREAM version $Revision: 5.10 $"
	.align 4
.LC9:
	.string	"This system uses %d bytes per array element.\n"
	.align 4
.LC10:
	.string	"Array size = %llu (elements), Offset = %d (elements)\n"
	.align 4
.LC13:
	.string	"Memory per array = %.1f MiB (= %.1f GiB).\n"
	.align 4
.LC15:
	.string	"Total memory required = %.1f MiB (= %.1f GiB).\n"
	.align 4
.LC16:
	.string	"Each kernel will be executed %d times.\n"
	.align 4
.LC17:
	.string	" The *best* time for each kernel (excluding the first iteration)"
	.align 4
.LC18:
	.string	" will be used to compute the reported bandwidth."
	.align 4
.LC22:
	.string	"Your clock granularity/precision appears to be %d microseconds.\n"
	.align 4
.LC23:
	.string	"Your clock granularity appears to be less than one microsecond."
	.align 4
.LC25:
	.string	"Each test below will take on the order of %d microseconds.\n"
.LC26:
	.string	"   (= %d clock ticks)\n"
	.align 4
.LC27:
	.string	"Increase the size of the arrays if this shows that"
	.align 4
.LC28:
	.string	"you are not getting at least 20 clock ticks per test."
	.align 4
.LC29:
	.string	"WARNING -- The above is only a rough guideline."
	.align 4
.LC30:
	.string	"For best results, please be sure you know the"
	.align 4
.LC31:
	.string	"precision of your system timer."
	.align 4
.LC32:
	.string	"mtier: error protecting memory (%d)\n"
	.align 4
.LC33:
	.string	"Function    Best Rate MB/s  Avg time     Min time     Max time"
	.align 4
.LC36:
	.string	"%s%12.1f  %11.6f  %11.6f  %11.6f\n"
	.text
.globl main
	.type	main, @function
main:
	pushl	%ebp
	movl	%esp, %ebp
	andl	$-16, %esp
	pushl	%ebx
	subl	$444, %esp
	movl	$.LC4, %eax
	movl	%eax, (%esp)
	call	printf
	movl	$512000000, (%esp)
	call	valloc
	movl	%eax, a
	movl	$512000000, (%esp)
	call	valloc
	movl	%eax, b
	movl	$512000000, (%esp)
	call	valloc
	movl	%eax, c
	movl	a, %eax
	testl	%eax, %eax
	je	.L2
	movl	b, %eax
	testl	%eax, %eax
	je	.L2
	movl	c, %eax
	testl	%eax, %eax
	jne	.L3
.L2:
	movl	$.LC5, (%esp)
	call	puts
.L3:
	movl	c, %eax
	movl	%eax, %ebx
	movl	b, %eax
	movl	%eax, %ecx
	movl	a, %eax
	movl	%eax, %edx
	movl	$.LC6, %eax
	movl	%ebx, 12(%esp)
	movl	%ecx, 8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$.LC7, (%esp)
	call	puts
	movl	$.LC8, (%esp)
	call	puts
	movl	$.LC7, (%esp)
	call	puts
	movl	$8, 396(%esp)
	movl	$.LC9, %eax
	movl	396(%esp), %edx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$.LC7, (%esp)
	call	puts
	movl	$.LC10, %eax
	movl	$0, 12(%esp)
	movl	$64000000, 4(%esp)
	movl	$0, 8(%esp)
	movl	%eax, (%esp)
	call	printf
	fildl	396(%esp)
	fldl	.LC11
	fmulp	%st, %st(1)
	fildl	396(%esp)
	fldl	.LC12
	fmulp	%st, %st(1)
	fxch	%st(1)
	movl	$.LC13, %eax
	fstpl	12(%esp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	fildl	396(%esp)
	fldl	.LC14
	fmulp	%st, %st(1)
	fldl	.LC11
	fmulp	%st, %st(1)
	fildl	396(%esp)
	fldl	.LC14
	fmulp	%st, %st(1)
	fldl	.LC12
	fmulp	%st, %st(1)
	fxch	%st(1)
	movl	$.LC15, %eax
	fstpl	12(%esp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$.LC16, %eax
	movl	$10, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$.LC17, (%esp)
	call	puts
	movl	$.LC18, (%esp)
	call	puts
	movl	$0, 412(%esp)
	jmp	.L4
.L5:
	movl	a, %eax
	movl	412(%esp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fld1
	fstpl	(%eax)
	movl	b, %eax
	movl	412(%esp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	.LC20
	fstpl	(%eax)
	movl	c, %eax
	movl	412(%esp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldz
	fstpl	(%eax)
	incl	412(%esp)
.L4:
	cmpl	$63999999, 412(%esp)
	jle	.L5
	movl	$.LC7, (%esp)
	call	puts
	call	checktick
	movl	%eax, 392(%esp)
	cmpl	$0, 392(%esp)
	jle	.L6
	movl	$.LC22, %eax
	movl	392(%esp), %edx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	jmp	.L7
.L6:
	movl	$.LC23, (%esp)
	call	puts
	movl	$1, 392(%esp)
.L7:
	call	mysecond
	fstpl	424(%esp)
	movl	$0, 412(%esp)
	jmp	.L8
.L9:
	movl	a, %eax
	movl	412(%esp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	movl	a, %edx
	movl	412(%esp), %ecx
	sall	$3, %ecx
	addl	%ecx, %edx
	fldl	(%edx)
	fadd	%st(0), %st
	fstpl	(%eax)
	incl	412(%esp)
.L8:
	cmpl	$63999999, 412(%esp)
	jle	.L9
	call	mysecond
	fsubl	424(%esp)
	fldl	.LC24
	fmulp	%st, %st(1)
	fstpl	424(%esp)
	fldl	424(%esp)
	fisttpl	60(%esp)
	movl	60(%esp), %edx
	movl	$.LC25, %eax
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	fildl	392(%esp)
	fldl	424(%esp)
	fdivp	%st, %st(1)
	fisttpl	60(%esp)
	movl	60(%esp), %edx
	movl	$.LC26, %eax
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$.LC27, (%esp)
	call	puts
	movl	$.LC28, (%esp)
	call	puts
	movl	$.LC7, (%esp)
	call	puts
	movl	$.LC29, (%esp)
	call	puts
	movl	$.LC30, (%esp)
	call	puts
	movl	$.LC31, (%esp)
	call	puts
	movl	$.LC7, (%esp)
	call	puts
	movl	a, %eax
	movl	$1, 8(%esp)
	movl	$512000000, 4(%esp)
	movl	%eax, (%esp)
	call	mprotect
	movl	%eax, 404(%esp)
	movl	b, %eax
	movl	$1, 8(%esp)
	movl	$512000000, 4(%esp)
	movl	%eax, (%esp)
	call	mprotect
	movl	%eax, 408(%esp)
	cmpl	$0, 404(%esp)
	jne	.L10
	cmpl	$0, 408(%esp)
	je	.L11
.L10:
	call	__errno_location
	movl	(%eax), %edx
	movl	$.LC32, %eax
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$-1, %eax
	jmp	.L12
.L11:
	fldl	.LC14
	fstpl	416(%esp)
	movl	$0, 400(%esp)
	jmp	.L13
.L22:
	movl	400(%esp), %ebx
	call	mysecond
	fstpl	72(%esp,%ebx,8)
	movl	$0, 412(%esp)
	jmp	.L14
.L15:
	movl	c, %eax
	movl	412(%esp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	movl	a, %edx
	movl	412(%esp), %ecx
	sall	$3, %ecx
	addl	%ecx, %edx
	fldl	(%edx)
	fstpl	(%eax)
	incl	412(%esp)
.L14:
	cmpl	$63999999, 412(%esp)
	jle	.L15
	movl	400(%esp), %ebx
	call	mysecond
	movl	400(%esp), %eax
	fldl	72(%esp,%eax,8)
	fsubrp	%st, %st(1)
	fstpl	72(%esp,%ebx,8)
	movl	400(%esp), %ebx
	call	mysecond
	leal	10(%ebx), %eax
	fstpl	72(%esp,%eax,8)
	movl	$0, 412(%esp)
	jmp	.L16
.L17:
	movl	c, %eax
	movl	412(%esp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	movl	b, %edx
	movl	412(%esp), %ecx
	sall	$3, %ecx
	addl	%ecx, %edx
	fldl	(%edx)
	fmull	416(%esp)
	fstpl	(%eax)
	incl	412(%esp)
.L16:
	cmpl	$63999999, 412(%esp)
	jle	.L17
	movl	400(%esp), %ebx
	call	mysecond
	movl	400(%esp), %eax
	addl	$10, %eax
	fldl	72(%esp,%eax,8)
	fsubrp	%st, %st(1)
	leal	10(%ebx), %eax
	fstpl	72(%esp,%eax,8)
	movl	400(%esp), %ebx
	call	mysecond
	leal	20(%ebx), %eax
	fstpl	72(%esp,%eax,8)
	movl	$0, 412(%esp)
	jmp	.L18
.L19:
	movl	c, %eax
	movl	412(%esp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	movl	a, %edx
	movl	412(%esp), %ecx
	sall	$3, %ecx
	addl	%ecx, %edx
	fldl	(%edx)
	movl	b, %edx
	movl	412(%esp), %ecx
	sall	$3, %ecx
	addl	%ecx, %edx
	fldl	(%edx)
	faddp	%st, %st(1)
	fstpl	(%eax)
	incl	412(%esp)
.L18:
	cmpl	$63999999, 412(%esp)
	jle	.L19
	movl	400(%esp), %ebx
	call	mysecond
	movl	400(%esp), %eax
	addl	$20, %eax
	fldl	72(%esp,%eax,8)
	fsubrp	%st, %st(1)
	leal	20(%ebx), %eax
	fstpl	72(%esp,%eax,8)
	movl	400(%esp), %ebx
	call	mysecond
	leal	30(%ebx), %eax
	fstpl	72(%esp,%eax,8)
	movl	$0, 412(%esp)
	jmp	.L20
.L21:
	movl	c, %eax
	movl	412(%esp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	movl	b, %edx
	movl	412(%esp), %ecx
	sall	$3, %ecx
	addl	%ecx, %edx
	fldl	(%edx)
	movl	a, %edx
	movl	412(%esp), %ecx
	sall	$3, %ecx
	addl	%ecx, %edx
	fldl	(%edx)
	fmull	416(%esp)
	faddp	%st, %st(1)
	fstpl	(%eax)
	incl	412(%esp)
.L20:
	cmpl	$63999999, 412(%esp)
	jle	.L21
	movl	400(%esp), %ebx
	call	mysecond
	movl	400(%esp), %eax
	addl	$30, %eax
	fldl	72(%esp,%eax,8)
	fsubrp	%st, %st(1)
	leal	30(%ebx), %eax
	fstpl	72(%esp,%eax,8)
	incl	400(%esp)
.L13:
	cmpl	$9, 400(%esp)
	jle	.L22
	movl	$1, 400(%esp)
	jmp	.L23
.L30:
	movl	$0, 412(%esp)
	jmp	.L24
.L29:
	movl	412(%esp), %ecx
	movl	412(%esp), %eax
	fldl	avgtime(,%eax,8)
	movl	412(%esp), %edx
	movl	400(%esp), %ebx
	movl	%edx, %eax
	sall	$2, %eax
	addl	%edx, %eax
	addl	%eax, %eax
	addl	%ebx, %eax
	fldl	72(%esp,%eax,8)
	faddp	%st, %st(1)
	fstpl	avgtime(,%ecx,8)
	movl	412(%esp), %ecx
	movl	412(%esp), %eax
	fldl	mintime(,%eax,8)
	movl	412(%esp), %edx
	movl	400(%esp), %ebx
	movl	%edx, %eax
	sall	$2, %eax
	addl	%edx, %eax
	addl	%eax, %eax
	addl	%ebx, %eax
	fldl	72(%esp,%eax,8)
	fucomip	%st(1), %st
	fstp	%st(0)
	seta	%al
	testb	%al, %al
	je	.L25
	movl	412(%esp), %eax
	fldl	mintime(,%eax,8)
	jmp	.L26
.L25:
	movl	412(%esp), %edx
	movl	400(%esp), %ebx
	movl	%edx, %eax
	sall	$2, %eax
	addl	%edx, %eax
	addl	%eax, %eax
	addl	%ebx, %eax
	fldl	72(%esp,%eax,8)
.L26:
	fstpl	mintime(,%ecx,8)
	movl	412(%esp), %ecx
	movl	412(%esp), %eax
	fldl	maxtime(,%eax,8)
	movl	412(%esp), %edx
	movl	400(%esp), %ebx
	movl	%edx, %eax
	sall	$2, %eax
	addl	%edx, %eax
	addl	%eax, %eax
	addl	%ebx, %eax
	fldl	72(%esp,%eax,8)
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	seta	%al
	testb	%al, %al
	je	.L27
	movl	412(%esp), %eax
	fldl	maxtime(,%eax,8)
	jmp	.L28
.L27:
	movl	412(%esp), %edx
	movl	400(%esp), %ebx
	movl	%edx, %eax
	sall	$2, %eax
	addl	%edx, %eax
	addl	%eax, %eax
	addl	%ebx, %eax
	fldl	72(%esp,%eax,8)
.L28:
	fstpl	maxtime(,%ecx,8)
	incl	412(%esp)
.L24:
	cmpl	$3, 412(%esp)
	jle	.L29
	incl	400(%esp)
.L23:
	cmpl	$9, 400(%esp)
	jle	.L30
	movl	$.LC33, (%esp)
	call	puts
	movl	$0, 412(%esp)
	jmp	.L31
.L32:
	movl	412(%esp), %eax
	movl	412(%esp), %edx
	fldl	avgtime(,%edx,8)
	fldl	.LC34
	fdivrp	%st, %st(1)
	fstpl	avgtime(,%eax,8)
	movl	412(%esp), %eax
	fldl	maxtime(,%eax,8)
	movl	412(%esp), %eax
	fldl	mintime(,%eax,8)
	movl	412(%esp), %eax
	fldl	avgtime(,%eax,8)
	movl	412(%esp), %eax
	fldl	bytes(,%eax,8)
	fldl	.LC35
	fmulp	%st, %st(1)
	movl	412(%esp), %eax
	fldl	mintime(,%eax,8)
	fdivrp	%st, %st(1)
	fxch	%st(3)
	movl	412(%esp), %eax
	movl	label(,%eax,4), %edx
	movl	$.LC36, %eax
	fstpl	32(%esp)
	fxch	%st(1)
	fstpl	24(%esp)
	fstpl	16(%esp)
	fstpl	8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	incl	412(%esp)
.L31:
	cmpl	$3, 412(%esp)
	jle	.L32
	movl	$.LC7, (%esp)
	call	puts
	call	checkSTREAMresults
	movl	$.LC7, (%esp)
	call	puts
	movl	$0, %eax
.L12:
	addl	$444, %esp
	popl	%ebx
	leave
	ret
	.size	main, .-main
.globl checktick
	.type	checktick, @function
checktick:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$200, %esp
	movl	$0, -36(%ebp)
	jmp	.L35
.L37:
	call	mysecond
	fstpl	-24(%ebp)
.L36:
	call	mysecond
	fstpl	-16(%ebp)
	fldl	-16(%ebp)
	fsubl	-24(%ebp)
	fldl	.LC35
	fucomip	%st(1), %st
	fstp	%st(0)
	seta	%al
	testb	%al, %al
	jne	.L36
	movl	-36(%ebp), %eax
	fldl	-16(%ebp)
	fstpl	-24(%ebp)
	fldl	-24(%ebp)
	fstpl	-200(%ebp,%eax,8)
	incl	-36(%ebp)
.L35:
	cmpl	$19, -36(%ebp)
	jle	.L37
	movl	$1000000, -32(%ebp)
	movl	$1, -36(%ebp)
	jmp	.L38
.L39:
	movl	-36(%ebp), %eax
	fldl	-200(%ebp,%eax,8)
	movl	-36(%ebp), %eax
	decl	%eax
	fldl	-200(%ebp,%eax,8)
	fsubrp	%st, %st(1)
	fldl	.LC24
	fmulp	%st, %st(1)
	fisttpl	-28(%ebp)
	movl	$0, %eax
	cmpl	$0, -28(%ebp)
	movl	%eax, %edx
	cmovns	-28(%ebp), %edx
	movl	-32(%ebp), %eax
	cmpl	%eax, %edx
	cmovle	%edx, %eax
	movl	%eax, -32(%ebp)
	incl	-36(%ebp)
.L38:
	cmpl	$19, -36(%ebp)
	jle	.L39
	movl	-32(%ebp), %eax
	leave
	ret
	.size	checktick, .-checktick
.globl mysecond
	.type	mysecond, @function
mysecond:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$72, %esp
	leal	-28(%ebp), %eax
	movl	%eax, 4(%esp)
	leal	-20(%ebp), %eax
	movl	%eax, (%esp)
	call	gettimeofday
	movl	%eax, -12(%ebp)
	movl	-20(%ebp), %eax
	movl	%eax, -44(%ebp)
	fildl	-44(%ebp)
	movl	-16(%ebp), %eax
	movl	%eax, -44(%ebp)
	fildl	-44(%ebp)
	fldl	.LC35
	fmulp	%st, %st(1)
	faddp	%st, %st(1)
	leave
	ret
	.size	mysecond, .-mysecond
	.section	.rodata
	.align 4
.LC40:
	.string	"Failed Validation on array a[], AvgRelAbsErr > epsilon (%e)\n"
	.align 4
.LC41:
	.string	"     Expected Value: %e, AvgAbsErr: %e, AvgRelAbsErr: %e\n"
	.align 4
.LC42:
	.string	"     For array a[], %d errors were found.\n"
	.align 4
.LC43:
	.string	"Failed Validation on array b[], AvgRelAbsErr > epsilon (%e)\n"
	.align 4
.LC44:
	.string	"     AvgRelAbsErr > Epsilon (%e)\n"
	.align 4
.LC45:
	.string	"     For array b[], %d errors were found.\n"
	.align 4
.LC46:
	.string	"Failed Validation on array c[], AvgRelAbsErr > epsilon (%e)\n"
	.align 4
.LC47:
	.string	"     For array c[], %d errors were found.\n"
	.align 4
.LC48:
	.string	"Solution Validates: avg error less than %e on all three arrays\n"
	.text
.globl checkSTREAMresults
	.type	checkSTREAMresults, @function
checkSTREAMresults:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$152, %esp
	fld1
	fstpl	-112(%ebp)
	fldl	.LC20
	fstpl	-104(%ebp)
	fldz
	fstpl	-96(%ebp)
	fldl	-112(%ebp)
	fadd	%st(0), %st
	fstpl	-112(%ebp)
	fldl	.LC14
	fstpl	-88(%ebp)
	movl	$0, -20(%ebp)
	jmp	.L44
.L45:
	fldl	-112(%ebp)
	fstpl	-96(%ebp)
	fldl	-88(%ebp)
	fmull	-96(%ebp)
	fstpl	-104(%ebp)
	fldl	-112(%ebp)
	faddl	-104(%ebp)
	fstpl	-96(%ebp)
	fldl	-88(%ebp)
	fmull	-96(%ebp)
	faddl	-104(%ebp)
	fstpl	-112(%ebp)
	incl	-20(%ebp)
.L44:
	cmpl	$9, -20(%ebp)
	jle	.L45
	fldz
	fstpl	-80(%ebp)
	fldz
	fstpl	-72(%ebp)
	fldz
	fstpl	-64(%ebp)
	movl	$0, -24(%ebp)
	jmp	.L46
.L53:
	movl	a, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fsubl	-112(%ebp)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L47
	movl	a, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fsubl	-112(%ebp)
	jmp	.L48
.L47:
	movl	a, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fsubl	-112(%ebp)
	fchs
.L48:
	fldl	-80(%ebp)
	faddp	%st, %st(1)
	fstpl	-80(%ebp)
	movl	b, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fsubl	-104(%ebp)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L49
	movl	b, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fsubl	-104(%ebp)
	jmp	.L50
.L49:
	movl	b, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fsubl	-104(%ebp)
	fchs
.L50:
	fldl	-72(%ebp)
	faddp	%st, %st(1)
	fstpl	-72(%ebp)
	movl	c, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fsubl	-96(%ebp)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L51
	movl	c, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fsubl	-96(%ebp)
	jmp	.L52
.L51:
	movl	c, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fsubl	-96(%ebp)
	fchs
.L52:
	fldl	-64(%ebp)
	faddp	%st, %st(1)
	fstpl	-64(%ebp)
	incl	-24(%ebp)
.L46:
	cmpl	$63999999, -24(%ebp)
	jle	.L53
	fldl	-80(%ebp)
	fldl	.LC38
	fdivrp	%st, %st(1)
	fstpl	-56(%ebp)
	fldl	-72(%ebp)
	fldl	.LC38
	fdivrp	%st, %st(1)
	fstpl	-48(%ebp)
	fldl	-64(%ebp)
	fldl	.LC38
	fdivrp	%st, %st(1)
	fstpl	-40(%ebp)
	nop
	fldl	.LC39
	fstpl	-32(%ebp)
	movl	$0, -12(%ebp)
	fldl	-56(%ebp)
	fdivl	-112(%ebp)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L54
	fldl	-56(%ebp)
	fdivl	-112(%ebp)
	jmp	.L55
.L54:
	fldl	-56(%ebp)
	fdivl	-112(%ebp)
	fchs
.L55:
	fldl	-32(%ebp)
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	seta	%al
	testb	%al, %al
	je	.L56
	incl	-12(%ebp)
	movl	$.LC40, %eax
	fldl	-32(%ebp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	fldl	-56(%ebp)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L57
	fldl	-56(%ebp)
	jmp	.L58
.L57:
	fldl	-56(%ebp)
	fchs
.L58:
	fdivl	-112(%ebp)
	movl	$.LC41, %eax
	fstpl	20(%esp)
	fldl	-56(%ebp)
	fstpl	12(%esp)
	fldl	-112(%ebp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$0, -16(%ebp)
	movl	$0, -24(%ebp)
	jmp	.L59
.L63:
	movl	a, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fdivl	-112(%ebp)
	fld1
	fsubrp	%st, %st(1)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L60
	movl	a, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fdivl	-112(%ebp)
	fld1
	fsubrp	%st, %st(1)
	jmp	.L61
.L60:
	movl	a, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fdivl	-112(%ebp)
	fld1
	fsubrp	%st, %st(1)
	fchs
.L61:
	fldl	-32(%ebp)
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	seta	%al
	testb	%al, %al
	je	.L62
	incl	-16(%ebp)
.L62:
	incl	-24(%ebp)
.L59:
	cmpl	$63999999, -24(%ebp)
	jle	.L63
	movl	$.LC42, %eax
	movl	-16(%ebp), %edx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
.L56:
	fldl	-48(%ebp)
	fdivl	-104(%ebp)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L64
	fldl	-48(%ebp)
	fdivl	-104(%ebp)
	jmp	.L65
.L64:
	fldl	-48(%ebp)
	fdivl	-104(%ebp)
	fchs
.L65:
	fldl	-32(%ebp)
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	seta	%al
	testb	%al, %al
	je	.L66
	incl	-12(%ebp)
	movl	$.LC43, %eax
	fldl	-32(%ebp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	fldl	-48(%ebp)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L67
	fldl	-48(%ebp)
	jmp	.L68
.L67:
	fldl	-48(%ebp)
	fchs
.L68:
	fdivl	-104(%ebp)
	movl	$.LC41, %eax
	fstpl	20(%esp)
	fldl	-48(%ebp)
	fstpl	12(%esp)
	fldl	-104(%ebp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$.LC44, %eax
	fldl	-32(%ebp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$0, -16(%ebp)
	movl	$0, -24(%ebp)
	jmp	.L69
.L73:
	movl	b, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fdivl	-104(%ebp)
	fld1
	fsubrp	%st, %st(1)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L70
	movl	b, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fdivl	-104(%ebp)
	fld1
	fsubrp	%st, %st(1)
	jmp	.L71
.L70:
	movl	b, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fdivl	-104(%ebp)
	fld1
	fsubrp	%st, %st(1)
	fchs
.L71:
	fldl	-32(%ebp)
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	seta	%al
	testb	%al, %al
	je	.L72
	incl	-16(%ebp)
.L72:
	incl	-24(%ebp)
.L69:
	cmpl	$63999999, -24(%ebp)
	jle	.L73
	movl	$.LC45, %eax
	movl	-16(%ebp), %edx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
.L66:
	fldl	-40(%ebp)
	fdivl	-96(%ebp)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L74
	fldl	-40(%ebp)
	fdivl	-96(%ebp)
	jmp	.L75
.L74:
	fldl	-40(%ebp)
	fdivl	-96(%ebp)
	fchs
.L75:
	fldl	-32(%ebp)
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	seta	%al
	testb	%al, %al
	je	.L76
	incl	-12(%ebp)
	movl	$.LC46, %eax
	fldl	-32(%ebp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	fldl	-40(%ebp)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L77
	fldl	-40(%ebp)
	jmp	.L78
.L77:
	fldl	-40(%ebp)
	fchs
.L78:
	fdivl	-96(%ebp)
	movl	$.LC41, %eax
	fstpl	20(%esp)
	fldl	-40(%ebp)
	fstpl	12(%esp)
	fldl	-96(%ebp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$.LC44, %eax
	fldl	-32(%ebp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$0, -16(%ebp)
	movl	$0, -24(%ebp)
	jmp	.L79
.L83:
	movl	c, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fdivl	-96(%ebp)
	fld1
	fsubrp	%st, %st(1)
	fldz
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	setae	%al
	testb	%al, %al
	je	.L80
	movl	c, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fdivl	-96(%ebp)
	fld1
	fsubrp	%st, %st(1)
	jmp	.L81
.L80:
	movl	c, %eax
	movl	-24(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	fldl	(%eax)
	fdivl	-96(%ebp)
	fld1
	fsubrp	%st, %st(1)
	fchs
.L81:
	fldl	-32(%ebp)
	fxch	%st(1)
	fucomip	%st(1), %st
	fstp	%st(0)
	seta	%al
	testb	%al, %al
	je	.L82
	incl	-16(%ebp)
.L82:
	incl	-24(%ebp)
.L79:
	cmpl	$63999999, -24(%ebp)
	jle	.L83
	movl	$.LC47, %eax
	movl	-16(%ebp), %edx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
.L76:
	cmpl	$0, -12(%ebp)
	jne	.L85
	movl	$.LC48, %eax
	fldl	-32(%ebp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
.L85:
	leave
	ret
	.size	checkSTREAMresults, .-checkSTREAMresults
	.section	.rodata
	.align 8
.LC11:
	.long	0
	.long	1068401792
	.align 8
.LC12:
	.long	0
	.long	1078887552
	.align 8
.LC14:
	.long	0
	.long	1074266112
	.align 8
.LC20:
	.long	0
	.long	1073741824
	.align 8
.LC24:
	.long	0
	.long	1093567616
	.align 8
.LC34:
	.long	0
	.long	1075970048
	.align 8
.LC35:
	.long	2696277389
	.long	1051772663
	.align 8
.LC38:
	.long	0
	.long	1099859072
	.align 8
.LC39:
	.long	1749644930
	.long	1027352002
	.ident	"GCC: (GNU) 4.4.7 20120313 (Red Hat 4.4.7-17)"
	.section	.note.GNU-stack,"",@progbits
