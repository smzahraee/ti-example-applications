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
 *  @file   MessageQZeroCpy.c
 *
 *  @brief  Sample application for MessageQ module between MPU and Remote Proc
 *          by Zero Copy Mechanism
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
#define SLAVE_MESSAGEQNAME          "SLAVE"
#define HOST_MESSAGEQNAME           "HOST"

/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */

/** [TODO] The below could change as per the platform used **/
/*OMAP5 remoteproc0:DSP:proc id 2*/
#define COREPROC0					2u
/*OMAP5 remoteproc1:IPU:proc id 1*/
#define COREPROC1					1u

#define  NUM_MESSAGES_DFLT 25200
#define  NUM_THREADS_DFLT 4
#define  NUM_MSGSIZE_DFLT 4
#define  MAX_NUM_THREADS  50

/** ============================================================================
 *  Globals
 *  ============================================================================
 */
static Int     numMessages, numThreads, numMsgSize ,procNum;
static unsigned int   bufferPtr, bufferPtrRemoteAddr, bufferPtrSize;

/*!
 *  @brief   structure
 */
typedef struct {
    int         size;  /* Size of the shared region */
    uint32_t *  inBuf; /* This is the shared region space address */
} Mx_Compute;

struct thread_info {    /* Used as argument to thread_start() */
    pthread_t thread_id;        /* ID returned by pthread_create() */
    int       thread_num;       /* Application-defined thread # */
    unsigned int boBufPayloadPtr; /*Shared Region pointer address per thread*/
    unsigned int boBufPayloadSize; /*Shared Region Size for this index of address*/
};

typedef struct RemoteBufSyncMsg {
    MessageQ_MsgHeader header;
    unsigned int boBufPayloadPtr; /*Shared Region pointer address to be exchanged using MessageQ*/
    unsigned int boBufPayloadSize; /*Shared Region pointer address  Size*/
} RemoteBufSyncMsg;


typedef struct SyncMsg {
    MessageQ_MsgHeader header; /* This is a mandatory requirement for MessageQ_alloc */
    unsigned int numChannels; /* Indicates the number of threads to the other core: Handshake */
    unsigned int numMessages; /* Indicates the number of messages: Handshake */
    unsigned int payloadSize; /* Indicates the size of the payload which will be carried by each message: Handshake */
} SyncMsg;

static void * pingThreadFxn(void *arg);

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

static int callCompute_Linux(uint32_t *mmInBufPtr, uint32_t *mmRemoteInBufPtr, uint32_t *mmInBufPtrSize)
{
    Mx_Compute *compute = NULL;
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
        compute = (Mx_Compute *)omap_bo_map(compute_bo);
    }
    else {
        fprintf(stderr, "failed to allocate omap_bo\n");
    }

    if (compute == NULL) {
        fprintf(stderr, "failed to map omap_bo to user space\n");
        return -1;
    }

    /* initialize compute structure */
    compute->size = (numMsgSize/(sizeof(uint32_t))); /* [TODO]Assumption: user_payload_size/sizeof(uint32_t) */
    compute->inBuf = NULL;

    /* allocate an input buffer in shared memory */
    size = compute->size * sizeof(uint32_t);
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
    for (i = 0; i < compute->size; i++) {
        inBufPtr[i] = 0xbeefdead;
    }

    compute->inBuf = (uint32_t *)inBufPtr;

    /* print some debug info */
#if PRINT_DEBUG    
    printf("compute->size=0x%x\n", compute->size);
    printf("compute->inBuf=0x%x\n", (unsigned int)compute->inBuf);
#endif
    /* process the buffer */
    ret = Mx_compute_Linux(compute, omap_bo_dmabuf(compute_bo),
                                    omap_bo_dmabuf(inBuf_bo));

    if (ret < 0) {
        status = -1;
        printf("Error: Mx_Compute() failed\n");
        return -1;
    }
#if PRINT_DEBUG
    printf("After Mx_compute_Linux mmrpc call \n");
    printf("compute->inBuf=0x%x\n", (unsigned int)compute->inBuf);
    printf("compute->inBuf[0]=0x%x\n",
            (unsigned int)compute->inBuf[0]);
#endif
    /*This checks Data Transaction at mmrpc level;check the output buffer */
   /* starting at index 1 because 0th index contains translated address of inBufPtr*/
    for (i = 1; i < compute->size; i++) {
        if (inBufPtr[i] != 0xdeadbeef) {
            status = 1;
            printf("Error: incorrect InBuf\n");
            break;
        }
    }

    *mmInBufPtr = (unsigned int)compute->inBuf;
    *mmRemoteInBufPtr = (unsigned int)compute->inBuf[0];
    *mmInBufPtrSize = size;

    return (status);
}

Int MessageQApp_handshake(UInt32 numChannels, UInt32 numMessages, UInt32 payloadSize, UInt16 procId)
{
    Int32                    status = 0;
    MessageQ_Msg             msg = NULL;
    MessageQ_Params          msgParams;
    MessageQ_QueueId         queueId = MessageQ_INVALIDMESSAGEQ;
    MessageQ_Handle          msgqHandle;
    char                     remoteQueueName[64];

#if PRINT_DEBUG
    printf("Entered MessageQApp_handshake\n");
#endif

    /* Create the local Message Queue for receiving. */
    MessageQ_Params_init(&msgParams);
    msgqHandle = MessageQ_create(HOST_MESSAGEQNAME, &msgParams);
    if (msgqHandle == NULL) {
        printf("Error in MessageQ_create\n");
        goto exit;
    }
#if PRINT_DEBUG
    else {
        printf("Local MessageQId: 0x%x\n", MessageQ_getQueueId(msgqHandle));
    }
#endif
    sprintf(remoteQueueName, "%s_%s", SLAVE_MESSAGEQNAME,
             MultiProc_getName(procId));

    //printf("%s:%d remoteQueueName:%s\n",__func__,__LINE__,remoteQueueName);
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
        printf("Remote queueId  [0x%x]\n", queueId);
    }
#endif

    msg = MessageQ_alloc(HEAPID, sizeof(SyncMsg));
    if (msg == NULL) {
        printf("Error in MessageQ_alloc\n");
        MessageQ_close(&queueId);
        goto cleanup;
    }

    /* handshake with remote to set the number of loops */
    MessageQ_setReplyQueue(msgqHandle, msg);
    ((SyncMsg *)msg)->numMessages = numMessages;
    ((SyncMsg *)msg)->numChannels = numChannels;
    ((SyncMsg *)msg)->payloadSize = payloadSize;
    
    MessageQ_put(queueId, msg);
    MessageQ_get(msgqHandle, &msg, MessageQ_FOREVER);
#if PRINT_DEBUG
    printf("Exchanged handshake with remote processor %s...\n",
           MultiProc_getName(procId));
#endif

    MessageQ_free(msg);
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


static Void * pingThreadFxn(void *arg)
{
    struct thread_info pingThreadFxnData = *(struct thread_info *)arg;   
    Int                      threadNum = 0;
    Int32                    status     = 0;
    MessageQ_Msg             msg        = NULL;
    MessageQ_Params          msgParams;
    UInt16                   i;
    MessageQ_Handle          handle;
    MessageQ_QueueId         queueId = MessageQ_INVALIDMESSAGEQ;

    char             remoteQueueName[64];
    char             hostQueueName[64];

    threadNum = pingThreadFxnData.thread_num;
    sprintf(remoteQueueName, "%s_%d", SLAVE_MESSAGEQNAME, threadNum );
    sprintf(hostQueueName,   "%s_%d", HOST_MESSAGEQNAME,  threadNum );

#if PRINT_DEBUG
    printf("pingThreadFxn This thread num: %d\n", threadNum);
#endif
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

    /* Poll until remote side has it's messageQ created before we send: */
    do {
        status = MessageQ_open (remoteQueueName, &queueId);
        sleep (1);
    } while (status == MessageQ_E_NOTFOUND);
    if (status < 0) {
        printf ("Error in MessageQ_open [0x%x]\n", status);
        goto cleanup;
    }
#if PRINT_DEBUG
    else {
        printf ("thread: %d, Remote queue: %s, QId: 0x%x\n",
                 threadNum, remoteQueueName, queueId);
    }

    printf ("\nthread: %d: Exchanging messages with remote processor...\n",
            threadNum);
#endif

    for (i = 0 ; i < numMessages ; i++) {
        /* Allocate message. */
        msg = MessageQ_alloc (HEAPID, sizeof(RemoteBufSyncMsg));
        if (msg == NULL) {
            printf ("Error in MessageQ_alloc\n");
            break;
        }

        MessageQ_setMsgId (msg, i);

        /* Have the remote proc reply to this message queue */
        MessageQ_setReplyQueue (handle, msg);
      ((RemoteBufSyncMsg *)msg)->boBufPayloadPtr = pingThreadFxnData.boBufPayloadPtr;
      ((RemoteBufSyncMsg *)msg)->boBufPayloadSize = pingThreadFxnData.boBufPayloadSize;

        status = MessageQ_put (queueId, msg);
        if (status < 0) {
            printf ("Error in MessageQ_put [0x%x]\n", status);
            break;
        }

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
    
    MessageQ_close (&queueId);

cleanup:
    /* Clean-up */
    status = MessageQ_delete (&handle);
    if (status < 0) {
        printf ("Error in MessageQ_delete [0x%x]\n", status);
    }

exit:
    return ((void *)status);
}

Int dataTransactFxn()
{
    Int32                    status     = 0;
    MessageQ_Msg             msg        = NULL;
    MessageQ_Params          msgParams;
    UInt16                   i;
    MessageQ_Handle          handle;
    MessageQ_QueueId         queueId = MessageQ_INVALIDMESSAGEQ;

    char             remoteQueueName[64];
    char             hostQueueName[64];

    sprintf(remoteQueueName, "%s_%d", SLAVE_MESSAGEQNAME, 0 );
    sprintf(hostQueueName,   "%s_%d", HOST_MESSAGEQNAME,  0 );

    printf("dataTransactFxn: This function is a test to showcase data transaction \n");

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
    msg = MessageQ_alloc (HEAPID, sizeof(RemoteBufSyncMsg));
    if (msg == NULL) {
        printf ("Error in MessageQ_alloc\n");
        goto cleanup;
    }

    MessageQ_setMsgId (msg, 1);/* Set a random number to associate with this message*/

    for (i = 0; i < (bufferPtrSize/sizeof(uint32_t)); i++) {
        ((unsigned int *)bufferPtr)[i] = 0xbeefdead;
    }

    /* Have the rem te proc reply to this message queue */
    MessageQ_setReplyQueue (handle, msg);
    ((RemoteBufSyncMsg *)msg)->boBufPayloadPtr = bufferPtrRemoteAddr;
    ((RemoteBufSyncMsg *)msg)->boBufPayloadSize = bufferPtrSize;

    status = MessageQ_put (queueId, msg);
    if (status < 0) {
        printf ("Error in MessageQ_put [0x%x]\n", status);
        goto cleanup;
    }

    status = MessageQ_get(handle, &msg, MessageQ_FOREVER);
    if (status < 0) {
       printf ("Error in MessageQ_get [0x%x]\n", status);
       goto cleanup;
    }
    else {
       for (i = 0; i < (bufferPtrSize/sizeof(uint32_t)); i++) {
              if (((unsigned int *)bufferPtr)[i] != 0xdeadbeef) {
              status = 1;
              printf ("Data integrity failure!\n"
                "    Expected %s\n"
                "    Received 0x%x\n",
                "0xdeadbeef", ((unsigned int *)bufferPtr)[i]);
              break;
        }
    }
    if(status != 1)
        printf("%s:Test Pass\n",__func__);
    /* Validate the returned message. */
    status = MessageQ_free (msg);
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


int main (int argc, char ** argv)
{
    struct thread_info threads[MAX_NUM_THREADS]={{0}};
    int ret,i, tempnummsg;
    struct timespec start, end;
    long                     elapsed;
    Int32 status = 0;
    tempnummsg = 0;

    bufferPtr = 0;
    bufferPtrRemoteAddr = 0;

    /* Parse Args: */
    numMessages = NUM_MESSAGES_DFLT;
    numThreads = NUM_THREADS_DFLT;
    numMsgSize = NUM_MSGSIZE_DFLT;
    procNum = COREPROC1;
    switch (argc) {
        case 1:
           /* use defaults */
           break;
        case 2:
           numThreads = atoi(argv[1]);
           break;
        case 3:
           numThreads = atoi(argv[1]);
           numMessages   = atoi(argv[2]);
           break;
        case 4:
           numThreads = atoi(argv[1]);
           numMessages   = atoi(argv[2]);
           numMsgSize  = atoi(argv[3]);
           break;
        case 5:
           numThreads = atoi(argv[1]);
           numMessages   = atoi(argv[2]);
           numMsgSize  = atoi(argv[3]);
           procNum = atoi(argv[4]);
           break;
        default:
           printf("Usage: %s [<numThreads>] [<numMessages>] [<numMsgPayloadSize>] [<Proc Id #]>\n",
                   argv[0]);
           printf("\tDefaults: numThreads: 4, numMessages: 25200, numMsgSize: 4, IPU Proc Id: 1\n");
           printf("\tMax Threads: 50\n");
           exit(0);
    }

    if(numMsgSize & 0x3)
    {
        printf("each message payload size should be 4bytes aligned\n");
        numMsgSize = NUM_MSGSIZE_DFLT;
    }
    
    printf("Using: %d Threads, Messages: %d, Message Size:%d bytes, Message Payload Size(bytes):%d, ProcId %d\n", 
          numThreads, 
          numMessages, 
          sizeof(RemoteBufSyncMsg),
          numMsgSize, 
          procNum);

    tempnummsg = numMsgSize * numThreads;
    numMsgSize = tempnummsg;

    status = Ipc_start();
    if (status < 0) {
        printf ("Ipc_start failed: status = 0x%x\n", status);
        goto exit;
    }

    /* setup rpc connection) */
    status = Mx_initialize(procNum);

    if (status < 0) {
        printf("Mx_initialize failed: status = 0x%x\n", status);
    }

    /*Use the MMRPC to get a shared space address*/
    ret = callCompute_Linux(&bufferPtr, &bufferPtrRemoteAddr, &bufferPtrSize);
    if (ret < 0) {
        status = -1;
        goto leave;
    }

    tempnummsg = (numMessages/numThreads);
    numMessages = tempnummsg;/*Num messages to be exchanged per thread*/

    /* handshake with remote to set numThreads, numMessages, Message Size */
    MessageQApp_handshake(numThreads,numMessages,numMsgSize,procNum);

    clock_gettime(CLOCK_REALTIME, &start);

    /* Launch multiple threads: */
    for (i = 0; i < numThreads; i++) {
        /* Create the test thread: */
        threads[i].thread_num = i;
        threads[i].boBufPayloadPtr = bufferPtrRemoteAddr + (i * numMsgSize/numThreads);
        threads[i].boBufPayloadSize  = (bufferPtrSize/numThreads);
        ret = pthread_create(&threads[i].thread_id, NULL, &pingThreadFxn,
                           &(threads[i]));
        if (ret) {
            printf("MessageQZeroCpy: can't spawn thread: %d, %s\n",
                    i, strerror(ret));
        }
#if PRINT_DEBUG
        printf("thread:%d  bufferPtr = 0x%x, bufferPtrRemoteAddr = 0x%x bufferPtrSize = %d\n", 
                 i, 
                 bufferPtr, 
                 threads[i].boBufPayloadPtr,
                 threads[i].boBufPayloadSize);
#endif
    }

    /* Join all threads: */
    for (i = 0; i < numThreads; i++) {
        ret = pthread_join(threads[i].thread_id, NULL);
        if (ret != 0) {
            printf("MessageQZeroCpy: failed to join thread: %d, %s\n",
                    i, strerror(ret));
        }
#if PRINT_DEBUG
        printf("MessageQZeroCpy: Joined with thread %d\n",threads[i].thread_num);
#endif
    }
    
    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = diff(start, end);

    printf("This run took a total return time of %ld msecs to transport totally about \n %d Messages each containing %d bytes of data across %d threads\n",
        (elapsed/1000),
        (numMessages*numThreads),
         (numMsgSize/numThreads),
        numThreads);

        /*** Data Transaction Prototype Function***/
        /* Create the test thread: */
        ret = dataTransactFxn();
        if (ret) {
            printf("MessageQZeroCpy: can't spawn thread: %d, %s\n",
                    0, strerror(ret));
        }
    
leave:
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
