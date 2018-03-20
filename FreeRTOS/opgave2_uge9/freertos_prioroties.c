/*
 * @brief FreeRTOS Blinky example
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define DEBUG_PORT1		1
#define DEBUG_PIN1		12
#define DEBUG_PORT2		1
#define DEBUG_PIN2		11
#define DEBUG_PORT3		1
#define DEBUG_PIN3		6
#define DEBUG_PORT4		0
#define DEBUG_PIN4		6

/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);

	/*Setup IO pins for profiling to outputs */
	//Chip_GPIO_WriteDirBit(LPC_GPIO, DEBUG_PORT1, DEBUG_PIN1, true);
	//Chip_GPIO_WriteDirBit(LPC_GPIO, DEBUG_PORT2, DEBUG_PIN2, true);
	//Chip_GPIO_WriteDirBit(LPC_GPIO, DEBUG_PORT3, DEBUG_PIN3, true);
	//Chip_GPIO_WriteDirBit(LPC_GPIO, DEBUG_PORT4, DEBUG_PIN4, true);

	// setup for port 1 pin 12 (low priority)
	Chip_IOCON_PinMuxSet(LPC_IOCON, DEBUG_PORT1, DEBUG_PIN1, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, DEBUG_PORT1, DEBUG_PIN1);

	// setup for port 1 pin 11 (high priority)
	Chip_IOCON_PinMuxSet(LPC_IOCON, DEBUG_PORT2, DEBUG_PIN2, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, DEBUG_PORT2, DEBUG_PIN2);

	//setup for port 1 pin 6
	Chip_IOCON_PinMuxSet(LPC_IOCON, DEBUG_PORT3, DEBUG_PIN3, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, DEBUG_PORT3, DEBUG_PIN3);

	//setup for port 0 pin 6
	Chip_IOCON_PinMuxSet(LPC_IOCON, DEBUG_PORT4, DEBUG_PIN4, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, DEBUG_PORT4, DEBUG_PIN4);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	main routine for FreeRTOS blinky example
 * @return	Nothing, function should not exit
 */

typedef struct
{
	unsigned int ID;
	unsigned char ID_ext;
	unsigned char DLC;
	unsigned char Data[8];
}can_tlg_t;

can_tlg_t canO;

void CANMSG(void)
{
	canO.ID = 14;
	canO.Data[1] = 173;
	canO.Data[2] = 173;
	canO.Data[3] = 173;
	canO.Data[4] = 173;
	canO.Data[5] = 173;
	canO.Data[6] = 173;
	canO.Data[7] = 173;
}

xQueueHandle queue_handle = NULL;

void tx(void *p)
{
	while(1) {
		printf("tx() send to queue : \n");
		long ok = xQueueSend(queue_handle, &canO, 500);
		puts(ok ? "OK" : "FAILED");

		vTaskDelay(200);
	}
}

void rx(void *p)
{
	while(1) {
		if (xQueueReceive(queue_handle, &canO, 500)) {
			printf("Got item from queue %d %d %d %d %d %d %d %d \n", canO.ID, canO.Data[1],
					canO.Data[2], canO.Data[3], canO.Data[4], canO.Data[5],
					canO.Data[6], canO.Data[7]);
		}
	}
}

int main(void)
{
	prvSetupHardware();

	CANMSG();

	setvbuf(stdout, 0, _IONBF, 0);
	setvbuf(stdout, 0, _IONBF, 0);

	queue_handle = xQueueCreate(16, sizeof(canO));

	/* LED1 toggle thread */
	xTaskCreate(tx, (signed char * ) "tx", 1024, NULL, 1, NULL);

	/* LED2 toggle thread */
	xTaskCreate(rx, (signed char * ) "rx", 1024, NULL, 2, NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}
