/** stm32c0xx_it.c — обробники переривань Cortex-M0+ */
#include "main.h"

void NMI_Handler(void)       { while (1) { } }
void HardFault_Handler(void) { while (1) { } }
void SVC_Handler(void)       { }
void PendSV_Handler(void)    { }
void SysTick_Handler(void)   { HAL_IncTick(); }
