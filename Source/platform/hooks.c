/*
 * hooks.c
 *
 *  Created on: 23 cze 2017
 *      Author: Kuba
 */

#include "config.h"

void vApplicationStackOverflowHook(xTaskHandle pxTask,
                                   signed char *pcTaskName) {
  /* Bad thing =( Print some useful info and reboot */
  printf("Stack overflow in %s\n", pcTaskName);
  for (;;) {
  }
}

void vApplicationIdleHook(void) {
	IWDG_ReloadCounter();
}

void vApplicationTickHook(void) {
}

void HardFault_Handler(void) {
  /* Very bad thing =( Print some useful info and reboot */
  printf("HardFault CFSR = %X  BFAR = %X\n", (unsigned int) SCB->CFSR,
         (unsigned int) SCB->BFAR);
  for (;;) {}
}

void NMI_Handler(void) {
  printf("NMI\n");
  for (;;) {
  }
}

void MemManage_Handler(void) {
  printf("MemManage\n");
  for (;;) {
  }
}

void BusFault_Handler(void) {
  printf("BusFault\n");
  for (;;) {
  }
}

void UsageFault_Handler(void) {
  printf("UsageFault\n");
  for (;;) {
  }
}

void DebugMon_Handler(void) {
  printf("DebugMon\n");
  for (;;) {
  }
}
