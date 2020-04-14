	.set noreorder
	.set noat

	.global SaveVFPUContext
	.ent    SaveVFPUContext

SaveVFPUContext:
        mfc0       $t0, Status
        lui        $at, 0x4000
        or         $t7, $t0, $at
        mtc0       $t7, Status
        nop
        sv.q       C000, 0x0($a0)
        sv.q       C010, 0x10($a0)
        sv.q       C020, 0x20($a0)
        sv.q       C030, 0x30($a0)
        sv.q       C100, 0x40($a0)
        sv.q       C110, 0x50($a0)
        sv.q       C120, 0x60($a0)
        sv.q       C130, 0x70($a0)
        sv.q       C200, 0x80($a0)
        sv.q       C210, 0x90($a0)
        sv.q       C220, 0xA0($a0)
        sv.q       C230, 0xB0($a0)
        sv.q       C300, 0xC0($a0)
        sv.q       C310, 0xD0($a0)
        sv.q       C320, 0xE0($a0)
        sv.q       C330, 0xF0($a0)
        sv.q       C400, 0x100($a0)
        sv.q       C410, 0x110($a0)
        sv.q       C420, 0x120($a0)
        sv.q       C430, 0x130($a0)
        sv.q       C500, 0x140($a0)
        sv.q       C510, 0x150($a0)
        sv.q       C520, 0x160($a0)
        sv.q       C530, 0x170($a0)
        sv.q       C600, 0x180($a0)
        sv.q       C610, 0x190($a0)
        sv.q       C620, 0x1A0($a0)
        sv.q       C630, 0x1B0($a0)
        sv.q       C700, 0x1C0($a0)
        sv.q       C710, 0x1D0($a0)
        sv.q       C720, 0x1E0($a0)
        sv.q       C730, 0x1F0($a0)
        mfvc       $t7, VFPU_PFXS
        mfvc       $t6, VFPU_PFXT
        mfvc       $t5, VFPU_PFXD
        mfvc       $t4, VFPU_CC
        mfvc       $t3, VFPU_INF4
        sw         $t7, 0x200($a0)
        sw         $t6, 0x204($a0)
        sw         $t5, 0x208($a0)
        sw         $t4, 0x20C($a0)
        sw         $t3, 0x210($a0)
        mfvc       $t7, VFPU_RCX0
        mfvc       $t6, VFPU_RCX1
        mfvc       $t5, VFPU_RCX2
        mfvc       $t4, VFPU_RCX3
        sw         $t7, 0x220($a0)
        sw         $t6, 0x224($a0)
        sw         $t5, 0x228($a0)
        sw         $t4, 0x22C($a0)
        mfvc       $t7, VFPU_RCX4
        mfvc       $t6, VFPU_RCX5
        mfvc       $t5, VFPU_RCX6
        mfvc       $t4, VFPU_RCX7
        sw         $t7, 0x230($a0)
        sw         $t6, 0x234($a0)
        sw         $t5, 0x238($a0)
        sw         $t4, 0x23C($a0)
        vsync      0x320
        vnop
        mtc0       $t0, Status
        nop
        jr         $ra
        nop

	.end SaveVFPUContext

	.set noreorder
	.set noat

	.global LoadVFPUContext
	.ent    LoadVFPUContext

LoadVFPUContext:
        mfc0       $t0, Status
        lui        $at, 0x4000
        or         $t7, $t0, $at
        mtc0       $t7, Status
        nop
        vsync      0x320
        lv.q       C000, 0x0($a0)
        lv.q       C010, 0x10($a0)
        lv.q       C020, 0x20($a0)
        lv.q       C030, 0x30($a0)
        lv.q       C100, 0x40($a0)
        lv.q       C110, 0x50($a0)
        lv.q       C120, 0x60($a0)
        lv.q       C130, 0x70($a0)
        lv.q       C200, 0x80($a0)
        lv.q       C210, 0x90($a0)
        lv.q       C220, 0xA0($a0)
        lv.q       C230, 0xB0($a0)
        lv.q       C300, 0xC0($a0)
        lv.q       C310, 0xD0($a0)
        lv.q       C320, 0xE0($a0)
        lv.q       C330, 0xF0($a0)
        lv.q       C400, 0x100($a0)
        lv.q       C410, 0x110($a0)
        lv.q       C420, 0x120($a0)
        lv.q       C430, 0x130($a0)
        lv.q       C500, 0x140($a0)
        lv.q       C510, 0x150($a0)
        lv.q       C520, 0x160($a0)
        lv.q       C530, 0x170($a0)
        lv.q       C600, 0x180($a0)
        lv.q       C610, 0x190($a0)
        lv.q       C620, 0x1A0($a0)
        lv.q       C630, 0x1B0($a0)
        lv.q       C700, 0x1C0($a0)
        lv.q       C710, 0x1D0($a0)
        lv.q       C720, 0x1E0($a0)
        lv.q       C730, 0x1F0($a0)
        lw         $t7, 0x200($a0)
        lw         $t6, 0x204($a0)
        lw         $t5, 0x208($a0)
        lw         $t4, 0x20C($a0)
        lw         $t3, 0x210($a0)
        mtvc       $t7, VFPU_PFXS
        mtvc       $t6, VFPU_PFXT
        mtvc       $t5, VFPU_PFXD
        mtvc       $t4, VFPU_CC
        mtvc       $t3, VFPU_INF4
        lw         $t7, 0x220($a0)
        lw         $t6, 0x224($a0)
        lw         $t5, 0x228($a0)
        lw         $t4, 0x22C($a0)
        mtvc       $t7, VFPU_RCX0
        mtvc       $t6, VFPU_RCX1
        mtvc       $t5, VFPU_RCX2
        mtvc       $t4, VFPU_RCX3
        lw         $t7, 0x230($a0)
        lw         $t6, 0x234($a0)
        lw         $t5, 0x238($a0)
        lw         $t4, 0x23C($a0)
        mtvc       $t7, VFPU_RCX4
        mtvc       $t6, VFPU_RCX5
        mtvc       $t5, VFPU_RCX6
        mtvc       $t4, VFPU_RCX7
        mtc0       $t0, Status
        nop
        jr         $ra
        nop

	.end LoadVFPUContext
