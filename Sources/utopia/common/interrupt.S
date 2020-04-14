
	.set noreorder
	.set noat

	.global sceKernelCpuSuspendIntr
	.ent    sceKernelCpuSuspendIntr

sceKernelCpuSuspendIntr:
	mfic	$v0, $0
	mtic	$0, $0
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	jr		$ra
	nop

	.end sceKernelCpuSuspendIntr

	.global sceKernelCpuResumeIntr
	.ent    sceKernelCpuResumeIntr

sceKernelCpuResumeIntr:
	mtic	$a0, $0
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	jr		$ra
	nop

	.end sceKernelCpuResumeIntr
