/*
 * @brief CAN example
 * This example show how to use CAN peripheral
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


/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
#define CAN_CTRL_NO         0
#define LPC_CAN             (LPC_CAN1)
#define LPC_CAN_2           (LPC_CAN2)

#define FULL_CAN_AF_USED    1

#define CAN_TX_MSG_STD_ID (0x200)
#define CAN_TX_MSG_REMOTE_STD_ID (0x300)
#define CAN_TX_MSG_EXT_ID (0x10000200)
#define CAN_RX_MSG_ID (0x100)

//defines for Pins / ISR
#define GPIO_INTERRUPT_PIN     10	/* GPIO pin number mapped to interrupt */
#define GPIO_INTERRUPT_PORT    GPIOINT_PORT2	/* GPIO port number mapped to interrupt */
#define GPIO_IRQ_HANDLER  			GPIO_IRQHandler/* GPIO interrupt IRQ function name */
#define GPIO_INTERRUPT_NVIC_NAME    GPIO_IRQn	/* GPIO interrupt NVIC interrupt name */

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
static char WelcomeMenu[] = "\n\rHello NXP Semiconductors \r\n"
							"CAN DEMO : Use CAN to transmit and receive Message from CAN Analyzer\r\n"
							"CAN bit rate : 250kBit/s\r\n";
CAN_STD_ID_RANGE_ENTRY_T SffGrpSection[] = {
	{{CAN_CTRL_NO, 0, 0x300}, {CAN_CTRL_NO, 0, 0x400}},
	{{CAN_CTRL_NO, 0, 0x500}, {CAN_CTRL_NO, 0, 0x600}},
	{{CAN_CTRL_NO, 0, 0x700}, {CAN_CTRL_NO, 0, 0x780}},
};

/*****************************************************************************
 * Private functions
 ****************************************************************************/
/* Print error */
static void PrintCANErrorInfo(uint32_t Status)
{
	if (Status & CAN_ICR_EI) {
		//DEBUGOUT("Error Warning!\r\n");
	}
	if (Status & CAN_ICR_DOI) {
		DEBUGOUT("Data Overrun!\r\n");
	}
	if (Status & CAN_ICR_EPI) {
		DEBUGOUT("Error Passive!\r\n");
	}
	if (Status & CAN_ICR_ALI) {
		DEBUGOUT("Arbitration lost in the bit: %d(th)\r\n", CAN_ICR_ALCBIT_VAL(Status));
	}
	if (Status & CAN_ICR_BEI) {

		DEBUGOUT("Bus error !!!\r\n");

		if (Status & CAN_ICR_ERRDIR_RECEIVE) {
			DEBUGOUT("\t Error Direction: Transmiting\r\n");
		}
		else {
			DEBUGOUT("\t Error Direction: Receiving\r\n");
		}

		DEBUGOUT("\t Error Location: 0x%2x\r\n", CAN_ICR_ERRBIT_VAL(Status));
		DEBUGOUT("\t Error Type: 0x%1x\r\n", CAN_ICR_ERRC_VAL(Status));
	}
}

/* Print CAN Message */
static void PrintCANMsg(CAN_MSG_T *pMsg)
{
	uint8_t i;
	DEBUGOUT("\t**************************\r\n");
	DEBUGOUT("\tMessage Information: \r\n");
	DEBUGOUT("\tMessage Type: ");
	if (pMsg->ID & CAN_EXTEND_ID_USAGE) {
		DEBUGOUT(" Extend ID Message");
	}
	else {
		DEBUGOUT(" Standard ID Message");
	}
	if (pMsg->Type & CAN_REMOTE_MSG) {
		DEBUGOUT(", Remote Message");
	}
	DEBUGOUT("\r\n");
	DEBUGOUT("\tMessage ID :0x%x\r\n", (pMsg->ID & (~CAN_EXTEND_ID_USAGE)));
	DEBUGOUT("\tMessage Data :");
	for (i = 0; i < pMsg->DLC; i++)
		DEBUGOUT("%x ", pMsg->Data[i]);
	DEBUGOUT("\r\n\t**************************\r\n");
}

/* Reply remote message received */
static void ReplyRemoteMessage(CAN_MSG_T *pRcvMsg)
{
	CAN_MSG_T SendMsgBuf;
	CAN_BUFFER_ID_T   TxBuf;
	SendMsgBuf.ID  = pRcvMsg->ID + 1;
	SendMsgBuf.DLC = pRcvMsg->DLC;
	SendMsgBuf.Type = pRcvMsg->Type;
	SendMsgBuf.Data[0] = 'H';
	SendMsgBuf.Data[1] = 'E';
	SendMsgBuf.Data[2] = 'N';
	SendMsgBuf.Data[3] = 'R';
	SendMsgBuf.Data[4] = 'V';
	SendMsgBuf.Data[5] = 'I';
	SendMsgBuf.Data[6] = 'G';
	SendMsgBuf.Data[7] = 'G';

	TxBuf = Chip_CAN_GetFreeTxBuf(LPC_CAN);
	Chip_CAN_Send(LPC_CAN, TxBuf, &SendMsgBuf);
	DEBUGOUT("Message Replied!!!\r\n");
	PrintCANMsg(&SendMsgBuf);
}

/* Reply message received */
static void ReplyNormalMessage(CAN_MSG_T *pRcvMsg)
{
	CAN_MSG_T SendMsgBuf = *pRcvMsg;
	CAN_BUFFER_ID_T   TxBuf;
	SendMsgBuf.ID = CAN_TX_MSG_STD_ID;
	TxBuf = Chip_CAN_GetFreeTxBuf(LPC_CAN);
	Chip_CAN_Send(LPC_CAN, TxBuf, &SendMsgBuf);
	DEBUGOUT("Message Replied!!!\r\n");
	PrintCANMsg(&SendMsgBuf);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/
void CAN_IRQHandler(void)
{
#if FULL_CAN_AF_USED
	uint16_t i = 0, FullCANEntryNum = 0;
#endif
	uint32_t IntStatus;
	CAN_MSG_T RcvMsgBuf;
	IntStatus = Chip_CAN_GetIntStatus(LPC_CAN);

	PrintCANErrorInfo(IntStatus);

	/* New Message came */
	if (IntStatus & CAN_ICR_RI) {
		Chip_CAN_Receive(LPC_CAN, &RcvMsgBuf);
		DEBUGOUT("Message Received!!!\r\n");
		PrintCANMsg(&RcvMsgBuf);

		if (RcvMsgBuf.Type & CAN_REMOTE_MSG) {
			ReplyRemoteMessage(&RcvMsgBuf);
		}
		else {
			ReplyNormalMessage(&RcvMsgBuf);
		}

	}
#if FULL_CAN_AF_USED
	FullCANEntryNum = Chip_CAN_GetEntriesNum(LPC_CANAF, LPC_CANAF_RAM, CANAF_RAM_FULLCAN_SEC);
	if (FullCANEntryNum > 64) {
		FullCANEntryNum = 64;
	}
	for (i = 0; i < FullCANEntryNum; i++)
		if (Chip_CAN_GetFullCANIntStatus(LPC_CANAF, i)) {
			uint8_t SCC;
			Chip_CAN_FullCANReceive(LPC_CANAF, LPC_CANAF_RAM, i, &RcvMsgBuf, &SCC);
			if (SCC == CAN_CTRL_NO) {
				DEBUGOUT("FullCAN Message Received!!!\r\n");
				PrintCANMsg(&RcvMsgBuf);
				if (RcvMsgBuf.Type & CAN_REMOTE_MSG) {
					ReplyRemoteMessage(&RcvMsgBuf);
				}
				else {
					ReplyNormalMessage(&RcvMsgBuf);
				}
			}
		}

#endif /*FULL_CAN_AF_USED*/
}

void GPIO_IRQ_HANDLER(void)
{
	CAN_BUFFER_ID_T TxBuf;
	CAN_MSG_T SendMsgBuf;

	/* fill data in TX buffer */
	SendMsgBuf.ID = 45;//CAN_TX_MSG_STD_ID;
	SendMsgBuf.DLC = 8;
	SendMsgBuf.Type = CAN_REMOTE_MSG;
	SendMsgBuf.Data[0] = 'H';
	SendMsgBuf.Data[1] = 'E';
	SendMsgBuf.Data[2] = 'N';
	SendMsgBuf.Data[3] = 'R';
	SendMsgBuf.Data[4] = 'V';
	SendMsgBuf.Data[5] = 'I';
	SendMsgBuf.Data[6] = 'G';
	SendMsgBuf.Data[7] = 'G';

	TxBuf = Chip_CAN_GetFreeTxBuf(LPC_CAN);

	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIO_INTERRUPT_PORT, 1 << GPIO_INTERRUPT_PIN);
	Board_LED_Toggle(0);

	//send message
	Chip_CAN_Send(LPC_CAN_2, TxBuf, &SendMsgBuf);
	DEBUGOUT("Message Sent!!!\r\n");
	PrintCANMsg(&SendMsgBuf);

	//reply message
	Chip_CAN_Receive(LPC_CAN, &SendMsgBuf);
	ReplyRemoteMessage(&SendMsgBuf);
}

int main(void)
{
	SystemCoreClockUpdate();
	Board_Init();
	Board_LED_Set(0, false);
	DEBUGOUT(WelcomeMenu);
	/* use pins 0.0 and 0.1 */
	Board_CAN_Init(LPC_CAN);
	Board_CAN_Init(LPC_CAN_2);
	Chip_CAN_Init(LPC_CAN, LPC_CANAF, LPC_CANAF_RAM);
	Chip_CAN_Init(LPC_CAN_2, LPC_CANAF, LPC_CANAF_RAM);
	Chip_CAN_SetBitRate(LPC_CAN, 250000);
	Chip_CAN_SetBitRate(LPC_CAN_2, 250000);
	Chip_CAN_EnableInt(LPC_CAN, CAN_IER_BITMASK);
	Chip_CAN_EnableInt(LPC_CAN_2, CAN_IER_BITMASK);

	/* set acceptsance filters to bypass */
	Chip_CAN_SetAFMode(LPC_CANAF, CAN_AF_BYBASS_MODE);

	/* enable interrupts */
	NVIC_EnableIRQ(CAN_IRQn);

	// ISR Configs set up
	/* Configure GPIO interrupt pin as input */
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, GPIO_INTERRUPT_PORT, GPIO_INTERRUPT_PIN);

	/* Configure the GPIO interrupt */
	Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIO_INTERRUPT_PORT, 1 << GPIO_INTERRUPT_PIN);

	/* Enable interrupt in the NVIC */
	NVIC_ClearPendingIRQ(GPIO_INTERRUPT_NVIC_NAME);
	NVIC_EnableIRQ(GPIO_INTERRUPT_NVIC_NAME);

	/* Wait for interrupts - LED will toggle on each wakeup event */
	while (1) {
	__WFI();
	}

}
