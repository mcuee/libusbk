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

#define EP_TX_INDEX (0)
#define EP_RX_INDEX (1)

/** BMARK MACROS ****************************************************/
#define mSetWritePacketID(BufferPtr, BufferSize, FillCount, NextPacketKey)	\
	if ((BufferSize)>0)														\
	{																		\
		if (FillCount < ((BM_BANK_COUNT*2)*(BM_MAX_TRANSFER_SIZE/BM_EP_MAX_PACKET_SIZE)))	\
		{																	\
			FillCount++;													\
			Bm_FillBuffer(BufferPtr,BufferSize);							\
		}																	\
		BufferPtr[1]=NextPacketKey++;										\
	}

#define Bm_SubmitTransfer(ep,shortPacketEn,buffer,bufferLength,callbackFn) udd_ep_run(ep,shortPacketEn,buffer,bufferLength,callbackFn)
#define Bm_SubmitRead(ep,xferEL,callbackFn) Bm_SubmitTransfer(ep,false,(xferEL)->Buffer,BM_MAX_TRANSFER_SIZE,callbackFn)
#define Bm_SubmitWrite(ep,xferEL,callbackFn) Bm_SubmitTransfer(ep,false,(xferEL)->Buffer,(xferEL)->Transferred,callbackFn)

#define Bm_XferMove(fromList, toList,  moveEL) do {		\
	DL_DELETE(fromList, moveEL);						\
	DL_APPEND(toList, moveEL);							\
 } while(0)

#define Bm_IsNewTest() (Bm_TestType != Bm_PrevTestType)

typedef struct _BM_XFER_BUFFER
{
	COMPILER_WORD_ALIGNED uint8_t Buffer[BM_BUFFER_SIZE];
	uint16_t Transferred;
	
}BM_XFER_BUFFER;

typedef struct _BM_XFER_QUEUE_EL
{
	BM_XFER_BUFFER* Buffer;
	struct _BM_XFER_QUEUE_EL* next;
	struct _BM_XFER_QUEUE_EL* prev;
}BM_XFER_QUEUE_EL;

typedef struct _BM_XFER_EP
{
	BM_XFER_QUEUE_EL* Queued;
	volatile bool Busy;
	volatile uint8_t SofPeriod;
	
	udd_callback_trans_t OnXferComplete;
} BM_XFER_EP;

typedef struct _BM_TEST_CONTEXT
{
	BM_XFER_BUFFER Buffers[BM_BANK_COUNT*BM_EP_COUNT];
	BM_XFER_QUEUE_EL BufferElements[BM_BANK_COUNT*BM_EP_COUNT];

	BM_XFER_EP Rx;
	BM_XFER_EP Tx;

} BM_TEST_CONTEXT;

volatile Bm_RunTestDelegate Bm_SofEvent = NULL;
volatile Bm_RunTestDelegate Bm_RunTest = NULL;

static BM_TEST_CONTEXT bm;

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
static void Bm_InitXferBuffers(BM_XFER_QUEUE_EL** Bank1_AddListRef, BM_XFER_QUEUE_EL** Bank2_AddListRef);

static void Bm_Sof_Handler_HS(void);
static void Bm_Sof_Handler_FS(void);

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
	BM_XFER_QUEUE_EL* moveEL = bm.Tx.Queued;

	if (++Bm_Led_Counter==0)
		LED1_TGL();

	if (status)
	{
		
		bm.Tx.Busy = false;
		return;
	}

	bm.Tx.Queued->Buffer->Transferred=nb_transfered;
	Bm_XferMove(bm.Tx.Queued, bm.Rx.Queued, moveEL);
	
	bm.Tx.Busy = false;
}

static void Bm_XferLoopCompleteRx(udd_ep_status_t status, iram_size_t nb_transfered)
{
	BM_XFER_QUEUE_EL* moveEL = bm.Rx.Queued;
	
	if (++Bm_Led_Counter==0)
		LED1_TGL();

	if (status)
	{
		bm.Rx.Busy = false;
		return;
	}

	moveEL->Buffer->Transferred=nb_transfered;
	Bm_XferMove(bm.Rx.Queued, bm.Tx.Queued, moveEL);
	
	bm.Rx.Busy = false;
}

static void Bm_XferCompleteRx(udd_ep_status_t status, iram_size_t nb_transfered)
{
	BM_XFER_QUEUE_EL* moveEL = bm.Rx.Queued;

	if (++Bm_Led_Counter==0)
		LED1_TGL();

	if (status)
	{
		bm.Rx.Busy = false;
		return;
	}

	bm.Rx.Queued->Buffer->Transferred=nb_transfered;
	Bm_XferMove(bm.Rx.Queued, bm.Rx.Queued, moveEL);
	
	bm.Rx.Busy = false;
}

static void Bm_XferCompleteTx(udd_ep_status_t status, iram_size_t nb_transfered)
{
	BM_XFER_QUEUE_EL* moveEL = bm.Tx.Queued;

	if (++Bm_Led_Counter==0)
		LED1_TGL();

	if (status)
	{
		bm.Tx.Busy = false;
		return;
	}

	bm.Tx.Queued->Buffer->Transferred=nb_transfered;
	Bm_XferMove(bm.Tx.Queued, bm.Tx.Queued, moveEL);
	
	bm.Tx.Busy = false;
}

static void Bm_RunTest_Loop(void)
{
	if (!bm.Rx.Busy && bm.Rx.Queued)
	{
		#if (BM_EP_TYPE==EP_TYPE_ISO)
		if (!bm.Rx.SofPeriod)
		#endif
		bm.Rx.Busy = Bm_SubmitRead(BM_EP_RX, bm.Rx.Queued->Buffer, Bm_XferLoopCompleteRx);
	}
	if (!bm.Tx.Busy &&  bm.Tx.Queued)
	{
		#if (BM_EP_TYPE==EP_TYPE_ISO)
		if (!bm.Tx.SofPeriod)
		#endif
		bm.Tx.Busy = Bm_SubmitWrite(BM_EP_TX, bm.Tx.Queued->Buffer, Bm_XferLoopCompleteTx);
	}
}

static void Bm_RunTest_Read(void)
{
	if (!bm.Rx.Busy && bm.Rx.Queued)
	{
		#if (BM_EP_TYPE==EP_TYPE_ISO)
		if (!bm.Rx.SofPeriod)
		#endif
		bm.Rx.Busy = Bm_SubmitRead(BM_EP_RX, bm.Rx.Queued->Buffer, Bm_XferCompleteRx);
	}
}

void Bm_InitWritePackets(BM_XFER_QUEUE_EL* QueueEL);
void Bm_InitWritePackets(BM_XFER_QUEUE_EL* QueueEL)
{
	int i;
	for(i=0; i < BM_MAX_TRANSFER_SIZE; i+=BM_EP_MAX_PACKET_SIZE)
	{
		mSetWritePacketID((&QueueEL->Buffer->Buffer[i]), BM_EP_MAX_PACKET_SIZE, Bm_FillCount, Bm_NextPacketKey);
	}
}

static void Bm_RunTest_Write(void)
{

	if (!bm.Tx.Busy && bm.Tx.Queued)
	{
		#if (BM_EP_TYPE==EP_TYPE_ISO)
		if (!bm.Tx.SofPeriod) {
		#endif
		
		Bm_InitWritePackets(bm.Tx.Queued);
		bm.Tx.Busy = Bm_SubmitWrite(BM_EP_TX, bm.Tx.Queued->Buffer, Bm_XferCompleteTx);
		
		#if (BM_EP_TYPE==EP_TYPE_ISO)
		}
		#endif
	}
}

void RunApplication(void)
{
	Bm_Init();

	while(true)
	{
		if (Bm_IsNewTest())
		{
			if (bm.Rx.Busy)
			{
				udd_ep_abort(BM_EP_RX);
				continue;
			}
			else if (bm.Tx.Busy)
			{
				udd_ep_abort(BM_EP_TX);
				continue;
			}
			else
			{
				Bm_Init();
				continue;
			}
		}
		
		#if (BM_EP_TYPE != EP_TYPE_ISO)
			if (Bm_RunTest) Bm_RunTest();
		#endif
	}
}

static void Bm_InitXferBuffers(BM_XFER_QUEUE_EL** Bank1_AddListRef, BM_XFER_QUEUE_EL** Bank2_AddListRef)
{
	int i,j;

	for(i = 0; i < (BM_BANK_COUNT); i++)
	{
		for(j = 0; j < (BM_EP_COUNT); j++)
		{
			bm.Buffers[i*j].Transferred=BM_MAX_TRANSFER_SIZE;
			bm.BufferElements[i*j].Buffer=&bm.Buffers[i*j];
			bm.BufferElements[i*j].next=NULL;
			bm.BufferElements[i*j].prev=NULL;
			Bm_InitWritePackets(&bm.BufferElements[i*j]);
			if (j & (BM_EP_COUNT-1))
			{
				DL_APPEND(*Bank2_AddListRef, &bm.BufferElements[i*j]);
			}
			else
			{
				DL_APPEND(*Bank1_AddListRef, &bm.BufferElements[i*j]);
			}
		}
	}	
}

static void Bm_Init(void)
{
	Bm_FillCount = 0;
	Bm_Led_Counter = 0;
	LED1_OFF();
	
	memset(&bm.Rx,0,sizeof(bm.Rx));
	memset(&bm.Tx,0,sizeof(bm.Tx));
	
#if (BM_EP_TYPE==EP_TYPE_ISO)
	Bm_SofEvent = udd_is_high_speed() ? Bm_Sof_Handler_HS : Bm_Sof_Handler_FS;
	#ifdef BM_MANAGE_SOF_PERIOD
		bm.Rx.SofPeriod = BM_EP_POLLCOUNT-1;
		bm.Tx.SofPeriod = BM_EP_POLLCOUNT-1;
	#endif
#endif
	
	
	switch (Bm_TestType)
	{
	case TEST_NONE:
		Bm_InitXferBuffers(&bm.Rx.Queued, &bm.Rx.Queued);
		Bm_RunTest = NULL;
		break;
	case TEST_PCREAD:
		Bm_InitXferBuffers(&bm.Tx.Queued, &bm.Tx.Queued);
		Bm_RunTest = Bm_RunTest_Write;
		break;
	case TEST_PCWRITE:
		Bm_InitXferBuffers(&bm.Rx.Queued, &bm.Rx.Queued);
		Bm_RunTest = Bm_RunTest_Read;
		break;
	default:
		Bm_InitXferBuffers(&bm.Rx.Queued, &bm.Rx.Queued);
		Bm_TestType = TEST_LOOP;
		Bm_RunTest = Bm_RunTest_Loop;
		break;
	}
	
	Bm_NextPacketKey = 0;

	Bm_PrevTestType = Bm_TestType;
}
#if (BM_EP_TYPE==EP_TYPE_ISO)

static void Bm_Sof_Handler_HS(void)
{
	Bm_Sof_Handler_FS();
}

static void Bm_Sof_Handler_FS(void)
{
	if (!Bm_RunTest) return;
	#ifdef BM_MANAGE_SOF_PERIOD
	if ((++bm.Rx.SofPeriod) >= BM_EP_POLLCOUNT) bm.Rx.SofPeriod = 0;
	if ((++bm.Tx.SofPeriod) >= BM_EP_POLLCOUNT) bm.Tx.SofPeriod = 0;
	#endif
	Bm_RunTest();
}

#endif

bool Bm_Vendor_Handler(void)
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

