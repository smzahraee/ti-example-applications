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
/* =============================================================================
 *  @file   MessageQZCpy.c
 *
 *  @brief  Sample application for MessageQ module between MPU and Remote Proc
 *
 *  ============================================================================
 */

/* Standard headers */
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>

/* IPC Headers */
#include <ti/ipc/Std.h>
#include <ti/ipc/Ipc.h>
#include <ti/ipc/MessageQ.h>
#include <ti/ipc/mm/MmRpc.h>
#include <ti/ipc/MultiProc.h>

/* libDRM Headers */
#include <omap/omap_drm.h>
#include <libdrm/omap_drmif.h>

/* App defines: Must match on remote proc side: */
#define HEAPID                      0u
#define A15_HANDSHAKE_MQNAME     "A15_RECEIVER_HANDSHAKE"
#define A15_MESSAGEQNAME            "A15"
#define M4_MESSAGEQNAME             "M4"
#define SLAVE_MESSAGEQNAME	"SLAVE"
#define HOST_MESSAGEQNAME	"HOST"


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */

/** [TODO] The below could change as per the platform used **/
/*OMAP5 remoteproc0:DSP:proc id 2*/
#define COREPROC0					2u
/*OMAP5 remoteproc1:IPU:proc id 1*/
#define COREPROC1					1u

/*Before increasing the below value handle the thrDirection variable capacity in SyncMsg 
 and check the same on the BIOS side code too*/
#define  MAX_NUM_THREADS  100

#define MAX_INPUT_STR_SIZE                      (128)
/** ============================================================================
 *  Globals
 *  ============================================================================
 */

/*!
 *  @brief   structure
 */
typedef struct {
    int         size;  /* Size of the shared region */
    uint32_t *  inBuf; /* This is the shared region space address */
} Mx_Compute;


typedef struct RemoteBufSyncMsg {
    MessageQ_MsgHeader header;
    unsigned int boBufPayloadPtr; /*Shared Region pointer address to be exchanged using MessageQ*/
    unsigned int boBufPayloadSize; /*Shared Region pointer address  Size*/
} RemoteBufSyncMsg;


typedef struct SyncMsg {
    MessageQ_MsgHeader header; /* This is a mandatory requirement for MessageQ_alloc */
    unsigned int boBufPayloadPtr; /*Shared Region pointer address to be exchanged using MessageQ*/
    unsigned int boBufPayloadSize; /*Shared Region pointer address  Size*/
    unsigned int numThreads; /* Indicates the number of threads to the other core: Handshake */
    unsigned int numMessages; /* Indicates the number of messages: Handshake */
    unsigned int numWaitTime; /* Indicates the sleep/wait between send messages: Handshake */
    unsigned int payloadSize; /* Indicates the size of the payload which will be carried by each message: Handshake */
    unsigned int procId; /* Indicates the size of the payload which will be carried by each message: Handshake */
    unsigned int thrDirection; /* Used a double to use each bit field for a thread. hence restriction of 64 threads*/
} SyncMsg;

enum ThreadDirection {
	unidirectional_send = 0,
	unidirectional_recv,
	bidirectional
};

typedef struct thread_info {    /* Used as argument to thread_start() */
    pthread_t thread_id;        /* ID returned by pthread_create() */
    unsigned int thread_num;       /* Application-defined thread # */
    int config_param; /* This will define the communication direction of this thread.*/
    unsigned int numMessages; /* Indicates the number of messages: Handshake */
    unsigned int numWaitTime; /* Indicates the sleep/wait between send messages: Handshake */
    unsigned int payloadSize; /* Indicates the size of the payload which will be carried by each message: Handshake */
    unsigned int procId; /* Indicates the procid for this thread: Handshake */
    unsigned int boBufPayloadPtr; /*Shared Region pointer address per thread*/
    unsigned int boBufPayloadSize; /*Shared Region Size for this index of address*/
}thrConfigs;

typedef struct stProPerfConfig {
	int procId;
	int numThreads;
	int bufferPtr;
	int bufferPtrRemoteAddr;
	int bufferPtrSize;
	int totalReqPayloadSize;
	thrConfigs *pThrConfig;
	Mx_Compute *compute;
	SyncMsg stSyncMsg;
}stProPerfConfig;


static void * pingThreadFxn_uni_send(void *arg);
static void * pingThreadFxn_uni_recv(void *arg);

struct omap_device *dev = NULL;
struct omap_bo *compute_bo = NULL;
struct omap_bo *inBuf_bo = NULL;
int drmFd = 0;

/* hande used for remote communication */
static MmRpc_Handle Mx_rpcIpu = NULL;

/* params to This allows the remote processor to maintain persistent references
    to the buffers across multiple calls to MmRcp_call */
MmRpc_BufDesc desc[1];
int num = 0;

/* static function indicies: This should map to  */
#define Mx_Fxn_compute  (0x80000000 | 0)

#define Mx_OFFSET(base, member) ((uint_t)(member) - (uint_t)(base))
/*This is the name of the service opened on host as well as remote size, should be same*/
#define SERVICE_NAME    "rpc_example"

/** ============================================================================
 *  Functions
 *  ============================================================================
 */

/*
 *  ======== Mx_initialize ========
 */
int Mx_initialize(UInt16 procId)
{
    int status;
    MmRpc_Params args;
    Char mmServerName[20];

    /* create remote server instance */
    MmRpc_Params_init(&args);

    /* Construct an MmRpc server name adorned with core name: */
    sprintf(mmServerName, "%s_%d", SERVICE_NAME, procId);

    status = MmRpc_create(mmServerName, &args, &Mx_rpcIpu);

    if (status < 0) {
        printf("Error: MmRpc_create of %s failed\n", mmServerName);
        status = -1;
    }
    else {
        status = 0;
    }

    return(status);
}

/*
 *  ======== Mx_finalize ========
 */
void Mx_finalize(void)
{
    /* delete remote server instance */
    if (Mx_rpcIpu != NULL) {
        MmRpc_delete(&Mx_rpcIpu);
    }
}

/*
 *  ======== Mx_compute_Linux ========
 */
int32_t Mx_compute_Linux(Mx_Compute *compute, int fd, int fdIn)
{
    MmRpc_FxnCtx *fxnCtx;
    MmRpc_Xlt xltAry[1];
    int32_t fxnRet;
    char send_buf[512] = {0};
    int status;

    /* make the output buffer persistent */
    desc[0].handle = fdIn;
    num = 1;
    status = MmRpc_use(Mx_rpcIpu, MmRpc_BufType_Handle, num, desc);

    if (status < 0) {
        printf("Error: MmRpc_use failed\n");
        num = 0;
        fxnRet = -1;
    }

    /* marshall function arguments into the send buffer */
    fxnCtx = (MmRpc_FxnCtx *)send_buf;

    fxnCtx->fxn_id = Mx_Fxn_compute;
    fxnCtx->num_params = 1;
    fxnCtx->params[0].type = MmRpc_ParamType_Ptr;
    fxnCtx->params[0].param.ptr.size = sizeof(Mx_Compute);
    fxnCtx->params[0].param.ptr.addr = (size_t)compute;
    fxnCtx->params[0].param.ptr.handle = fd;

    fxnCtx->num_xlts = 1;
    fxnCtx->xltAry = xltAry;

    fxnCtx->xltAry[0].index = 0;
    fxnCtx->xltAry[0].offset = MmRpc_OFFSET(compute, &compute->inBuf);
    fxnCtx->xltAry[0].base = (size_t)compute->inBuf;
    fxnCtx->xltAry[0].handle = fdIn;

    /* invoke the remote function call */
    status = MmRpc_call(Mx_rpcIpu, fxnCtx, &fxnRet);

    if (status < 0) {
        printf("mmrpc_test: Error: MmRpc_call failed\n");
        fxnRet = -1;
    }

    return(fxnRet);
}

/*
 *  ======== Mx_compute_Release ========
 */
int32_t Mx_compute_Release()
{
    int status = 0;
    /* make the output buffer persistent */
    if (num > 0) {
        status = MmRpc_release(Mx_rpcIpu, MmRpc_BufType_Handle, num, desc);
        return status;
    }
    return 0;
}

long diff(struct timespec dstart, struct timespec dend)
{
    struct timespec temp;

    if ((dend.tv_nsec - dstart.tv_nsec) < 0) {
        temp.tv_sec = dend.tv_sec - dstart.tv_sec-1;
        temp.tv_nsec = 1000000000UL + dend.tv_nsec - dstart.tv_nsec;
    } else {
        temp.tv_sec = dend.tv_sec - dstart.tv_sec;
        temp.tv_nsec = dend.tv_nsec - dstart.tv_nsec;
    }

    return (temp.tv_sec * 1000000UL + temp.tv_nsec / 1000);
}

static int callCompute_Linux(stProPerfConfig *stExpConfig)
{
    uint32_t * inBufPtr = NULL;
    int status = 0;
    int size;
    int i;
    int32_t ret;

    /* On Linux, use omapdrm driver to get a Tiler buffer for shared memory */
    drmFd = drmOpen("omapdrm", NULL);

    if (drmFd < 0) {
        fprintf(stderr, "could not open omapdrm device: %d: %s\n",
                         errno, strerror(errno));
        return -1;
    }

    dev = omap_device_new(drmFd);
    if (!dev) {
        fprintf(stderr, "could not get device from fd\n");
        return -1;
    }

    /* allocate a compute structure in shared memory and get a pointer */
    compute_bo = omap_bo_new(dev, sizeof(Mx_Compute), OMAP_BO_WC);
    if (compute_bo) {
        stExpConfig->compute = (Mx_Compute *)omap_bo_map(compute_bo);
    }
    else {
        fprintf(stderr, "failed to allocate omap_bo\n");
    }

    if (stExpConfig->compute == NULL) {
        fprintf(stderr, "failed to map omap_bo to user space\n");
        return -1;
    }

    /* initialize compute structure */
    stExpConfig->compute->size = (stExpConfig->totalReqPayloadSize/(sizeof(uint32_t))); /* [TODO]Assumption: user_payload_size/sizeof(uint32_t) */
    stExpConfig->compute->inBuf = NULL;

    /* allocate an input buffer in shared memory */
    size = stExpConfig->compute->size * sizeof(uint32_t);
    inBuf_bo = omap_bo_new(dev, size, OMAP_BO_WC);
    if (inBuf_bo) {
        inBufPtr = (uint32_t *)omap_bo_map(inBuf_bo);
    }
    else {
        fprintf(stderr, "failed to allocate inBuf_bo\n");
    }

    if (inBufPtr == NULL) {
        printf("Error: inBufPtr == NULL\n");
        status = -1;
        return -1;
    }

    /* fill input buffer with seed value */
    for (i = 0; i < stExpConfig->compute->size; i++) {
        inBufPtr[i] = 0xbeefdead;
    }

    stExpConfig->compute->inBuf = (uint32_t *)inBufPtr;

    /* print some debug info */
#if PRINT_DEBUG    
    printf("compute->size=0x%x\n", stExpConfig->compute->size);
    printf("compute->inBuf=0x%x\n", (unsigned int)stExpConfig->compute->inBuf);
#endif
    /* process the buffer */
    ret = Mx_compute_Linux(stExpConfig->compute, omap_bo_dmabuf(compute_bo),
                                    omap_bo_dmabuf(inBuf_bo));

    if (ret < 0) {
        status = -1;
        printf("Error: Mx_Compute() failed\n");
        return -1;
    }
#if PRINT_DEBUG
    printf("After Mx_compute_Linux mmrpc call \n");
    printf("compute->inBuf=0x%x\n", (unsigned int)stExpConfig->compute->inBuf);
    printf("compute->inBuf[0]=0x%x\n",
            (unsigned int)stExpConfig->compute->inBuf[0]);
#endif
    /*This checks Data Transaction at mmrpc level;check the output buffer */
   /* starting at index 1 because 0th index contains translated address of inBufPtr*/
    for (i = 1; i < stExpConfig->compute->size; i++) {
        if (inBufPtr[i] != 0xdeadbeef) {
            status = 1;
            printf("Error: incorrect InBuf\n");
            break;
        }
    }

    stExpConfig->bufferPtr = (unsigned int)stExpConfig->compute->inBuf;
    stExpConfig->bufferPtrRemoteAddr = (unsigned int)stExpConfig->compute->inBuf[0];
    stExpConfig->bufferPtrSize = stExpConfig->compute->size * sizeof(uint32_t);

    return (status);
}

Int MessageQApp_handshake(stProPerfConfig stExpConfig)
{
    Int32                    i = 0, status = 0;
    MessageQ_Msg             msg = NULL;
    MessageQ_Params          msgParams;
    MessageQ_QueueId         queueId = MessageQ_INVALIDMESSAGEQ;
    MessageQ_Handle          msgqHandle;
    char                     remoteQueueName[64];
    int tempOffsetCtr = 0;

#if PRINT_DEBUG
    printf("Entered MessageQApp_handshake\n");
#endif
    
    /* Create the local Message Queue for receiving. */
    MessageQ_Params_init(&msgParams);
    msgqHandle = MessageQ_create(A15_HANDSHAKE_MQNAME, &msgParams);
    if (msgqHandle == NULL) {
        printf("Error in MessageQ_create\n");
        goto exit;
    }
#if PRINT_DEBUG
    else {
        printf("%s,Local MessageQId: 0x%x\n",A15_HANDSHAKE_MQNAME, MessageQ_getQueueId(msgqHandle));
    }
#endif
    sprintf(remoteQueueName, "%s_%s", SLAVE_MESSAGEQNAME,
             MultiProc_getName(stExpConfig.procId));
    /* Poll until remote side has it's messageQ created before we send: */
    do {
        status = MessageQ_open(remoteQueueName, &queueId);
        sleep (1);
    } while (status == MessageQ_E_NOTFOUND);

    if (status < 0) {
        printf("Error in MessageQ_open [%d]\n", status);
        goto cleanup;
    }
#if PRINT_DEBUG
    else {
        printf("%s Remote queueId  [0x%x]\n",remoteQueueName, queueId);
    }
#endif

    msg = MessageQ_alloc(HEAPID, sizeof(SyncMsg));
    if (msg == NULL) {
        printf("Error in MessageQ_alloc\n");
        goto close_cleanup;
    }
    
    /* handshake with remote to set the number of loops */
    MessageQ_setReplyQueue(msgqHandle, msg);

    /* Ensuring first I handshake no. of threads/tasks so that a loop can be created at bios side*/
    ((SyncMsg *)msg)->numThreads = stExpConfig.numThreads;
    status = MessageQ_put(queueId, msg);
    if (status < 0) {
        printf("MessageQ_put handshake failed [%d]\n", status);
        goto free_cleanup;
    }

    status = MessageQ_get(msgqHandle, &msg, MessageQ_FOREVER);
    if (status < 0) {
            printf ("Error in MessageQ_get [0x%x]\n", status);
        }
   else {
#if PRINT_DEBUG
    printf("Exchanged thread count handshake with remote processor %s...\n",
           MultiProc_getName(stExpConfig.procId));
#endif
	}

	for (i = 0; i < stExpConfig.numThreads; i++) 
	{
		((SyncMsg *)msg)->boBufPayloadPtr = stExpConfig.bufferPtrRemoteAddr + tempOffsetCtr;
		((SyncMsg *)msg)->boBufPayloadSize = stExpConfig.pThrConfig[i].payloadSize;
		((SyncMsg *)msg)->numMessages = stExpConfig.pThrConfig[i].numMessages;
		((SyncMsg *)msg)->numWaitTime = stExpConfig.pThrConfig[i].numWaitTime;
		((SyncMsg *)msg)->payloadSize = stExpConfig.pThrConfig[i].payloadSize;
		((SyncMsg *)msg)->procId = stExpConfig.pThrConfig[i].procId; 
		((SyncMsg *)msg)->thrDirection = stExpConfig.pThrConfig[i].config_param;
      		tempOffsetCtr += stExpConfig.pThrConfig[i].payloadSize;
		status = MessageQ_put(queueId, msg);
		if (status < 0) {
			printf("MessageQ_put handshake failed [%d]\n", status);
			goto free_cleanup;
		}

		status = MessageQ_get(msgqHandle, &msg, MessageQ_FOREVER);
	        if (status < 0) {
			printf ("Error in MessageQ_get [0x%x]\n", status);
			break;
	        }
  	}

#if PRINT_DEBUG
    printf("Exchanged handshake with remote processor %s...\n",
           MultiProc_getName(stExpConfig.procId));
#endif

free_cleanup:
    MessageQ_free(msg);
close_cleanup:
    MessageQ_close(&queueId);
cleanup:
    /* Clean-up */
    status = MessageQ_delete(&msgqHandle);
    if (status < 0) {
        printf("Error in MessageQ_delete [%d]\n", status);
    }

exit:
#if PRINT_DEBUG
    printf("Leaving MessageQApp_handshake\n\n");
#endif
    return (status);
}

static Void * pingThreadFxn_uni_send(void *arg)
{
    struct thread_info pingThreadFxnData = *(struct thread_info *)arg;   
    Int                      threadNum = 0;
    Int32                    status     = 0;
    MessageQ_Msg             msg        = NULL;
    UInt16                   i;
    MessageQ_QueueId         queueId = MessageQ_INVALIDMESSAGEQ;

    char             remoteQueueName[64];

    threadNum = pingThreadFxnData.thread_num;
    sprintf(remoteQueueName, "%s_RECV_MQ_%d", M4_MESSAGEQNAME, threadNum );
    
    //printf("pingThreadFxn This thread sending to num: %d, Name: %s\n", threadNum,remoteQueueName);

    /* Poll until remote side has it's messageQ created before we send: */
    do {
        status = MessageQ_open (remoteQueueName, &queueId);
        sleep (1);
    } while (status == MessageQ_E_NOTFOUND);
    if (status < 0) {
        printf ("Error in MessageQ_open [0x%x]\n", status);
	return ((void *)status);
    }
#if PRINT_DEBUG
    else {
        printf ("thread: %d, Remote queue: %s, QId: 0x%x\n",
                 threadNum, remoteQueueName, queueId);
    }
    printf ("\nthread: %d: Exchanging messages with remote processor...\n",
            threadNum);
#endif
    for (i = 0 ; i < pingThreadFxnData.numMessages ; i++) {
        /* Allocate message. */
        msg = MessageQ_alloc (HEAPID, sizeof(RemoteBufSyncMsg));
        if (msg == NULL) {
            printf ("Error in MessageQ_alloc\n");
            break;
        }

        MessageQ_setMsgId (msg, i);

        /* Have the remote proc reply to this message queue */
      ((RemoteBufSyncMsg *)msg)->boBufPayloadPtr = pingThreadFxnData.boBufPayloadPtr;
      ((RemoteBufSyncMsg *)msg)->boBufPayloadSize = pingThreadFxnData.boBufPayloadSize;

        status = MessageQ_put (queueId, msg);
        if (status < 0) {
            printf ("Error in MessageQ_put [0x%x]\n", status);
            break;
        }
        usleep (pingThreadFxnData.numWaitTime); /*Sleep for 2.5ms*/
    }
    
    MessageQ_close (&queueId);
    return ((void *)status);
}

static Void * pingThreadFxn_uni_recv(void *arg)
{
    struct thread_info pingThreadFxnData = *(struct thread_info *)arg;   
    Int                      threadNum = 0;
    Int32                    status     = 0;
    MessageQ_Msg             msg        = NULL;
    MessageQ_Params          msgParams;
    UInt16                   i;
    MessageQ_Handle          handle;

    char             hostQueueName[64];

    threadNum = pingThreadFxnData.thread_num;
    sprintf(hostQueueName,   "%s_RECV_MQ_%d", A15_MESSAGEQNAME,  threadNum );

    //printf("pingThreadFxn_uni_recv This thread num: %d from Name:%s  \n", threadNum,hostQueueName);
    
    /* Create the local Message Queue for receiving. */
    MessageQ_Params_init (&msgParams);
    handle = MessageQ_create (hostQueueName, &msgParams);
    if (handle == NULL) {
        printf ("Error in MessageQ_create\n");
        goto exit;
    }
#if PRINT_DEBUG
    else {
        printf ("thread: %d, Local Message: %s, QId: 0x%x\n",
            threadNum, hostQueueName, MessageQ_getQueueId(handle));
    }
#endif
    for (i = 0 ; i < pingThreadFxnData.numMessages ; i++) {
        status = MessageQ_get(handle, &msg, MessageQ_FOREVER);
        if (status < 0) {
            printf ("Error in MessageQ_get [0x%x]\n", status);
            break;
        }
        else {
            /* Validate the returned message. */
            if ((msg != NULL) && (MessageQ_getMsgId (msg) != i)) {
                printf ("Data integrity failure!\n"
                        "    Expected %d\n"
                        "    Received %d\n",
                        i, MessageQ_getMsgId (msg));
                break;
            }
            status = MessageQ_free (msg);
       }
    }

    /* Clean-up */
    status = MessageQ_delete (&handle);
    if (status < 0) {
        printf ("Error in MessageQ_delete [0x%x]\n", status);
    }

exit:
    return ((void *)status);
}


Int dataTransactFxn(stProPerfConfig stExpConfig)
{
    Int32                    status     = 0;
    MessageQ_Msg             msg1        = NULL;
    MessageQ_Params          msgParams;
    Int32                    i;
    MessageQ_Handle          handle;
    MessageQ_QueueId         queueId = MessageQ_INVALIDMESSAGEQ;

    char             remoteQueueName[64];
    char             hostQueueName[64];

    sprintf(remoteQueueName, "%s_%d", SLAVE_MESSAGEQNAME,0);
    sprintf(hostQueueName,   "%s_%d", HOST_MESSAGEQNAME,0);

    printf("dataTransactFxn: This function is a test to showcase data transaction\n");

    /* Create the local Message Queue for receiving. */
    MessageQ_Params_init (&msgParams);
    handle = MessageQ_create (hostQueueName, &msgParams);
    if (handle == NULL) {
        printf ("Error in MessageQ_create\n");
        goto exit;
    }
    /* Poll until remote side has it's messageQ created before we send: */
    do {
        status = MessageQ_open (remoteQueueName, &queueId);
        sleep (1);
    } while (status == MessageQ_E_NOTFOUND);
    if (status < 0) {
        printf ("Error in MessageQ_open [0x%x]\n", status);
        goto cleanup;
    }

    /* Allocate message. */
    msg1 = MessageQ_alloc (HEAPID, sizeof(RemoteBufSyncMsg));
    if (msg1 == NULL) {
        printf ("Error in MessageQ_alloc\n");
        goto cleanup;
    }

    MessageQ_setMsgId (msg1, 1);/* Set a random number to associate with this message*/

    for (i = 0; i < (stExpConfig.bufferPtrSize/sizeof(uint32_t)); i++) {
        ((unsigned int *)stExpConfig.bufferPtr)[i] = 0xbeefdead;
    }

    /* Have the rem te proc reply to this message queue */
    MessageQ_setReplyQueue (handle, msg1);
    ((RemoteBufSyncMsg *)msg1)->boBufPayloadPtr = stExpConfig.bufferPtrRemoteAddr;
    ((RemoteBufSyncMsg *)msg1)->boBufPayloadSize = stExpConfig.bufferPtrSize;

    status = MessageQ_put (queueId, msg1);
    if (status < 0) {
        printf ("Error in MessageQ_put [0x%x]\n", status);
        goto cleanup;
    }

    status = MessageQ_get(handle, &msg1, MessageQ_FOREVER);
    if (status < 0) {
       printf ("Error in MessageQ_get [0x%x]\n", status);
       goto cleanup;
    }
    else {
       for (i = 0; i < (stExpConfig.bufferPtrSize/sizeof(uint32_t)); i++) {
              if (((unsigned int *)stExpConfig.bufferPtr)[i] != 0xdeadbeef) {
              status = 1;
              printf ("Data integrity failure!\n"
                "    Expected %s\n"
                "    Received 0x%x\n",
                "0xdeadbeef", ((unsigned int *)stExpConfig.bufferPtr)[i]);
              break;
        }
    }
    if(status != 1)
        printf("%s:Test Pass\n",__func__);
    /* Validate the returned message. */
    status = MessageQ_free (msg1);
   }

    MessageQ_close (&queueId);

cleanup:
    /* Clean-up */
    status = MessageQ_delete (&handle);
    if (status < 0) {
        printf ("Error in MessageQ_delete [0x%x]\n", status);
    }

exit:
    return status;
}

#define printd(fmt, ...) \
	do { if (debug) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

int debug=0;

struct config
{
	int direction;
	int interval;
	int msgSize;
	int msgCount;
	int procID;
};

struct config xyz[MAX_NUM_THREADS];/* [TODO]: Remove Hardcode, Assuming for the time being max threads 100*/
int linecount = 0;

char keylist[5][50] = {
	"direction",
	"msgSize",
	"msgCount",
	"interval",
	"procID"
};

int validatekey(char *ptr)
{
	int i;
	for(i=0; i<5; i++)
		if(strcmp(ptr, keylist[i]) == 0)
			return 0;

	return 1;
}

/*
direction: 0-Send, 1-Recv
msgSize: in bytes(Should be 4 bytes aligned)
msgCount: number of messages to be exchanged
interval: in microsecs(us)
procID: core, Hardcoded to IPU2 for the time being

Sample contents of cfg file:");

#Thread 1
direction=0, msgSize=6400, msgCount=400, interval=2500, procID=1
#Thread 2
direction=1, msgSize=640, msgCount=800, interval=2500, procID=1

*/
#define PRINTEXIT {printusage(); exit(1);}

void printusage() {
    printf("\n --------------------------------------------");
    printf("\n INCORRECT USAGE !");
    printf("\n --------------------------------------------");
    printf("\n ./MessageQZCpy -f <CFG file path>");
    printf("\n");
    printf("\n -f : MANDATORY  : Configuration file which contains use-case configurations");
    printf("\n");
    printf("\n Sample contents of cfg file:");
    printf("\n #Thread 1");
    printf("\n direction=0, msgSize=6400, msgCount=400, interval=2500, procID=1");
    printf("\n #Thread 2");
    printf("\n direction=1, msgSize=640, msgCount=800, interval=2500, procID=1");
    printf("\n");
}


void add_key_value(char *key, int value)
{
    if((linecount + 1) < MAX_NUM_THREADS)
    {
	printd("%s", "Inside add_key_value\n");
	
	if(strcmp(key, "direction") == 0)
		xyz[linecount].direction = value;
	else if(strcmp(key, "interval") == 0)
		xyz[linecount].interval = value;
	else if(strcmp(key, "msgSize") == 0)
		xyz[linecount].msgSize = value;
	else if(strcmp(key, "msgCount") == 0)
		xyz[linecount].msgCount = value;
	else if(strcmp(key, "procID") == 0)
		xyz[linecount].procID = value;
	else
		printd("%s", "********** UNKNOWN**********");
    }
    else
    {
	printf("Max threads limit(%d) exceeded\n",MAX_NUM_THREADS);
	exit(1);
    }
}

int main (int argc, char ** argv)
{
	stProPerfConfig stMQConfig = {0};

	int ret,i, tempOffsetCtr = 0;
	struct timespec start, end;
        long elapsed;
	FILE *fp;
	char line[512];
	char tokens[6][512];
	char path[100];
	int  temp, flag = 0;
	char *keyvalue, *pair;
	char key[100];

	Int32 status = 0;
	int option;


    /* Initialize this to turn off verbosity of getopt */
    opterr = 0;
    
    while ((option = getopt (argc, argv, "f:")) != -1)
	{
		switch(option)
		{
			case 'f':
				strcpy(path, optarg);
				break;
			default:
				printf("Invalid option.. Exiting\n");
				PRINTEXIT
		}
	}

	fp = fopen(path, "r");
	if (fp == NULL) {
		fprintf(stderr, "couldn't open the specified file\n");
		PRINTEXIT
	}

	while (fgets(line, sizeof line, fp)) {
		printd("Line is = %s", line);

		if (line[0] == '#' || line[0] == '\n') {
			continue;
		}

		memset(tokens, 0, sizeof(tokens));
		i = 0;

		pair = strtok (line," ,");
		while (pair != NULL)
		{
			printd ("\tPair is = %s\n",pair);
			strcpy(tokens[i++], pair);
			pair = strtok (NULL, " ,.-");
		}

		for(temp=0; temp< i; temp++)
		{
			printd("Line %d: %s\n", temp, tokens[temp]);
			keyvalue = strtok (tokens[temp]," =");
			while (keyvalue != NULL)
			{
				if(flag == 0)
				{
					if(validatekey(keyvalue))
					{
						printf("Invalid key found\n");
						exit(0);
					}
					strcpy(key, keyvalue);
					printd ("\tKey is = %s\n",key);
					flag++;
				}
				else
				{
					printd ("\tValue is = %s",keyvalue);
					printd (" (%d)\n", atoi(keyvalue));
					add_key_value(key, atoi(keyvalue));
					flag = 0;
				}
				keyvalue = strtok (NULL, " =");
			}
		}
		linecount++;
		printd("%s", "------------------- \n");
	}

        	
	fclose(fp);

	stMQConfig.procId = COREPROC1; /* [TODO]: Remove Hardcode, Assuming for the time being same proc for all threads */ 
	stMQConfig.numThreads = linecount;
        stMQConfig.pThrConfig = (thrConfigs *)malloc(stMQConfig.numThreads * sizeof(struct stProPerfConfig)); 

	for(i = 0; i<stMQConfig.numThreads; i++)
	{
                stMQConfig.pThrConfig[i].thread_num = i;
                stMQConfig.pThrConfig[i].numMessages = xyz[i].msgCount;
                stMQConfig.pThrConfig[i].numWaitTime = xyz[i].interval;
                stMQConfig.pThrConfig[i].payloadSize = xyz[i].msgSize;
                stMQConfig.pThrConfig[i].procId = xyz[i].procID;
                stMQConfig.pThrConfig[i].config_param = xyz[i].direction;

		printf("Thread [%d] : direction = %d, msgSize = %d, msgCount = %d, interval = %d, procID = %d\n",
		stMQConfig.pThrConfig[i].thread_num,
                stMQConfig.pThrConfig[i].config_param,
                stMQConfig.pThrConfig[i].payloadSize,
                stMQConfig.pThrConfig[i].numMessages,
                stMQConfig.pThrConfig[i].numWaitTime,
                stMQConfig.pThrConfig[i].procId);

                stMQConfig.totalReqPayloadSize += stMQConfig.pThrConfig[i].payloadSize;
	}

    status = Ipc_start();
    if (status < 0) {
        printf ("Ipc_start failed: status = 0x%x\n", status);
        goto exit;
    }

    /* setup rpc connection) */
    status = Mx_initialize(stMQConfig.procId);
    if (status < 0) {
        printf("Mx_initialize failed: status = 0x%x\n", status);
	goto exit;
    }

    /*Use the MMRPC to get a shared space address*/
    ret = callCompute_Linux(&stMQConfig);
    if (ret < 0) {
        status = -1;
        goto leave;
    }

    /* handshake with remote to set numThreads, Message Size,Waittime */
    MessageQApp_handshake(stMQConfig);

    clock_gettime(CLOCK_REALTIME, &start);

    /* Launch multiple threads: */
    for (i = 0; i < stMQConfig.numThreads; i++) {
        /* Create the test threads as per directions: */
        stMQConfig.pThrConfig[i].boBufPayloadPtr = stMQConfig.bufferPtrRemoteAddr + tempOffsetCtr;
        stMQConfig.pThrConfig[i].boBufPayloadSize  = stMQConfig.pThrConfig[i].payloadSize;
	tempOffsetCtr += stMQConfig.pThrConfig[i].payloadSize;

	if(stMQConfig.pThrConfig[i].config_param == unidirectional_send)
	{
		ret = pthread_create(&stMQConfig.pThrConfig[i].thread_id, NULL, &pingThreadFxn_uni_send,
				   &(stMQConfig.pThrConfig[i]));
		if (ret) {
			printf("MessageQMulti: can't spawn thread: %d, %s\n",
						i, strerror(ret));
		}
#if PRINT_DEBUG
		printf("Sender Thread:%d Direction:%d bufferPtr = 0x%x, bufferPtrRemoteAddr = 0x%x bufferPtrSize = %d\n", 
			 stMQConfig.pThrConfig[i].thread_num, 
			 stMQConfig.pThrConfig[i].config_param,
			 stMQConfig.bufferPtr,
			 stMQConfig.pThrConfig[i].boBufPayloadPtr,
			 stMQConfig.pThrConfig[i].boBufPayloadSize);
#endif
	}
	else if(stMQConfig.pThrConfig[i].config_param == unidirectional_recv)
	{
			ret = pthread_create(&stMQConfig.pThrConfig[i].thread_id, NULL, &pingThreadFxn_uni_recv,
							   &(stMQConfig.pThrConfig[i]));
			if (ret) {
				printf("MessageQMulti: can't spawn thread: %d, %s\n",
						i, strerror(ret));
			}
#if PRINT_DEBUG
                printf("Receiver Thread:%d Direction:%d bufferPtr = 0x%x, bufferPtrRemoteAddr = 0x%x bufferPtrSize = %d\n",
                         stMQConfig.pThrConfig[i].thread_num,
                         stMQConfig.pThrConfig[i].config_param,
                         stMQConfig.bufferPtr,
                         stMQConfig.pThrConfig[i].boBufPayloadPtr,
                         stMQConfig.pThrConfig[i].boBufPayloadSize);

#endif
	}
    }

    /* Join all threads: */
    for (i = 0; i < stMQConfig.numThreads; i++) {
        ret = pthread_join(stMQConfig.pThrConfig[i].thread_id, NULL);
        if (ret != 0) {
            printf("MessageQMulti: failed to join thread: %d, %s\n",
                    i, strerror(ret));
        }
#if PRINT_DEBUG
        printf("MessageQMulti: Joined with thread %d\n",stMQConfig.pThrConfig[i].thread_num);
#endif
    }
    
    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = diff(start, end);

    printf("This use-case run took a total time of %ld msecs to transport totally\n",
        (elapsed/1000));

        /*** Data Transaction Prototype Function***/
        /* Create the test thread: */
        ret = dataTransactFxn(stMQConfig);
        if (ret) {
            printf("MessageQMulti: can't spawn thread: %d, %s\n",
                    0, strerror(ret));
        }

leave:
    free(stMQConfig.pThrConfig);
    stMQConfig.pThrConfig = NULL;

    status = Mx_compute_Release();
    if (status < 0) {
        printf("mmrpc_test: Error: MmRpc_release failed\n");
    }

    if (inBuf_bo) {
        omap_bo_del(inBuf_bo);
    }
    if (compute_bo) {
        omap_bo_del(compute_bo);
    }
    if (dev) {
        omap_device_del(dev);
    }
    if (drmFd) {
        close(drmFd);
    }
    
    /* finalize Mx module (destroy rpc connection) */
    Mx_finalize();

    Ipc_stop();

exit:

    return (0);
}
