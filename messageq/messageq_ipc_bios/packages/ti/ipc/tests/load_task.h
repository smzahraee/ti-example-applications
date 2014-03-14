/******************************************************************************
 *                                                                            *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/     *
 *                                                                            *
 * All rights reserved. Property of Texas Instruments Incorporated.           *
 * Restricted rights to use, duplicate or disclose this code are              *
 * granted through contract.                                                  *
 *                                                                            *
 * The program may not be used without the written permission                 *
 * of Texas Instruments Incorporated or against the terms and conditions      *
 * stipulated in the agreement under which this program has been              *
 * supplied.                                                                  *
 *                                                                            *
 *****************************************************************************/
/*
*   @file  profile_task.h
*
*/

#ifndef PROFILE_TASK_H
#define PROFILE_TASK_H


#ifdef _cplusplus
extern "C"
{
#endif /* _cplusplus */

#define UTILS_PRF_MAX_HNDL     (10)

/*
 * Time to sleep between load reporting attempts, in ticks.
 * On TI platforms, 1 tick == 1 ms.
 */
#define SLEEP_TICKS 1000

#define UTILS_FLOAT2INT_ROUND(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

typedef struct {
    Bool isAlloc;
    char name[32];
    Task_Handle pTsk;
    UInt64 totalTskThreadTime;

} Utils_PrfLoadObj;

typedef struct {
    //ils_PrfTsHndl tsObj[UTILS_PRF_MAX_HNDL];
    Utils_PrfLoadObj loadObj[UTILS_PRF_MAX_HNDL];

} Utils_PrfObj;

typedef struct {
    UInt64 totalSwiThreadTime;
    UInt64 totalHwiThreadTime;
    UInt64 totalTime;
    UInt64 totalIdlTskTime;
} Utils_AccPrfLoadObj;

void start_load_task(void);
Int32 Utils_prfLoadRegister(Task_Handle pTsk, char *name);
Void Utils_prfLoadCalcReset();
Int32 Utils_prfInit();

#ifdef _cplusplus
}
#endif /* _cplusplus */

#endif /* PROFILE_TASK_H */
