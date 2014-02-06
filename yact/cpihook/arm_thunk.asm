    THUMB

    AREA    |.text|, CODE, THUMB

|MyThunk|
       push.w           {r0-r3}
       push.w           {r4-r11,lr}
       add              r0, sp, #36
       ldr.w            r1,=0x55667788
       blx              r1
       ldr.w            r0, [sp,#36]
       ldr.w            r1, [sp,#40]
       ldr.w            r2, [sp,#44]
       ldr.w            r3, [sp,#48]
       addw             R11, SP, #0x1C
       ldr.w            pc, =0x44332211


    END
