/*
 * hooks.c
 *
 *  Created on: 23 cze 2017
 *      Author: Kuba
 */

#include "config.h"

void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName) {
	/* Bad thing =( Print some useful info and reboot */
	printf("Stack overflow in %s\n", pcTaskName);
	for (;;) {};
}

void vApplicationIdleHook(void) {
	IWDG_ReloadCounter();
}

void vApplicationTickHook(void) {
}

void HardFault_Handler(void) {
	/* Very bad thing =( Print some useful info and reboot */
	led_bar_write(0, 0, 255);

    __asm(  ".syntax unified\n"
                    "MOVS   R0, #4  \n"
                    "MOV    R1, LR  \n"
                    "TST    R0, R1  \n"
                    "BEQ    _MSP    \n"
                    "MRS    R0, PSP \n"
                    "B      HardFault_HandlerC      \n"
            "_MSP:  \n"
                    "MRS    R0, MSP \n"
                    "B      HardFault_HandlerC      \n"
            ".syntax divided\n") ;

	for (;;) {};
}

void HardFault_HandlerC(unsigned long *hardfault_args){
        volatile unsigned long stacked_r0 ;
        volatile unsigned long stacked_r1 ;
        volatile unsigned long stacked_r2 ;
        volatile unsigned long stacked_r3 ;
        volatile unsigned long stacked_r12 ;
        volatile unsigned long stacked_lr ;
        volatile unsigned long stacked_pc ;
        volatile unsigned long stacked_psr ;
        volatile unsigned long _CFSR ;
        volatile unsigned long _HFSR ;
        volatile unsigned long _DFSR ;
        volatile unsigned long _AFSR ;
        volatile unsigned long _BFAR ;
        volatile unsigned long _MMAR ;

        stacked_r0 = ((unsigned long)hardfault_args[0]) ;
        stacked_r1 = ((unsigned long)hardfault_args[1]) ;
        stacked_r2 = ((unsigned long)hardfault_args[2]) ;
        stacked_r3 = ((unsigned long)hardfault_args[3]) ;
        stacked_r12 = ((unsigned long)hardfault_args[4]) ;
        stacked_lr = ((unsigned long)hardfault_args[5]) ;
        stacked_pc = ((unsigned long)hardfault_args[6]) ;
        stacked_psr = ((unsigned long)hardfault_args[7]) ;

        // Configurable Fault Status Register
        // Consists of MMSR, BFSR and UFSR
        _CFSR = (*((volatile unsigned long *)(0xE000ED28))) ;

        // Hard Fault Status Register
        _HFSR = (*((volatile unsigned long *)(0xE000ED2C))) ;

        // Debug Fault Status Register
        _DFSR = (*((volatile unsigned long *)(0xE000ED30))) ;

        // Auxiliary Fault Status Register
        _AFSR = (*((volatile unsigned long *)(0xE000ED3C))) ;

        // Read the Fault Address Registers. These may not contain valid values.
        // Check BFARVALID/MMARVALID to see if they are valid values
        // MemManage Fault Address Register
        _MMAR = (*((volatile unsigned long *)(0xE000ED34))) ;
        // Bus Fault Address Register
        _BFAR = (*((volatile unsigned long *)(0xE000ED38))) ;

        for(;;){}
}

void NMI_Handler(void) {
	printf("NMI\n");
	for (;;) {}
}

void MemManage_Handler(void) {
	printf("MemManage\n");
	for (;;) {}
}

void BusFault_Handler(void) {
	printf("BusFault\n");
	for (;;) {}
}

void UsageFault_Handler(void) {
	printf("UsageFault\n");
	for (;;) {}
}

void DebugMon_Handler(void) {
	printf("DebugMon\n");
	for (;;) {}
}
