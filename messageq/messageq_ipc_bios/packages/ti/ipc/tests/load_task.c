/*
 * Monitor load and trace any change.
 * Author: Vincent Stehlév-stehle@ti.com>, copied from ping_tasks.c
 *
 * Copyright (c) 2014, Texas Instruments Incorporated
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

#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Types.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/utils/Load.h>
#include <ti/sysbios/knl/Clock.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <load_task.h>

#ifdef __TMS470__
#pragma DATA_SECTION(gUtils_prfObj, ".bss")
#pragma DATA_SECTION(gUtils_accPrfLoadObj, ".bss")
#endif

Utils_PrfObj gUtils_prfObj;
Utils_AccPrfLoadObj gUtils_accPrfLoadObj;
UInt32 gUtils_startLoadCalc = 0;
UInt32 gUtils_LoadLogInterval = 0;
UInt32 gUtils_AutoResetLogInterval = 0;
Task_Handle loadTaskHndl;

Int32 Utils_prfInit()
{
    memset(&gUtils_prfObj, 0, sizeof(gUtils_prfObj));
    memset(&gUtils_accPrfLoadObj, 0, sizeof(Utils_AccPrfLoadObj));
/*
 * Time to sleep between load reporting attempts, in ticks.
 * On TI platforms, 1 tick == 1 ms.
 */
    gUtils_LoadLogInterval = 5000;

    return 0;
}

Int32 Utils_prfLoadRegister(Task_Handle pTsk, char *name)
{
    UInt32 hndlId;
    UInt32 cookie;
    Int32 status = -1;
    Utils_PrfLoadObj *pHndl;

    cookie = Hwi_disable();

    for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
    {
        pHndl = &gUtils_prfObj.loadObj[hndlId];

        if (pHndl->isAlloc == FALSE)
        {
            pHndl->isAlloc = TRUE;
            pHndl->pTsk = pTsk;
            strncpy(pHndl->name, name, sizeof(pHndl->name));
            System_printf("Utils_prfLoadRegister: %s, handle:0x%x\n", name, pTsk);
            status = 0;
            break;
        }
    }

    Hwi_restore(cookie);

    return status;
}

Int32 Utils_prfLoadUnRegister(Task_Handle pTsk)
{
    UInt32 hndlId;
    UInt32 cookie;
    Int32 status = -1;
    Utils_PrfLoadObj *pHndl;

    cookie = Hwi_disable();

    for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
    {
        pHndl = &gUtils_prfObj.loadObj[hndlId];

        if (pHndl->isAlloc && pHndl->pTsk == pTsk)
        {
            pHndl->isAlloc = FALSE;
            System_printf("Utils_prfLoadUnRegister:handle:0x%x\n", pTsk);
            status = 0;
            break;
        }
    }

    Hwi_restore(cookie);

    return status;
}

Int32 Utils_prfLoadPrintAll()
{
    Int32 hwiLoad, swiLoad, tskLoad, hndlId, cpuLoad, totalLoad, otherLoad;
    Utils_PrfLoadObj *pHndl;

    hwiLoad = swiLoad = tskLoad  = -1;

    hwiLoad = (gUtils_accPrfLoadObj.totalHwiThreadTime * 1000) /
        gUtils_accPrfLoadObj.totalTime;
    swiLoad = (gUtils_accPrfLoadObj.totalSwiThreadTime * 1000) /
        gUtils_accPrfLoadObj.totalTime;
    cpuLoad = 1000 - ((gUtils_accPrfLoadObj.totalIdlTskTime * 1000) /
                     gUtils_accPrfLoadObj.totalTime);

    totalLoad = hwiLoad+swiLoad;

    System_printf("\n [%d]LOAD: CPU: %d.%d%% HWI: %d.%d%%, SWI:%d.%d%%, ",gUtils_AutoResetLogInterval,
                cpuLoad/10, cpuLoad%10,
                hwiLoad/10, hwiLoad%10,
                swiLoad/10, swiLoad%10
                );

        for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
        {
            pHndl = &gUtils_prfObj.loadObj[hndlId];

            if (pHndl->isAlloc)
            {
                tskLoad = -1;

                tskLoad = (pHndl->totalTskThreadTime * 1000) /
                    gUtils_accPrfLoadObj.totalTime;

                totalLoad += tskLoad;
                if (tskLoad)
                {
                    System_printf(" TSK: %s: %d.%d%%",
                           pHndl->name,
                           tskLoad/10,
                           tskLoad%10
                        );
                }
            }
        }

        otherLoad = cpuLoad - totalLoad;

        System_printf(" %s: %d.%d%%",
               "MISC",
               otherLoad/10,
               otherLoad%10
            );

    System_printf(" \n");

    return 0;
}

Void Utils_prfLoadCalcReset()
{
    Utils_PrfLoadObj *pHndl;
    UInt32 hndlId;

    gUtils_accPrfLoadObj.totalHwiThreadTime = 0;
    gUtils_accPrfLoadObj.totalSwiThreadTime = 0;
    gUtils_accPrfLoadObj.totalTime = 0;
    gUtils_accPrfLoadObj.totalIdlTskTime = 0;
    /* Reset the performace loads accumulator */
    for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
    {
        pHndl = &gUtils_prfObj.loadObj[hndlId];

        if (pHndl->isAlloc && pHndl->pTsk != NULL)
        {
            pHndl->totalTskThreadTime = 0;

        }
    }
}

/* Monitor load and trace any change. */
static Void loadTaskFxn(UArg arg0, UArg arg1)
{
    /* Suppress warnings. */
    (void)arg0;
    (void)arg1;

    System_printf(
        "loadTask: started\n"
        "  SLEEP_TICKS: %u\n"
        "  Load_hwiEnabled: %d\n"
        "  Load_swiEnabled: %d\n"
        "  Load_taskEnabled: %d\n"
        "  Load_updateInIdle: %d\n"
        "  Load_windowInMs: %u\n"
        ,
        gUtils_LoadLogInterval,
        Load_hwiEnabled,
        Load_swiEnabled,
        Load_taskEnabled,
        Load_updateInIdle,
        Load_windowInMs
    );

    /* Infinite loop to trace load. */
    for (;;) {

	if (TRUE == gUtils_startLoadCalc)
	{
	    Utils_prfLoadPrintAll();
            gUtils_AutoResetLogInterval++;
	}

        /* Delay. */
	Task_sleep(gUtils_LoadLogInterval);

        /*Resetting the LoadCalcs every fifty iterations of data assumed to be collected,
          So that acummulated load figures are more real and correct*/
	if((gUtils_AutoResetLogInterval >= 50) && (gUtils_LoadLogInterval != 500))
	{
		Utils_prfLoadCalcReset();
		gUtils_AutoResetLogInterval = 0;
	}
    }
}

/* Function called by Loadupdate for each update cycle */
Void Utils_prfLoadUpdate()
{
    Utils_PrfLoadObj *pHndl;
    Load_Stat hwiLoadStat, swiLoadStat, idlTskLoadStat,tskLoadStat;
    Task_Handle idlTskHndl = NULL;
    UInt32 hndlId;

    idlTskHndl = Task_getIdleTask();
    /* Get the all loads first */
    Load_getGlobalHwiLoad(&hwiLoadStat);
    Load_getGlobalSwiLoad(&swiLoadStat);
    Load_getTaskLoad(idlTskHndl, &idlTskLoadStat);

    gUtils_accPrfLoadObj.totalHwiThreadTime += hwiLoadStat.threadTime;
    gUtils_accPrfLoadObj.totalSwiThreadTime += swiLoadStat.threadTime;
    gUtils_accPrfLoadObj.totalTime += hwiLoadStat.totalTime;
    gUtils_accPrfLoadObj.totalIdlTskTime += idlTskLoadStat.threadTime;

    for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
    {
        pHndl = &gUtils_prfObj.loadObj[hndlId];

        if (pHndl->isAlloc && pHndl->pTsk != NULL)
        {
            Load_getTaskLoad(pHndl->pTsk, &tskLoadStat);
            pHndl->totalTskThreadTime += tskLoadStat.threadTime;
        }
    }

}

void start_load_task(void)
{
    Task_Params params;
    Utils_prfInit();
    /* Monitor load and trace any change. */
    Task_Params_init(&params);
    params.instance->name = "loadtsk";
    params.priority = 4;
    loadTaskHndl = Task_create(loadTaskFxn, &params, NULL);

    if( NULL != loadTaskHndl)
    {
        Utils_prfLoadRegister(loadTaskHndl, "loadtsk");
    }
    else
        System_printf("Could not create load task!\n");
}
