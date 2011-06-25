/*! benchmark.c
 Created: 6/8/2011 10:27:18 PM
 Author : Travis

 - Copyright (c) 2011, Travis Lee Robinson
 - All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Travis Lee Robinson nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS ROBINSON BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <benchmark.h>
#include <conf_board.h>
#include <string.h>

#define HAS_CRITICAL_SECTION()  irqflags_t flags
#define ENTER_CRITICAL_SECTION() flags = cpu_irq_save()
#define LEAVE_CRITICAL_SECTION() cpu_irq_restore(flags)


/** BMARK MACROS ****************************************************/
#define mSetWritePacketID(BufferPtr, BufferSize, FillCount, NextPacketKey)	\
	if ((BufferSize)>0)														\
	{																		\
		if (FillCount < ((BM_BUFFER_COUNT*2)*(BM_BUFFER_SIZE/BM_EP_MAX_PACKET_SIZE)))	\
		{																	\
			FillCount++;													\
			Bm_FillBuffer(BufferPtr,BufferSize);							\
		}																	\
		BufferPtr[1]=NextPacketKey++;										\
	}

#define Bm_SubmitTransfer(ep,shortPacketEn,buffer,bufferLength,callbackFn) udd_ep_run(ep,shortPacketEn,buffer,bufferLength,callbackFn)
#define Bm_SubmitRead(ep,xferEL,callbackFn) Bm_SubmitTransfer(ep,true,(xferEL)->Buffer,sizeof((xferEL)->Buffer),callbackFn)
#define Bm_SubmitWrite(ep,xferEL,callbackFn) Bm_SubmitTransfer(ep,false,(xferEL)->Buffer,(xferEL)->Transferred,callbackFn)

#define Bm_XferMove(fromList, toList,  moveEL) do {		\
	DL_DELETE(fromList, moveEL);						\
	DL_APPEND(toList, moveEL);							\
 } while(0)

#define Bm_IsNewTest() (Bm_TestType != Bm_PrevTestType)

typedef struct _BM_XFER_EL
{
	COMPILER_WORD_ALIGNED uint8_t Buffer[BM_BUFFER_SIZE];
	uint16_t Transferred;

	struct _BM_XFER_EL* next;
	struct _BM_XFER_EL* prev;

} BM_XFER_EL;

typedef void (*Bm_RunTestDelegate) (void);

static volatile Bm_RunTestDelegate Bm_RunTest = NULL;

COMPILER_WORD_ALIGNED static BM_XFER_EL Bm_XferArray[2][BM_BUFFER_COUNT];

static BM_XFER_EL* Bm_XfersReadyForTx;
static BM_XFER_EL* Bm_XfersReadyForRx;

static volatile bool Bm_IsJobPendingTx = false;
static volatile bool Bm_IsJobPendingRx = false;

static volatile uint8_t Bm_TestType = TEST_LOOP;
static volatile uint8_t Bm_PrevTestType = TEST_NONE;
static volatile uint8_t Bm_FillCount = 0;
static volatile uint8_t Bm_NextPacketKey = 0;
static volatile uint8_t Bm_Led_Counter = 0;

COMPILER_WORD_ALIGNED static uint8_t Bm_VendorBuffer[8];

void RunApplication(void);

static void Bm_Init(void);

static void Bm_XferLoopCompleteTx(udd_ep_status_t status, iram_size_t nb_transfered);
static void Bm_XferLoopCompleteRx(udd_ep_status_t status, iram_size_t nb_transfered);
static void Bm_XferCompleteTx(udd_ep_status_t status, iram_size_t nb_transfered);
static void Bm_XferCompleteRx(udd_ep_status_t status, iram_size_t nb_transfered);

static void Bm_RunTest_Loop(void);
static void Bm_RunTest_Read(void);
static void Bm_RunTest_Write(void);

static void Bm_FillBuffer(uint8_t* pBuffer, uint16_t size);
static void Bm_InitXferArray(BM_XFER_EL** Bank1_AddListRef, BM_XFER_EL** Bank2_AddListRef);

static void Bm_FillBuffer(uint8_t* pBuffer, uint16_t size)
{
	uint8_t dataByte = 0;
	uint16_t counter;

	for (counter = 0; counter < size; counter++)
	{
		pBuffer[counter] = dataByte++;
		if (dataByte == 0) dataByte++;
	}
}

static void Bm_XferLoopCompleteTx(udd_ep_status_t status, iram_size_t nb_transfered)
{
	BM_XFER_EL* moveEL;
	
	if (++Bm_Led_Counter==0)
		LED1_TGL();
		
	if (status)
	{
		Bm_IsJobPendingTx = false;
		return;
	}

	moveEL = Bm_XfersReadyForTx;
	moveEL->Transferred = (uint16_t)nb_transfered;
	Bm_XferMove(Bm_XfersReadyForTx, Bm_XfersReadyForRx, moveEL);

	if (Bm_XfersReadyForTx)
	{
		Bm_IsJobPendingTx = Bm_SubmitWrite(BM_EP_TX, Bm_XfersReadyForTx, Bm_XferLoopCompleteTx);
	}
	else
	{
		Bm_IsJobPendingTx = false;
	}
}

static void Bm_XferLoopCompleteRx(udd_ep_status_t status, iram_size_t nb_transfered)
{
	BM_XFER_EL* moveEL;

	if (++Bm_Led_Counter==0)
		LED1_TGL();

	if (status)
	{
		Bm_IsJobPendingRx = false;
		return;
	}

	moveEL = Bm_XfersReadyForRx;
	moveEL->Transferred = (uint16_t)nb_transfered;
	Bm_XferMove(Bm_XfersReadyForRx, Bm_XfersReadyForTx, moveEL);

	if (Bm_XfersReadyForRx)
	{
		Bm_IsJobPendingRx = Bm_SubmitRead(BM_EP_RX, Bm_XfersReadyForRx, Bm_XferLoopCompleteRx);
	}
	else
	{
		Bm_IsJobPendingRx = false;
	}
}

static void Bm_XferCompleteRx(udd_ep_status_t status, iram_size_t nb_transfered)
{
	BM_XFER_EL* moveEL;

	if (++Bm_Led_Counter==0)
		LED1_TGL();

	if (status)
	{
		Bm_IsJobPendingRx = false;
		return;
	}

	moveEL = Bm_XfersReadyForRx;
	moveEL->Transferred = (uint16_t)nb_transfered;
	Bm_XferMove(Bm_XfersReadyForRx, Bm_XfersReadyForRx, moveEL);

	Bm_IsJobPendingRx = Bm_SubmitRead(BM_EP_RX, Bm_XfersReadyForRx, Bm_XferCompleteRx);
}

static void Bm_XferCompleteTx(udd_ep_status_t status, iram_size_t nb_transfered)
{
	BM_XFER_EL* moveEL;
	int i;
	uint8_t* buffer;
	
	if (++Bm_Led_Counter==0)
		LED1_TGL();

	if (status)
	{
		Bm_IsJobPendingTx = false;
		return;
	}

	moveEL = Bm_XfersReadyForTx;
	moveEL->Transferred = (uint16_t)nb_transfered;
	Bm_XferMove(Bm_XfersReadyForTx, Bm_XfersReadyForTx, moveEL);

	Bm_XfersReadyForTx->Transferred = sizeof(Bm_XfersReadyForTx->Buffer);
	for(i=0; i < sizeof(Bm_XfersReadyForTx->Buffer); i+=BM_EP_MAX_PACKET_SIZE)
	{
		buffer=&Bm_XfersReadyForTx->Buffer[i];
		mSetWritePacketID(buffer, BM_EP_MAX_PACKET_SIZE, Bm_FillCount, Bm_NextPacketKey);
	}
	Bm_IsJobPendingTx = Bm_SubmitWrite(BM_EP_TX, Bm_XfersReadyForTx, Bm_XferCompleteTx);

	//Bm_IsJobPendingTx = false;
}

static void Bm_RunTest_Loop(void)
{
	if (!Bm_IsJobPendingRx && Bm_XfersReadyForRx)
	{
		Bm_IsJobPendingRx = Bm_SubmitRead(BM_EP_RX, Bm_XfersReadyForRx, Bm_XferLoopCompleteRx);
	}
	if (!Bm_IsJobPendingTx && Bm_XfersReadyForTx)
	{
		Bm_IsJobPendingTx = Bm_SubmitWrite(BM_EP_TX, Bm_XfersReadyForTx, Bm_XferLoopCompleteTx);
	}
}

static void Bm_RunTest_Read(void)
{
	if (!Bm_IsJobPendingRx)
	{
		Bm_IsJobPendingRx = Bm_SubmitRead(BM_EP_RX, Bm_XfersReadyForRx, Bm_XferCompleteRx);
	}
}

static void Bm_RunTest_Write(void)
{
	if (!Bm_IsJobPendingTx)
	{
		int i;
		uint8_t* buffer;
		Bm_XfersReadyForTx->Transferred = sizeof(Bm_XfersReadyForTx->Buffer);
		for(i=0; i < sizeof(Bm_XfersReadyForTx->Buffer); i+=BM_EP_MAX_PACKET_SIZE)
		{
			buffer=&Bm_XfersReadyForTx->Buffer[i];
			mSetWritePacketID(buffer, BM_EP_MAX_PACKET_SIZE, Bm_FillCount, Bm_NextPacketKey);
		}
		Bm_IsJobPendingTx = Bm_SubmitWrite(BM_EP_TX, Bm_XfersReadyForTx, Bm_XferCompleteTx);
	}
}

void RunApplication(void)
{
	Bm_Init();

	while(true)
	{
		if (Bm_IsNewTest())
		{
			if (Bm_IsJobPendingRx)
			{
				udd_ep_abort(BM_EP_RX);
			}
			else if (Bm_IsJobPendingTx)
			{
				udd_ep_abort(BM_EP_TX);
			}
			else
			{
				Bm_Init();
			}
		}
		else
		{
			if (Bm_RunTest)
			{
				Bm_RunTest();
			}
		}
	}
}

static void Bm_InitXferArray(BM_XFER_EL** Bank1_AddListRef, BM_XFER_EL** Bank2_AddListRef)
{
	int i;

	for(i = 0; i < BM_BUFFER_COUNT; i++)
	{
		Bm_XferArray[0][i].Transferred=BM_BUFFER_SIZE;
		Bm_XferArray[1][i].Transferred=BM_BUFFER_SIZE;
		
		Bm_XferArray[0][i].next=NULL;
		Bm_XferArray[1][i].next=NULL;
		
		Bm_XferArray[0][i].prev=NULL;
		Bm_XferArray[1][i].prev=NULL;

		DL_APPEND(*Bank1_AddListRef, &Bm_XferArray[0][i]);
		DL_APPEND(*Bank2_AddListRef, &Bm_XferArray[1][i]);
	}
}

static void Bm_Init(void)
{
	Bm_FillCount = 0;
	Bm_NextPacketKey = 0;
	Bm_XfersReadyForTx = NULL;
	Bm_XfersReadyForRx = NULL;
	Bm_Led_Counter = 0;
	LED1_OFF();
	
	switch (Bm_TestType)
	{
	case TEST_NONE:
		Bm_InitXferArray(&Bm_XfersReadyForRx, &Bm_XfersReadyForRx);
		Bm_RunTest = Bm_RunTest_Read;
		break;
	case TEST_PCREAD:
		Bm_InitXferArray(&Bm_XfersReadyForTx, &Bm_XfersReadyForTx);
		Bm_RunTest = Bm_RunTest_Write;
		break;
	case TEST_PCWRITE:
		Bm_InitXferArray(&Bm_XfersReadyForRx, &Bm_XfersReadyForRx);
		Bm_RunTest = Bm_RunTest_Read;
		break;
	default:
		Bm_InitXferArray(&Bm_XfersReadyForRx, &Bm_XfersReadyForRx);
		Bm_TestType = TEST_LOOP;
		Bm_RunTest = Bm_RunTest_Loop;
		break;
	}
	Bm_PrevTestType = Bm_TestType;
}

bool Bm_VendorRequestHandler(void)
{
	static uint8_t testType;

	if (Udd_setup_type() != USB_REQ_TYPE_VENDOR)
		return false;

	// handles test type select
	if (udd_g_ctrlreq.req.bRequest == PICFW_SET_TEST ||
	        udd_g_ctrlreq.req.bRequest == PICFW_GET_TEST)
	{
		if (!Udd_setup_is_in())
			return false;

		if (udd_g_ctrlreq.req.wLength != 1)
			return false;

		if (udd_g_ctrlreq.req.bRequest == PICFW_SET_TEST)
			Bm_TestType = (uint8_t)udd_g_ctrlreq.req.wValue;

		testType = Bm_TestType;
		udd_set_setup_payload(&testType, 1);
		return true;
	}

	// handles vendor buffer ctrl read/writes
	if (udd_g_ctrlreq.req.bRequest == PICFW_GET_VENDOR_BUFFER ||
	        udd_g_ctrlreq.req.bRequest == PICFW_SET_VENDOR_BUFFER)
	{
		if (udd_g_ctrlreq.req.wLength != sizeof(Bm_VendorBuffer))
			return false;

		if (Udd_setup_is_in() && udd_g_ctrlreq.req.bRequest != PICFW_GET_VENDOR_BUFFER)
			return false;
		if (Udd_setup_is_out() && udd_g_ctrlreq.req.bRequest != PICFW_SET_VENDOR_BUFFER)
			return false;

		udd_set_setup_payload(Bm_VendorBuffer, udd_g_ctrlreq.req.wLength);
		return true;
	}

	return false;
}
