
#include "board.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "ms_timer.h"

xSemaphoreHandle binary_semaphore;

// pins 5-8
#define GPIO_DEBUG_PORT1 1
#define GPIO_DEBUG_PIN1 24
#define GPIO_DEBUG_PORT2 1
#define GPIO_DEBUG_PIN2 23
#define GPIO_DEBUG_PORT3 1
#define GPIO_DEBUG_PIN3 20
#define GPIO_DEBUG_PORT4 0
#define GPIO_DEBUG_PIN4 21

can_tlg_t;

static void one_second_isr() {
	//Generates one second ISR
	long task_woken;
	xSemaphoreGiveFromISR(binary_semaphore, &task_woken);
	if(task_woken) {
		//puts("secondcallback");
		Chip_GPIO_WritePortBit(LPC_GPIO, GPIO_DEBUG_PORT2, GPIO_DEBUG_PIN2, 1);
		Board_LED_Toggle(3);
		Chip_GPIO_WritePortBit(LPC_GPIO, GPIO_DEBUG_PORT2, GPIO_DEBUG_PIN2, 0);
		//portYIELD_FROM_ISR(task_woken);
	}
}

/* High Semaphore task. receives a semaphore everytime one_second_isr is called. */
static void high_task(void *pvParameters) {
	while (1) {
		if(xSemaphoreTake(binary_semaphore, portMAX_DELAY)) {
			//puts("sem_task");
			Chip_GPIO_WritePortBit(LPC_GPIO, GPIO_DEBUG_PORT1, GPIO_DEBUG_PIN3, 1);
			Board_LED_Toggle(1);
			Chip_GPIO_WritePortBit(LPC_GPIO, GPIO_DEBUG_PORT1, GPIO_DEBUG_PIN3, 0);
		}
	}
}

/* Low priority tasks - not expected to finish its work before ISR re-calls*/
static void lowpri_task(void *pvParameters) {
	while (1) {
		Board_LED_Toggle(2);
		//puts("lowprio_task");
		Chip_GPIO_WritePortBit(LPC_GPIO, GPIO_DEBUG_PORT1, GPIO_DEBUG_PIN1, 1);
		for(long c = 0; c <6000000; c++);
		Chip_GPIO_WritePortBit(LPC_GPIO, GPIO_DEBUG_PORT1, GPIO_DEBUG_PIN1, 0);
		Board_LED_Toggle(2);
		vTaskDelay(configTICK_RATE_HZ / 20); //1 second
	}
}

/**
 * @brief	main routine for Semaphore example
 * @return	Nothing, function should not exit
 */
int main(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	Chip_IOCON_PinMuxSet(LPC_IOCON, GPIO_DEBUG_PORT1, GPIO_DEBUG_PIN1, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, GPIO_DEBUG_PORT1, GPIO_DEBUG_PIN1);

	Chip_IOCON_PinMuxSet(LPC_IOCON, GPIO_DEBUG_PORT3, GPIO_DEBUG_PIN3, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, GPIO_DEBUG_PORT3, GPIO_DEBUG_PIN3);

	Chip_IOCON_PinMuxSet(LPC_IOCON, GPIO_DEBUG_PORT2, GPIO_DEBUG_PIN2, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, GPIO_DEBUG_PORT2, GPIO_DEBUG_PIN2);

	Chip_IOCON_PinMuxSet(LPC_IOCON, GPIO_DEBUG_PORT4, GPIO_DEBUG_PIN4, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, GPIO_DEBUG_PORT4, GPIO_DEBUG_PIN4);

	//Setup the semaphore
	vSemaphoreCreateBinary(binary_semaphore);

	ms_timer_init (one_second_isr);

	// Semaphore tasks. Prints ticks every second after it has received semaphore.
	xTaskCreate(high_task, (signed char *) "high_task", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
	(xTaskHandle *) NULL);

	xTaskCreate(lowpri_task, (signed char *) "lowpri_task",	configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}
