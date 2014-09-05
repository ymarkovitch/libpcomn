	.file	"tth.cpp"
	.text
	.align 2
	.p2align 4,,15
.globl _Z1fPKN5pcomn7ihandleIi5h_tagEE
	.type	_Z1fPKN5pcomn7ihandleIi5h_tagEE, @function
_Z1fPKN5pcomn7ihandleIi5h_tagEE:
.LFB1634:
	pushl	%ebp
.LCFI0:
	movl	%esp, %ebp
.LCFI1:
	movl	8(%ebp), %edx
	popl	%ebp
	movl	4(%edx), %ecx
	movl	(%edx), %eax
	addl	%ecx, %eax
	movl	8(%edx), %ecx
	addl	%ecx, %eax
	movl	12(%edx), %ecx
	addl	%ecx, %eax
	movl	16(%edx), %ecx
	addl	%ecx, %eax
	movl	20(%edx), %ecx
	addl	%ecx, %eax
	movl	24(%edx), %ecx
	addl	%ecx, %eax
	movl	28(%edx), %ecx
	addl	%ecx, %eax
	movl	32(%edx), %ecx
	addl	%ecx, %eax
	movl	36(%edx), %ecx
	addl	%ecx, %eax
	movl	40(%edx), %ecx
	addl	%ecx, %eax
	movl	44(%edx), %ecx
	addl	%ecx, %eax
	movl	48(%edx), %ecx
	addl	%ecx, %eax
	movl	52(%edx), %ecx
	addl	%ecx, %eax
	movl	56(%edx), %ecx
	addl	%ecx, %eax
	movl	60(%edx), %ecx
	addl	%ecx, %eax
	ret
.LFE1634:
	.size	_Z1fPKN5pcomn7ihandleIi5h_tagEE, .-_Z1fPKN5pcomn7ihandleIi5h_tagEE
.globl __gxx_personality_v0
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"%d\n"
	.text
	.align 2
	.p2align 4,,15
.globl main
	.type	main, @function
main:
.LFB1635:
	leal	4(%esp), %ecx
.LCFI2:
	andl	$-16, %esp
	pushl	-4(%ecx)
.LCFI3:
	xorl	%eax, %eax
	pushl	%ebp
.LCFI4:
	movl	%esp, %ebp
.LCFI5:
	pushl	%ecx
.LCFI6:
	subl	$84, %esp
.LCFI7:
	movl	$0, -68(%ebp)
	movl	$0, -64(%ebp)
	movl	$0, -60(%ebp)
	movl	$0, -56(%ebp)
	movl	$0, -52(%ebp)
	movl	$0, -48(%ebp)
	movl	$0, -44(%ebp)
	movl	$0, -40(%ebp)
	movl	$0, -36(%ebp)
	movl	$0, -32(%ebp)
	movl	$0, -28(%ebp)
	movl	$0, -24(%ebp)
	movl	$0, -20(%ebp)
	movl	$0, -16(%ebp)
	movl	$0, -12(%ebp)
	movl	$0, -8(%ebp)
	movl	%eax, 4(%esp)
	movl	$.LC0, (%esp)
	call	printf
	addl	$84, %esp
	xorl	%eax, %eax
	popl	%ecx
	popl	%ebp
	leal	-4(%ecx), %esp
	ret
.LFE1635:
	.size	main, .-main
	.weakref	_Z20__gthrw_pthread_oncePiPFvvE,pthread_once
	.weakref	_Z27__gthrw_pthread_getspecificj,pthread_getspecific
	.weakref	_Z27__gthrw_pthread_setspecificjPKv,pthread_setspecific
	.weakref	_Z22__gthrw_pthread_createPmPK14pthread_attr_tPFPvS3_ES3_,pthread_create
	.weakref	_Z22__gthrw_pthread_cancelm,pthread_cancel
	.weakref	_Z26__gthrw_pthread_mutex_lockP15pthread_mutex_t,pthread_mutex_lock
	.weakref	_Z29__gthrw_pthread_mutex_trylockP15pthread_mutex_t,pthread_mutex_trylock
	.weakref	_Z28__gthrw_pthread_mutex_unlockP15pthread_mutex_t,pthread_mutex_unlock
	.weakref	_Z26__gthrw_pthread_mutex_initP15pthread_mutex_tPK19pthread_mutexattr_t,pthread_mutex_init
	.weakref	_Z26__gthrw_pthread_key_createPjPFvPvE,pthread_key_create
	.weakref	_Z26__gthrw_pthread_key_deletej,pthread_key_delete
	.weakref	_Z30__gthrw_pthread_mutexattr_initP19pthread_mutexattr_t,pthread_mutexattr_init
	.weakref	_Z33__gthrw_pthread_mutexattr_settypeP19pthread_mutexattr_ti,pthread_mutexattr_settype
	.weakref	_Z33__gthrw_pthread_mutexattr_destroyP19pthread_mutexattr_t,pthread_mutexattr_destroy
	.section	.eh_frame,"a",@progbits
.Lframe1:
	.long	.LECIE1-.LSCIE1
.LSCIE1:
	.long	0x0
	.byte	0x1
	.string	"zP"
	.uleb128 0x1
	.sleb128 -4
	.byte	0x8
	.uleb128 0x5
	.byte	0x0
	.long	__gxx_personality_v0
	.byte	0xc
	.uleb128 0x4
	.uleb128 0x4
	.byte	0x88
	.uleb128 0x1
	.align 4
.LECIE1:
.LSFDE3:
	.long	.LEFDE3-.LASFDE3
.LASFDE3:
	.long	.LASFDE3-.Lframe1
	.long	.LFB1635
	.long	.LFE1635-.LFB1635
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI2-.LFB1635
	.byte	0xc
	.uleb128 0x1
	.uleb128 0x0
	.byte	0x9
	.uleb128 0x4
	.uleb128 0x1
	.byte	0x4
	.long	.LCFI3-.LCFI2
	.byte	0xc
	.uleb128 0x4
	.uleb128 0x4
	.byte	0x4
	.long	.LCFI4-.LCFI3
	.byte	0xe
	.uleb128 0x8
	.byte	0x85
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI5-.LCFI4
	.byte	0xd
	.uleb128 0x5
	.byte	0x4
	.long	.LCFI6-.LCFI5
	.byte	0x84
	.uleb128 0x3
	.align 4
.LEFDE3:
	.ident	"GCC: (GNU) 4.2.4 (Ubuntu 4.2.4-1ubuntu3)"
	.section	.note.GNU-stack,"",@progbits
