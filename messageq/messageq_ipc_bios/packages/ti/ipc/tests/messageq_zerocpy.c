/*
 * Copyright (c) 2012-2013, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  ======== messageq_zerocpy.c ========
 *
 *  Test for messageq operating in multiple simultaneous threads.
 *
 */

#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/heaps/HeapBuf.h>
#include <ti/sysbios/knl/Clock.h>

#include <ti/ipc/MessageQ.h>
#include <ti/ipc/MultiProc.h>

#include <ti/grcm/RcmServer.h>
#include <ti/ipc/mm/MmType.h>
#include <ti/ipc/mm/MmServiceMgr.h>
#include <ti/sysbios/hal/Cache.h>

#define SLAVE_MESSAGEQNAME "SLAVE"
#define HOST_MESSAGEQNAME "HOST"
#define NUMOFTHREADSMAX 20

static unsigned int numThreads;
static unsigned int numMsgs;
static unsigned int numMsgSize;

static int gFinishedCounter = 0;

#define MessageQ_payload(m) ((void *)((char *)(m) + sizeof(MessageQ_MsgHeader)))

/* turn on/off printf's */
#define CHATTER 0

#define SERVICE_NAME "rpc_example"

/* MxServer skel function declarations */
static Int32 MxServer_skel_compute(UInt32 size, UInt32 *data);

/* MxServer skel function array */
static RcmServer_FxnDesc mxSkelAry[] = {
    { "MxServer_compute",       MxServer_skel_compute   }
};

/* MxServer skel function table */
static const RcmServer_FxnDescAry rpc_fxnTab = {
    (sizeof(mxSkelAry) / sizeof(mxSkelAry[0])),
    mxSkelAry
};

static MmType_FxnSig rpc_sigAry[] = {
    { "MxServer_compute", 2,
        {
            { MmType_Dir_Out, MmType_Param_S32, 1 }, /* return */
            { MmType_Dir_In,  MmType_PtrType(MmType_Param_VOID), 1 }
        }
    }
};

static MmType_FxnSigTab rpc_fxnSigTab = {
    MmType_NumElem(rpc_sigAry), rpc_sigAry
};

/* the server create parameters, must be in persistent memory */
static RcmServer_Params rpc_Params;

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 *  @brief      Operation is successful
 */
#define MxServer_S_SUCCESS (0)

/*!
 *  @brief      Operation failed
 */
#define MxServer_E_FAIL (-1)

/*!
 *  @brief      Compute structure
 */
typedef struct {
    int         size;
    uint32_t *  inBuf;
} MxServer_Compute;

/*!
 *  @brief      Sample function which has pointer parameter to
 *              a structure with two embedded pointers.
 */
int32_t MxServer_compute(MxServer_Compute *compute);


#if defined(__cplusplus)
}
#endif

/*
 *  ======== MxServer_compute ========
 */
int32_t MxServer_compute(MxServer_Compute *compute)
{
    int i;

    /* process inBuf ptr as data*/
    for (i = 0; i < compute->size; i++) {
        compute->inBuf[i] = 0xdeadbeef;
    }
    compute->inBuf[0] = (unsigned int)compute->inBuf;
    return(0);
}


Void RPC_SKEL_SrvDelNotification(Void)
{
    System_printf("RPC_SKEL_SrvDelNotification: Nothing to cleanup\n");
}

/*
 *  ======== MxServer_skel_compute ========
 */
Int32 MxServer_skel_compute(UInt32 size, UInt32 *data)
{
    MmType_Param *payload;
    MxServer_Compute *compute;
    Int32 result = 0;

    payload = (MmType_Param *)data;
    compute = (MxServer_Compute *)payload[0].data;

#if CHATTER
    System_printf("skel_compute: compute=0x%x\n", compute);
    System_printf("skel_compute: compute size=%d\n", (Int)payload[0].size);
    System_printf("skel_compute: size=0x%x\n", compute->size);
    System_printf("skel_compute: inBuf=0x%x\n", compute->inBuf);
    System_printf("skel_compute: inBuf[0]=0x%x\n", compute->inBuf[0]);
#endif

    /* invoke the implementation function */
    result = MxServer_compute(compute);

    return(result);
}

/*
 *  ======== register_MxServer ========
 *
 *  Bootstrap function, must be configured in BIOS.addUserStartupFunctions.
 */
void register_MxServer(void)
{
    Int status = MmServiceMgr_S_SUCCESS;
    Char mMServerName[20];

    System_printf("register_MxServer: -->\n");

    /* must initialize these modules before using them */
    RcmServer_init();
    MmServiceMgr_init();

    /* setup the server create params */
    RcmServer_Params_init(&rpc_Params);
    rpc_Params.priority = Thread_Priority_ABOVE_NORMAL;
    rpc_Params.stackSize = 0x1000;
    rpc_Params.fxns.length = rpc_fxnTab.length;
    rpc_Params.fxns.elem = rpc_fxnTab.elem;

    /* Construct an MMServiceMgr name adorned with core name: */
    System_sprintf(mMServerName, "%s_%d", SERVICE_NAME,
                   MultiProc_self());

    /* register an example service */
    status = MmServiceMgr_register(mMServerName, &rpc_Params, &rpc_fxnSigTab,
            RPC_SKEL_SrvDelNotification);

    if (status < 0) {
        System_printf("register_MxServer: MmServiceMgr_register failed, "
                "status=%d\n", status);
        status = -1;
        goto leave;
    }

leave:
    System_printf("register_MxServer: <--, status=%d\n", status);
}

Void dataTransactFxn(Void)
{
    MessageQ_Msg     getMsg;
    MessageQ_Handle  messageQ;
    MessageQ_QueueId remoteQueueId;
    Int              status,i;
    Char             localQueueName[64];
    Char             hostQueueName[64];
    //UInt32 startC;
    //UInt32 endC;
    UInt32 *handshake_params;
    UInt32 *boBufPayloadPtr, boBufPayloadSize;

    System_printf("dataTransactFxn:\n");

    System_sprintf(localQueueName, "%s_%d", SLAVE_MESSAGEQNAME, 0);
    System_sprintf(hostQueueName,  "%s_%d", HOST_MESSAGEQNAME,  0);

    /* Create a message queue. */
    messageQ = MessageQ_create(localQueueName, NULL);
    if (messageQ == NULL) {
        System_abort("MessageQ_create failed\n");
    }

    System_printf("loopbackFxn: created MessageQ: %s; QueueID: 0x%x\n",
        localQueueName, MessageQ_getQueueId(messageQ));
    System_printf("Start the main loop: %d\n", 0);

    //startC = Clock_getTicks();
        /* Get a message */
        status = MessageQ_get(messageQ, &getMsg, MessageQ_FOREVER);
        if (status != MessageQ_S_SUCCESS) {
           System_abort("This should not happen since timeout is forever\n");
        }
        remoteQueueId = MessageQ_getReplyQueue(getMsg);

        handshake_params = MessageQ_payload(getMsg);
        boBufPayloadPtr = (UInt32 *)handshake_params[0];
        boBufPayloadSize = handshake_params[1];
#if CHATTER
        System_printf("Recvd boBufPayloadPtr:0x%x, boBufPayloadSize= %d, boBufPayloadPtr[0]=0x%x, boBufPayloadPtr[boBufPayloadSize-4]=0x%x\n",
        boBufPayloadPtr,
        boBufPayloadSize,
        boBufPayloadPtr[0],
        boBufPayloadPtr[(boBufPayloadSize/sizeof(uint32_t))-1]);
#endif

       for (i = 0; i < (boBufPayloadSize/sizeof(uint32_t)); i++) {
              if(boBufPayloadPtr[i] != 0xbeefdead) {
              status = 1;
              System_printf("Data integrity failure!\n"
                "    Expected %s\n"
                "    Received 0x%x\n",
                "0xbeefdead", boBufPayloadPtr[i]);
              break;
            }
       }

      for (i = 0; i < (boBufPayloadSize/sizeof(uint32_t)); i++) {
       boBufPayloadPtr[i] = 0xdeadbeef;
      }
 
        /* test id of message received */
        if (MessageQ_getMsgId(getMsg) != 1) {
            System_abort("The id received is incorrect!\n");
        }

        status = MessageQ_put(remoteQueueId, getMsg);
        if (status != MessageQ_S_SUCCESS) {
           System_abort("MessageQ_put had a failure/error\n");
        }
    /*endC = Clock_getTicks();
        System_printf("Thread %d: %d iterations took %d ticks or %d usecs/msg\n",
                          arg0,numMsgs,
            endC - startC, ((endC - startC) * Clock_tickPeriod) / numMsgs);*/

    MessageQ_delete(&messageQ);
}
/*
 *  ======== loopbackFxn========
 *  Receive and return messages.
 *  Run at priority lower than tsk1Fxn above.
 *  Inputs:
 *     - arg0: number of the thread, appended to MessageQ host and slave names.
 */
Void loopbackFxn(UArg arg0, UArg arg1)
{
    MessageQ_Msg     getMsg;
    MessageQ_Handle  messageQ;
    MessageQ_QueueId remoteQueueId;
    Int              status;
    UInt16           msgId = 0;
    Char             localQueueName[64];
    Char             hostQueueName[64];
    UInt32 startC;
    UInt32 endC;
    UInt32 *handshake_params;
    UInt32 *boBufPayloadPtr, boBufPayloadSize;
#if CHATTER
    System_printf("Thread loopbackFxn: %d\n", arg0);
#endif
    System_sprintf(localQueueName, "%s_%d", SLAVE_MESSAGEQNAME, arg0);
    System_sprintf(hostQueueName,  "%s_%d", HOST_MESSAGEQNAME,  arg0);

    /* Create a message queue. */
    messageQ = MessageQ_create(localQueueName, NULL);
    if (messageQ == NULL) {
        System_abort("MessageQ_create failed\n");
    }

#if CHATTER
    System_printf("loopbackFxn: created MessageQ: %s; QueueID: 0x%x\n",
        localQueueName, MessageQ_getQueueId(messageQ));
    System_printf("Start the main loop: %d\n", arg0);
#endif
    startC = Clock_getTicks();
    while (msgId < numMsgs) {
        /* Get a message */
        status = MessageQ_get(messageQ, &getMsg, MessageQ_FOREVER);
        if (status != MessageQ_S_SUCCESS) {
           System_abort("This should not happen since timeout is forever\n");
        }
        remoteQueueId = MessageQ_getReplyQueue(getMsg);

        handshake_params = MessageQ_payload(getMsg);
        boBufPayloadPtr = (UInt32 *)handshake_params[0];
        boBufPayloadSize = handshake_params[1];
#if CHATTER
        System_printf("Recvd boBufPayloadPtr:0x%x, boBufPayloadSize= %d, boBufPayloadPtr[0]=0x%x, boBufPayloadPtr[boBufPayloadSize-4]=0x%x\n",
        boBufPayloadPtr,
        boBufPayloadSize,
        boBufPayloadPtr[0],
        boBufPayloadPtr[boBufPayloadSize-1]);
#endif
        /* test id of message received */
        if (MessageQ_getMsgId(getMsg) != msgId) {
            System_abort("The id received is incorrect!\n");
        }

        /* Send it back */
        status = MessageQ_put(remoteQueueId, getMsg);
        if (status != MessageQ_S_SUCCESS) {
           System_abort("MessageQ_put had a failure/error\n");
        }
        msgId++;
    }

    gFinishedCounter++;
    endC = Clock_getTicks();

    System_printf("Thread %d: %d iterations took %d ticks or %d usecs/msg\n",
                          arg0,numMsgs,
            endC - startC, ((endC - startC) * Clock_tickPeriod) / numMsgs);
    
    MessageQ_delete(&messageQ);
#if CHATTER
    System_printf("Test thread %d complete!\n", arg0);
#endif
}

Void tsk1Fxn(UArg arg0, UArg arg1)
{
    MessageQ_Msg msg;
    MessageQ_Handle  messageQ;
    MessageQ_QueueId remoteQueueId;
    Char             localQueueName[64];
    UInt16 procId;
    UInt32 *handshake_params;
    Task_Params params;
    Int i;

   /* Create handshake thread to correspond with host side test app: */
    while(1)
    {
	gFinishedCounter = 0;
	
        /* Construct a MessageQ name adorned with core name: */
	System_sprintf(localQueueName, "%s_%s", SLAVE_MESSAGEQNAME,
				   MultiProc_getName(MultiProc_self()));

	messageQ = MessageQ_create(localQueueName, NULL);
	if (messageQ == NULL) {
		System_abort("MessageQ_create failed\n");
	}

	System_printf("tsk1Fxn: created MessageQ: %s; QueueID: 0x%x\n",
	localQueueName, MessageQ_getQueueId(messageQ));
	
	/* handshake with host to get starting parameters */
	System_printf("Awaiting handshake sync message from host...\n");
	MessageQ_get(messageQ, &msg, MessageQ_FOREVER);

	handshake_params = MessageQ_payload(msg);
	numThreads = handshake_params[0];
	numMsgs = handshake_params[1];
	numMsgSize = handshake_params[2];

	remoteQueueId = MessageQ_getReplyQueue(msg);
	procId = MessageQ_getProcId(remoteQueueId);

	System_printf("Received hanshake msg from (procId:remoteQueueId): 0x%x:0x%x\n"
		"Threads:%d messages: %d payload: %d bytes\n",
		procId, remoteQueueId,
		numThreads,
		numMsgs,
		numMsgSize);

	MessageQ_put(remoteQueueId, msg);
	MessageQ_delete(&messageQ);
	
        /* Create N threads to correspond with host side N thread test app: */
        Task_Params_init(&params);
        params.priority = 3;
        for (i = 0; i < numThreads; i++) {
            params.arg0 = i;
            Task_create(loopbackFxn, &params, NULL);
        }
    
       while(gFinishedCounter < numThreads)
       {   
          Task_sleep(1000);    // sleep 1 second
       }

       dataTransactFxn();
    }
}

Void mmrpc_tsk1Fxn(UArg arg0, UArg arg1)
{
    register_MxServer();
}

/*
 *  ======== main ========
 */
Int main(Int argc, Char* argv[])
{
    Task_create(tsk1Fxn, NULL, NULL);
    Task_create(mmrpc_tsk1Fxn, NULL, NULL);
    
    BIOS_start();

    return (0);
 }
