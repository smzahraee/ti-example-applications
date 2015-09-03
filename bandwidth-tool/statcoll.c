/*
 *  Copyright (c) 2015, Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file       statcoll.c
 * @authors    Karthik Ramanan <karthik.ramanan@ti.com>
 *
 *             Adapted to Linux with changes in framework from original example
 *             from prash@ti.com 
 *
 * @brief      Linux utility to configure and read the statistics collectors
 *             on the Jacinto 6 SoC
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "statcoll.h"

#define ENABLE_MODE      0x0
#define READ_STATUS_MODE 0x1

//#define DUMMY_MODE

const struct list_of_initiators initiators[STATCOL_MAX] =
{
    { STATCOL_EMIF1_SYS, "STATCOL_EMIF1_SYS" },
    { STATCOL_EMIF2_SYS,"STATCOL_EMIF2_SYS" },
    { STATCOL_MA_MPU_P1,"STATCOL_MA_MPU_P1" },
    { STATCOL_MA_MPU_P2,"STATCOL_MA_MPU_P2" },
    { STATCOL_MPU1,"STATCOL_MPU1" },
    { STATCOL_MMU1,"STATCOL_MMU1" },
    { STATCOL_TPTC_RD1,"STATCOL_TPTC_RD1" },
    { STATCOL_TPTC_WR1,"STATCOL_TPTC_WR1" },
    { STATCOL_TPTC_RD2,"STATCOL_TPTC_RD2" },
    { STATCOL_TPTC_WR2,"STATCOL_TPTC_WR2" },
    { STATCOL_VIP1_P1,"STATCOL_VIP1_P1" },
    { STATCOL_VIP1_P2,"STATCOL_VIP1_P2" },
    { STATCOL_VIP2_P1,"STATCOL_VIP2_P1" },
    { STATCOL_VIP2_P2,"STATCOL_VIP2_P2" },
    { STATCOL_VIP3_P1,"STATCOL_VIP3_P1" },
    { STATCOL_VIP3_P2,"STATCOL_VIP3_P2" },
    { STATCOL_VPE_P1,"STATCOL_VPE_P1" },
    { STATCOL_VPE_P2,"STATCOL_VPE_P2" },
    { STATCOL_EVE1_TC0,"STATCOL_EVE1_TC0" },
    { STATCOL_EVE1_TC1,"STATCOL_EVE1_TC1" },
    { STATCOL_EVE2_TC0,"STATCOL_EVE2_TC0" },
    { STATCOL_EVE2_TC1,"STATCOL_EVE2_TC1" },
    { STATCOL_EVE3_TC0,"STATCOL_EVE3_TC0" },
    { STATCOL_EVE3_TC1,"STATCOL_EVE3_TC1" },
    { STATCOL_EVE4_TC0,"STATCOL_EVE4_TC0" },
    { STATCOL_EVE4_TC1,"STATCOL_EVE4_TC1" },
    { STATCOL_DSP1_MDMA,"STATCOL_DSP1_MDMA" },
    { STATCOL_DSP1_EDMA,"STATCOL_DSP1_EDMA" },
    { STATCOL_DSP2_MDMA,"STATCOL_DSP2_MDMA" },
    { STATCOL_DSP2_EDMA,"STATCOL_DSP2_EDMA" },
    { STATCOL_IVA,"STATCOL_IVA" },
    { STATCOL_GPU_P1,"STATCOL_GPU_P1" },
    { STATCOL_GPU_P2,"STATCOL_GPU_P2" },
    { STATCOL_BB2D_P1,"STATCOL_BB2D_P1" },
    { STATCOL_DSS,"STATCOL_DSS" },
    { STATCOL_CSI2_2,"STATCOL_CSI2_2" },
    { STATCOL_MMU2,"STATCOL_MMU2" },
    { STATCOL_IPU1,"STATCOL_IPU1" },
    { STATCOL_IPU2,"STATCOL_IPU2" },
    { STATCOL_DMA_SYSTEM_RD,"STATCOL_DMA_SYSTEM_RD" },
    { STATCOL_DMA_SYSTEM_WR,"STATCOL_DMA_SYSTEM_WR" },
    { STATCOL_CSI2_1,"STATCOL_CSI2_1" },
    { STATCOL_USB3_SS,"STATCOL_USB3_SS" },
    { STATCOL_USB2_SS,"STATCOL_USB2_SS" },
    { STATCOL_USB2_ULPI_SS1,"STATCOL_USB2_ULPI_SS1" },
    { STATCOL_USB2_ULPI_SS2,"STATCOL_USB2_ULPI_SS2" },
    { STATCOL_PCIE_SS1,"STATCOL_PCIE_SS1" },
    { STATCOL_PCIE_SS2,"STATCOL_PCIE_SS2" },
    { STATCOL_DSP1_CFG,"STATCOL_DSP1_CFG" },
    { STATCOL_DSP2_CFG,"STATCOL_DSP2_CFG" },
    { STATCOL_GMAC_SW,"STATCOL_GMAC_SW" },
    { STATCOL_PRUSS1_P1,"STATCOL_PRUSS1_P1" },
    { STATCOL_PRUSS1_P2,"STATCOL_PRUSS1_P2" },
    { STATCOL_PRUSS2_P1,"STATCOL_PRUSS2_P1" },
    { STATCOL_PRUSS2_P2,"STATCOL_PRUSS2_P2" },
    { STATCOL_DMA_CRYPTO_RD,"STATCOL_DMA_CRYPTO_RD" },
    { STATCOL_DMA_CRYPTO_WR,"STATCOL_DMA_CRYPTO_WR" },
    { STATCOL_MPU2,"STATCOL_MPU2" },
    { STATCOL_MMC1,"STATCOL_MMC1" },
    { STATCOL_MMC2,"STATCOL_MMC2" },
    { STATCOL_SATA,"STATCOL_SATA" },
    { STATCOL_MLBSS,"STATCOL_MLBSS" },
    { STATCOL_BB2D_P2,"STATCOL_BB2D_P2" },
    { STATCOL_IEEE1500,"STATCOL_IEEE1500" },
    { STATCOL_DBG,"STATCOL_DBG" },
    { STATCOL_VCP1,"STATCOL_VCP1" },
    { STATCOL_OCMC_RAM1,"STATCOL_OCMC_RAM1" },
    { STATCOL_OCMC_RAM2,"STATCOL_OCMC_RAM2" },
    { STATCOL_OCMC_RAM3,"STATCOL_OCMC_RAM3" },
    { STATCOL_GPMC,"STATCOL_GPMC" },
    { STATCOL_MCASP1,"STATCOL_MCASP1" },
    { STATCOL_MCASP2,"STATCOL_MCASP2" },
    { STATCOL_MCASP3,"STATCOL_MCASP3" },
    { STATCOL_VCP2,  "STATCOL_VCP2" }
}; 

StatCollectorObj gStatColState;

static void *statcoll_base_mem;
static int *l3_3_clkctrl;

static UInt32 *statCountDSS = NULL;
static UInt32 *statCountIVA = NULL;
static UInt32 *statCountBB2DP1 = NULL;
static UInt32 *statCountBB2DP2 = NULL;
static UInt32 *statCountUSB4 = NULL;
static UInt32 *statCountSata = NULL;
static UInt32 *statCountEmif1 = NULL;
static UInt32 *statCountEmif2 = NULL;


static statcoll_initiators_object global_object[STATCOL_MAX];
UInt32 statCountIdx = 0;
UInt32 TRACE_SZ = 0;

void statCollectorInit()
{
    int index;

    gStatColState.stat0_filter_cnt = 0;
    gStatColState.stat1_filter_cnt = 0;
    gStatColState.stat2_filter_cnt = 0;
    gStatColState.stat3_filter_cnt = 0;
    gStatColState.stat4_filter_cnt = 0;
    gStatColState.stat5_filter_cnt = 0;
    gStatColState.stat6_filter_cnt = 0;
    gStatColState.stat7_filter_cnt = 0;
    gStatColState.stat8_filter_cnt = 0;
    gStatColState.stat9_filter_cnt = 0;

    /*
       typedef struct
       {
       UInt32 b_enabled;
       char name[100];
       UInt32 *readings;
       UInt32 *timestamp;
       UInt32 counter_id;
       UInt32 base_address;
       UInt32 mux_req;
       }statcoll_initiators_object;

     */
    for(index=STATCOL_EMIF1_SYS; index < STATCOL_MAX; index++)
    {
	global_object[index].b_enabled = 0;

	strcpy(global_object[index].name, initiators[index].name); 

	global_object[index].readings = malloc(TRACE_SZ * sizeof(UInt32));
    	memset(global_object[index].readings, 0, TRACE_SZ * sizeof(UInt32));

	global_object[index].timestamp = NULL;

	global_object[index].group_id = 0xFF;
	global_object[index].counter_id = 0;
	global_object[index].base_address = 0;
	global_object[index].mux_req = 0;
    }

}

void wr_stat_reg(UInt32 address, UInt32 data)
{
    UInt32 *mymem = statcoll_base_mem;
    UInt32 delta = (address - STATCOLL_BASE) / 4;
#ifndef DUMMY_MODE
    mymem[delta] = data;
#else
    printf("WRITE: Address = 0x%x, Data = 0x%x\n", address, data);
#endif
}

UInt32 rd_stat_reg(UInt32 address)
{
#ifndef DUMMY_MODE
    UInt32 *mymem = statcoll_base_mem;
    UInt32 data;
    UInt32 delta = (address - STATCOLL_BASE) / 4;
    data = mymem[delta];
    return data;
#else
    printf("READ: Address = 0x%x\n", address);
#endif
}

UInt32 statCollectorControlInitialize(UInt32 instance_id)
{
    UInt32 cur_base_address = 0;
    UInt32 cur_event_mux_req;
    UInt32 cur_event_mux_resp;
    UInt32 cur_stat_filter_cnt;

    switch (instance_id)
    {
    case STATCOL_EMIF1_SYS:
        cur_base_address = stat_coll0_base_address;
        cur_event_mux_req = 0;
        cur_event_mux_resp = 1;
        gStatColState.stat0_filter_cnt = gStatColState.stat0_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat0_filter_cnt;
	global_object[instance_id].group_id = 0;
        break;
    case STATCOL_EMIF2_SYS:
        cur_base_address = stat_coll0_base_address;
        cur_event_mux_req = 2;
        cur_event_mux_resp = 3;
        gStatColState.stat0_filter_cnt = gStatColState.stat0_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat0_filter_cnt;
	global_object[instance_id].group_id = 0;
        break;
    case STATCOL_MA_MPU_P1:
        cur_base_address = stat_coll0_base_address;
        cur_event_mux_req = 4;
        cur_event_mux_resp = 5;
        gStatColState.stat0_filter_cnt = gStatColState.stat0_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat0_filter_cnt;
	global_object[instance_id].group_id = 0;
        break;
    case STATCOL_MA_MPU_P2:
        cur_base_address = stat_coll0_base_address;
        cur_event_mux_req = 6;
        cur_event_mux_resp = 7;
        gStatColState.stat0_filter_cnt = gStatColState.stat0_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat0_filter_cnt;
	global_object[instance_id].group_id = 0;
        break;
    case STATCOL_MPU1:
        cur_base_address = stat_coll1_base_address;
        cur_event_mux_req = 0;
        cur_event_mux_resp = 1;
        gStatColState.stat1_filter_cnt = gStatColState.stat1_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat1_filter_cnt;
	global_object[instance_id].group_id = 1;
        break;
    case STATCOL_MMU1:
        cur_base_address = stat_coll1_base_address;
        cur_event_mux_req = 2;
        cur_event_mux_resp = 3;
        gStatColState.stat1_filter_cnt = gStatColState.stat1_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat1_filter_cnt;
	global_object[instance_id].group_id = 1;
        break;
    case STATCOL_TPTC_RD1:
        cur_base_address = stat_coll1_base_address;
        cur_event_mux_req = 4;
        cur_event_mux_resp = 5;
        gStatColState.stat1_filter_cnt = gStatColState.stat1_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat1_filter_cnt;
	global_object[instance_id].group_id = 1;
        break;
    case STATCOL_TPTC_WR1:
        cur_base_address = stat_coll1_base_address;
        cur_event_mux_req = 6;
        cur_event_mux_resp = 7;
        gStatColState.stat1_filter_cnt = gStatColState.stat1_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat1_filter_cnt;
	global_object[instance_id].group_id = 1;
        break;
    case STATCOL_TPTC_RD2:
        cur_base_address = stat_coll1_base_address;
        cur_event_mux_req = 8;
        cur_event_mux_resp = 9;
        gStatColState.stat1_filter_cnt = gStatColState.stat1_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat1_filter_cnt;
	global_object[instance_id].group_id = 1;
        break;
    case STATCOL_TPTC_WR2:
        cur_base_address = stat_coll1_base_address;
        cur_event_mux_req = 10;
        cur_event_mux_resp = 11;
        gStatColState.stat1_filter_cnt = gStatColState.stat1_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat1_filter_cnt;
	global_object[instance_id].group_id = 1;
        break;
    case STATCOL_VIP1_P1:
        cur_base_address = stat_coll2_base_address;
        cur_event_mux_req = 0;
        cur_event_mux_resp = 1;
        gStatColState.stat2_filter_cnt = gStatColState.stat2_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat2_filter_cnt;
	global_object[instance_id].group_id = 2;
        break;
    case STATCOL_VIP1_P2:
        cur_base_address = stat_coll2_base_address;
        cur_event_mux_req = 2;
        cur_event_mux_resp = 3;
        gStatColState.stat2_filter_cnt = gStatColState.stat2_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat2_filter_cnt;
	global_object[instance_id].group_id = 2;
        break;
    case STATCOL_VIP2_P1:
        cur_base_address = stat_coll2_base_address;
        cur_event_mux_req = 4;
        cur_event_mux_resp = 5;
        gStatColState.stat2_filter_cnt = gStatColState.stat2_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat2_filter_cnt;
	global_object[instance_id].group_id = 2;
        break;
    case STATCOL_VIP2_P2:
        cur_base_address = stat_coll2_base_address;
        cur_event_mux_req = 6;
        cur_event_mux_resp = 7;
        gStatColState.stat2_filter_cnt = gStatColState.stat2_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat2_filter_cnt;
	global_object[instance_id].group_id = 2;
        break;
    case STATCOL_VIP3_P1:
        cur_base_address = stat_coll2_base_address;
        cur_event_mux_req = 8;
        cur_event_mux_resp = 9;
        gStatColState.stat2_filter_cnt = gStatColState.stat2_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat2_filter_cnt;
	global_object[instance_id].group_id = 2;
        break;
    case STATCOL_VIP3_P2:
        cur_base_address = stat_coll2_base_address;
        cur_event_mux_req = 10;
        cur_event_mux_resp = 11;
        gStatColState.stat2_filter_cnt = gStatColState.stat2_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat2_filter_cnt;
	global_object[instance_id].group_id = 2;
        break;
    case STATCOL_VPE_P1:
        cur_base_address = stat_coll2_base_address;
        cur_event_mux_req = 12;
        cur_event_mux_resp = 13;
        gStatColState.stat2_filter_cnt = gStatColState.stat2_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat2_filter_cnt;
	global_object[instance_id].group_id = 2;
        break;
    case STATCOL_VPE_P2:
        cur_base_address = stat_coll2_base_address;
        cur_event_mux_req = 14;
        cur_event_mux_resp = 15;
        gStatColState.stat2_filter_cnt = gStatColState.stat2_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat2_filter_cnt;
	global_object[instance_id].group_id = 2;
        break;
    case STATCOL_EVE1_TC0:
        cur_base_address = stat_coll3_base_address;
        cur_event_mux_req = 0;
        cur_event_mux_resp = 1;
        gStatColState.stat3_filter_cnt = gStatColState.stat3_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat3_filter_cnt;
	global_object[instance_id].group_id = 3;
        break;
    case STATCOL_EVE1_TC1:
        cur_base_address = stat_coll3_base_address;
        cur_event_mux_req = 2;
        cur_event_mux_resp = 3;
        gStatColState.stat3_filter_cnt = gStatColState.stat3_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat3_filter_cnt;
	global_object[instance_id].group_id = 3;
        break;
    case STATCOL_EVE2_TC0:
        cur_base_address = stat_coll3_base_address;
        cur_event_mux_req = 4;
        cur_event_mux_resp = 5;
        gStatColState.stat3_filter_cnt = gStatColState.stat3_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat3_filter_cnt;
	global_object[instance_id].group_id = 3;
        break;
    case STATCOL_EVE2_TC1:
        cur_base_address = stat_coll3_base_address;
        cur_event_mux_req = 6;
        cur_event_mux_resp = 7;
        gStatColState.stat3_filter_cnt = gStatColState.stat3_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat3_filter_cnt;
	global_object[instance_id].group_id = 3;
        break;
    case STATCOL_EVE3_TC0:
        cur_base_address = stat_coll3_base_address;
        cur_event_mux_req = 8;
        cur_event_mux_resp = 9;
        gStatColState.stat3_filter_cnt = gStatColState.stat3_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat3_filter_cnt;
	global_object[instance_id].group_id = 3;
        break;
    case STATCOL_EVE3_TC1:
        cur_base_address = stat_coll3_base_address;
        cur_event_mux_req = 10;
        cur_event_mux_resp = 11;
        gStatColState.stat3_filter_cnt = gStatColState.stat3_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat3_filter_cnt;
	global_object[instance_id].group_id = 3;
        break;
    case STATCOL_EVE4_TC0:
        cur_base_address = stat_coll3_base_address;
        cur_event_mux_req = 12;
        cur_event_mux_resp = 13;
        gStatColState.stat3_filter_cnt = gStatColState.stat3_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat3_filter_cnt;
	global_object[instance_id].group_id = 3;
        break;
    case STATCOL_EVE4_TC1:
        cur_base_address = stat_coll3_base_address;
        cur_event_mux_req = 14;
        cur_event_mux_resp = 15;
        gStatColState.stat3_filter_cnt = gStatColState.stat3_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat3_filter_cnt;
	global_object[instance_id].group_id = 3;
        break;
    case STATCOL_DSP1_MDMA:
        cur_base_address = stat_coll4_base_address;
        cur_event_mux_req = 0;
        cur_event_mux_resp = 1;
        gStatColState.stat4_filter_cnt = gStatColState.stat4_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat4_filter_cnt;
	global_object[instance_id].group_id = 4;
        break;
    case STATCOL_DSP1_EDMA:
        cur_base_address = stat_coll4_base_address;
        cur_event_mux_req = 2;
        cur_event_mux_resp = 3;
        gStatColState.stat4_filter_cnt = gStatColState.stat4_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat4_filter_cnt;
	global_object[instance_id].group_id = 4;
        break;
    case STATCOL_DSP2_MDMA:
        cur_base_address = stat_coll4_base_address;
        cur_event_mux_req = 4;
        cur_event_mux_resp = 5;
        gStatColState.stat4_filter_cnt = gStatColState.stat4_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat4_filter_cnt;
	global_object[instance_id].group_id = 4;
        break;
    case STATCOL_DSP2_EDMA:
        cur_base_address = stat_coll4_base_address;
        cur_event_mux_req = 6;
        cur_event_mux_resp = 7;
        gStatColState.stat4_filter_cnt = gStatColState.stat4_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat4_filter_cnt;
	global_object[instance_id].group_id = 4;
        break;
    case STATCOL_IVA:
        cur_base_address = stat_coll4_base_address;
        cur_event_mux_req = 8;
        cur_event_mux_resp = 9;
        gStatColState.stat4_filter_cnt = gStatColState.stat4_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat4_filter_cnt;
	global_object[instance_id].group_id = 4;
        break;
    case STATCOL_GPU_P1:
        cur_base_address = stat_coll4_base_address;
        cur_event_mux_req = 10;
        cur_event_mux_resp = 11;
        gStatColState.stat4_filter_cnt = gStatColState.stat4_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat4_filter_cnt;
	global_object[instance_id].group_id = 4;
        break;
    case STATCOL_GPU_P2:
        cur_base_address = stat_coll4_base_address;
        cur_event_mux_req = 12;
        cur_event_mux_resp = 13;
        gStatColState.stat4_filter_cnt = gStatColState.stat4_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat4_filter_cnt;
	global_object[instance_id].group_id = 4;
        break;
    case STATCOL_BB2D_P1:
        cur_base_address = stat_coll4_base_address;
        cur_event_mux_req = 14;
        cur_event_mux_resp = 15;
        gStatColState.stat4_filter_cnt = gStatColState.stat4_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat4_filter_cnt;
	global_object[instance_id].group_id = 4;
        break;
    case STATCOL_DSS:
        cur_base_address = stat_coll5_base_address;
        cur_event_mux_req = 0;
        cur_event_mux_resp = 1;
        gStatColState.stat5_filter_cnt = gStatColState.stat5_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat5_filter_cnt;
	global_object[instance_id].group_id = 5;
        break;
    case STATCOL_CSI2_2:
        cur_base_address = stat_coll5_base_address;
        cur_event_mux_req = 2;
        cur_event_mux_resp = 3;
        gStatColState.stat5_filter_cnt = gStatColState.stat5_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat5_filter_cnt;
	global_object[instance_id].group_id = 5;
        break;
    case STATCOL_MMU2:
        cur_base_address = stat_coll5_base_address;
        cur_event_mux_req = 4;
        cur_event_mux_resp = 5;
        gStatColState.stat5_filter_cnt = gStatColState.stat5_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat5_filter_cnt;
	global_object[instance_id].group_id = 5;
        break;
    case STATCOL_IPU1:
        cur_base_address = stat_coll5_base_address;
        cur_event_mux_req = 6;
        cur_event_mux_resp = 7;
        gStatColState.stat5_filter_cnt = gStatColState.stat5_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat5_filter_cnt;
	global_object[instance_id].group_id = 5;
        break;
    case STATCOL_IPU2:
        cur_base_address = stat_coll5_base_address;
        cur_event_mux_req = 8;
        cur_event_mux_resp = 9;
        gStatColState.stat5_filter_cnt = gStatColState.stat5_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat5_filter_cnt;
	global_object[instance_id].group_id = 5;
        break;
    case STATCOL_DMA_SYSTEM_RD:
        cur_base_address = stat_coll5_base_address;
        cur_event_mux_req = 10;
        cur_event_mux_resp = 11;
        gStatColState.stat5_filter_cnt = gStatColState.stat5_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat5_filter_cnt;
	global_object[instance_id].group_id = 5;
        break;
    case STATCOL_DMA_SYSTEM_WR:
        cur_base_address = stat_coll5_base_address;
        cur_event_mux_req = 12;
        cur_event_mux_resp = 13;
        gStatColState.stat5_filter_cnt = gStatColState.stat5_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat5_filter_cnt;
	global_object[instance_id].group_id = 5;
        break;
    case STATCOL_CSI2_1:
        cur_base_address = stat_coll5_base_address;
        cur_event_mux_req = 14;
        cur_event_mux_resp = 15;
        gStatColState.stat5_filter_cnt = gStatColState.stat5_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat5_filter_cnt;
	global_object[instance_id].group_id = 5;
        break;
    case STATCOL_USB3_SS:
        cur_base_address = stat_coll6_base_address;
        cur_event_mux_req = 0;
        cur_event_mux_resp = 1;
        gStatColState.stat6_filter_cnt = gStatColState.stat6_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat6_filter_cnt;
	global_object[instance_id].group_id = 6;
        break;
    case STATCOL_USB2_SS:
        cur_base_address = stat_coll6_base_address;
        cur_event_mux_req = 2;
        cur_event_mux_resp = 3;
        gStatColState.stat6_filter_cnt = gStatColState.stat6_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat6_filter_cnt;
	global_object[instance_id].group_id = 6;
        break;
    case STATCOL_USB2_ULPI_SS1:
        cur_base_address = stat_coll6_base_address;
        cur_event_mux_req = 4;
        cur_event_mux_resp = 5;
        gStatColState.stat6_filter_cnt = gStatColState.stat6_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat6_filter_cnt;
	global_object[instance_id].group_id = 6;
        break;
    case STATCOL_USB2_ULPI_SS2:
        cur_base_address = stat_coll6_base_address;
        cur_event_mux_req = 6;
        cur_event_mux_resp = 7;
        gStatColState.stat6_filter_cnt = gStatColState.stat6_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat6_filter_cnt;
	global_object[instance_id].group_id = 6;
        break;
    case STATCOL_PCIE_SS1:
        cur_base_address = stat_coll6_base_address;
        cur_event_mux_req = 8;
        cur_event_mux_resp = 9;
        gStatColState.stat6_filter_cnt = gStatColState.stat6_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat6_filter_cnt;
	global_object[instance_id].group_id = 6;
        break;
    case STATCOL_PCIE_SS2:
        cur_base_address = stat_coll6_base_address;
        cur_event_mux_req = 10;
        cur_event_mux_resp = 11;
        gStatColState.stat6_filter_cnt = gStatColState.stat6_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat6_filter_cnt;
	global_object[instance_id].group_id = 6;
        break;
    case STATCOL_DSP1_CFG:
        cur_base_address = stat_coll6_base_address;
        cur_event_mux_req = 12;
        cur_event_mux_resp = 13;
        gStatColState.stat6_filter_cnt = gStatColState.stat6_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat6_filter_cnt;
	global_object[instance_id].group_id = 6;
        break;
    case STATCOL_DSP2_CFG:
        cur_base_address = stat_coll6_base_address;
        cur_event_mux_req = 14;
        cur_event_mux_resp = 15;
        gStatColState.stat6_filter_cnt = gStatColState.stat6_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat6_filter_cnt;
	global_object[instance_id].group_id = 6;
        break;
    case STATCOL_GMAC_SW:
        cur_base_address = stat_coll7_base_address;
        cur_event_mux_req = 0;
        cur_event_mux_resp = 1;
        gStatColState.stat7_filter_cnt = gStatColState.stat7_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat7_filter_cnt;
	global_object[instance_id].group_id = 7;
        break;
    case STATCOL_PRUSS1_P1:
        cur_base_address = stat_coll7_base_address;
        cur_event_mux_req = 2;
        cur_event_mux_resp = 3;
        gStatColState.stat7_filter_cnt = gStatColState.stat7_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat7_filter_cnt;
	global_object[instance_id].group_id = 7;
        break;
    case STATCOL_PRUSS1_P2:
        cur_base_address = stat_coll7_base_address;
        cur_event_mux_req = 4;
        cur_event_mux_resp = 5;
        gStatColState.stat7_filter_cnt = gStatColState.stat7_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat7_filter_cnt;
	global_object[instance_id].group_id = 7;
        break;
    case STATCOL_PRUSS2_P1:
        cur_base_address = stat_coll7_base_address;
        cur_event_mux_req = 6;
        cur_event_mux_resp = 7;
        gStatColState.stat7_filter_cnt = gStatColState.stat7_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat7_filter_cnt;
	global_object[instance_id].group_id = 7;
        break;
    case STATCOL_PRUSS2_P2:
        cur_base_address = stat_coll7_base_address;
        cur_event_mux_req = 8;
        cur_event_mux_resp = 9;
        gStatColState.stat7_filter_cnt = gStatColState.stat7_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat7_filter_cnt;
	global_object[instance_id].group_id = 7;
        break;
    case STATCOL_DMA_CRYPTO_RD:
        cur_base_address = stat_coll7_base_address;
        cur_event_mux_req = 10;
        cur_event_mux_resp = 11;
        gStatColState.stat7_filter_cnt = gStatColState.stat7_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat7_filter_cnt;
	global_object[instance_id].group_id = 7;
        break;
    case STATCOL_DMA_CRYPTO_WR:
        cur_base_address = stat_coll7_base_address;
        cur_event_mux_req = 12;
        cur_event_mux_resp = 13;
        gStatColState.stat7_filter_cnt = gStatColState.stat7_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat7_filter_cnt;
	global_object[instance_id].group_id = 7;
        break;
    case STATCOL_MPU2:
        cur_base_address = stat_coll7_base_address;
        cur_event_mux_req = 14;
        cur_event_mux_resp = 15;
        gStatColState.stat7_filter_cnt = gStatColState.stat7_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat7_filter_cnt;
	global_object[instance_id].group_id = 7;
        break;
    case STATCOL_MMC1:
        cur_base_address = stat_coll8_base_address;
        cur_event_mux_req = 0;
        cur_event_mux_resp = 1;
        gStatColState.stat8_filter_cnt = gStatColState.stat8_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat8_filter_cnt;
	global_object[instance_id].group_id = 8;
        break;
    case STATCOL_MMC2:
        cur_base_address = stat_coll8_base_address;
        cur_event_mux_req = 2;
        cur_event_mux_resp = 3;
        gStatColState.stat8_filter_cnt = gStatColState.stat8_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat8_filter_cnt;
	global_object[instance_id].group_id = 8;
        break;
    case STATCOL_SATA:
        cur_base_address = stat_coll8_base_address;
        cur_event_mux_req = 4;
        cur_event_mux_resp = 5;
        gStatColState.stat8_filter_cnt = gStatColState.stat8_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat8_filter_cnt;
	global_object[instance_id].group_id = 8;
        break;
    case STATCOL_MLBSS:
        cur_base_address = stat_coll8_base_address;
        cur_event_mux_req = 6;
        cur_event_mux_resp = 7;
        gStatColState.stat8_filter_cnt = gStatColState.stat8_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat8_filter_cnt;
	global_object[instance_id].group_id = 8;
        break;
    case STATCOL_BB2D_P2:
        cur_base_address = stat_coll8_base_address;
        cur_event_mux_req = 8;
        cur_event_mux_resp = 9;
        gStatColState.stat8_filter_cnt = gStatColState.stat8_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat8_filter_cnt;
	global_object[instance_id].group_id = 8;
        break;
    case STATCOL_IEEE1500:
        cur_base_address = stat_coll8_base_address;
        cur_event_mux_req = 10;
        cur_event_mux_resp = 11;
        gStatColState.stat8_filter_cnt = gStatColState.stat8_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat8_filter_cnt;
	global_object[instance_id].group_id = 8;
        break;
    case STATCOL_DBG:
        cur_base_address = stat_coll8_base_address;
        cur_event_mux_req = 12;
        cur_event_mux_resp = 13;
        gStatColState.stat8_filter_cnt = gStatColState.stat8_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat8_filter_cnt;
	global_object[instance_id].group_id = 8;
        break;
    case STATCOL_VCP1:
        cur_base_address = stat_coll8_base_address;
        cur_event_mux_req = 14;
        cur_event_mux_resp = 15;
        gStatColState.stat8_filter_cnt = gStatColState.stat8_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat8_filter_cnt;
	global_object[instance_id].group_id = 8;
        break;
    case STATCOL_OCMC_RAM1:
        cur_base_address = stat_coll9_base_address;
        cur_event_mux_req = 0;
        cur_event_mux_resp = 1;
        gStatColState.stat9_filter_cnt = gStatColState.stat9_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat9_filter_cnt;
	global_object[instance_id].group_id = 9;
        break;
    case STATCOL_OCMC_RAM2:
        cur_base_address = stat_coll9_base_address;
        cur_event_mux_req = 2;
        cur_event_mux_resp = 3;
        gStatColState.stat9_filter_cnt = gStatColState.stat9_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat9_filter_cnt;
	global_object[instance_id].group_id = 9;
        break;
    case STATCOL_OCMC_RAM3:
        cur_base_address = stat_coll9_base_address;
        cur_event_mux_req = 4;
        cur_event_mux_resp = 5;
        gStatColState.stat9_filter_cnt = gStatColState.stat9_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat9_filter_cnt;
	global_object[instance_id].group_id = 9;
        break;
    case STATCOL_GPMC:
        cur_base_address = stat_coll9_base_address;
        cur_event_mux_req = 6;
        cur_event_mux_resp = 7;
        gStatColState.stat9_filter_cnt = gStatColState.stat9_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat9_filter_cnt;
	global_object[instance_id].group_id = 9;
        break;
    case STATCOL_MCASP1:
        cur_base_address = stat_coll9_base_address;
        cur_event_mux_req = 8;
        cur_event_mux_resp = 9;
        gStatColState.stat9_filter_cnt = gStatColState.stat9_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat9_filter_cnt;
	global_object[instance_id].group_id = 9;
        break;
    case STATCOL_MCASP2:
        cur_base_address = stat_coll9_base_address;
        cur_event_mux_req = 10;
        cur_event_mux_resp = 11;
        gStatColState.stat9_filter_cnt = gStatColState.stat9_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat9_filter_cnt;
	global_object[instance_id].group_id = 9;
        break;
    case STATCOL_MCASP3:
        cur_base_address = stat_coll9_base_address;
        cur_event_mux_req = 12;
        cur_event_mux_resp = 13;
        gStatColState.stat9_filter_cnt = gStatColState.stat9_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat9_filter_cnt;
	global_object[instance_id].group_id = 9;
        break;
    case STATCOL_VCP2:
        cur_base_address = stat_coll9_base_address;
        cur_event_mux_req = 14;
        cur_event_mux_resp = 15;
        gStatColState.stat9_filter_cnt = gStatColState.stat9_filter_cnt + 1;
        cur_stat_filter_cnt = gStatColState.stat9_filter_cnt;
	global_object[instance_id].group_id = 9;
        break;
    default:
       printf("ERROR: Unknown initiator\n");
       exit(0);
    };

    {
        if ( cur_stat_filter_cnt > 4 )
        {
            printf("WARNING: We have exhausted filters/counters.....\n");
            return 0;
        }
        // Global Enable Stat Collector
        wr_stat_reg(cur_base_address+0x8,0x1);

        // Soft Enable Stat Collector
        wr_stat_reg(cur_base_address+0xC,0x1);

        wr_stat_reg(cur_base_address+0x18,0x5);
        // Operation of Stat Collector / RespEvt => Packet
        wr_stat_reg(cur_base_address+0x1C,0x5);


        // Event Sel
        wr_stat_reg(cur_base_address+0x20+4*(cur_stat_filter_cnt-1),cur_event_mux_req);

        // Op is EventInfo
        wr_stat_reg(cur_base_address+0x1FC+(0x158*(cur_stat_filter_cnt-1)),2);

        // Event Info Sel Op -> packet length
        wr_stat_reg(cur_base_address+0x1F8+(0x158*(cur_stat_filter_cnt-1)),0);

        // Filter Global Enable
        wr_stat_reg(cur_base_address+0xAC+(0x158*(cur_stat_filter_cnt-1)),0x1);

        // Filter Enable
        wr_stat_reg(cur_base_address+0xBC+(0x158*(cur_stat_filter_cnt-1)),0x1);

        // Manual dump
        wr_stat_reg(cur_base_address+0x54,0x1);
        // use send register to reset counters

    }

    global_object[instance_id].mux_req = cur_event_mux_req;
    global_object[instance_id].base_address = cur_base_address;
    global_object[instance_id].counter_id = cur_stat_filter_cnt;
    global_object[instance_id].b_enabled = 1;

    return cur_stat_filter_cnt;
}



void statCollectorReadGroup(UInt32 group_id)
{
    int i=0;
    UInt32 cur_base_address = 0x45001000 + ((group_id - 1) * 0x1000);

    wr_stat_reg(cur_base_address+0xC,0x0);

    for(i=0; i < STATCOL_MAX; i++)
    {
	if(global_object[i].group_id == (group_id - 1) && 
	   global_object[i].b_enabled == 1)
	{
	    UInt32 cur_stat_filter_cnt = global_object[i].counter_id;

    	    global_object[i].readings[statCountIdx] = rd_stat_reg(cur_base_address+0x8C+((cur_stat_filter_cnt-1)*4));
        }
    }

    wr_stat_reg(cur_base_address+0xC,0x1);
}


UInt32 statcoll_start(UInt32 TOTAL_TIME, UInt32 INTERVAL_US, char list[][50])
{
    int i, fd, index;
    UInt32 counterIdDss, counterIdIva, counterIdBB2dP1, counterIdBB2dP2, counterIdUsb4, counterIdSata, counterIdEmif1, counterIdEmif2;

  
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    printf("------------------------------------------------\n");
    printf("Compile time = %s %s\n",__DATE__,  __TIME__);
    printf("------------------------------------------------\n\n");
    //printd("Start time = %d\n", time(NULL));
    //printd("Time seconds = %d, usecs = %d\n", tv.tv_sec, tv.tv_usec);

    statcoll_params params;
    memset(&params, sizeof(params), 0);
    params.INTERVAL_US = INTERVAL_US;
    params.TOTAL_TIME = TOTAL_TIME;

    i=0;
    index=0;
    while(list[i][0] != 0)
    {
	for(index=0; index< STATCOL_MAX; index++) {
		if(strcmp(list[i], initiators[index].name) == 0)
		{
			strcpy(params.user_config_list[params.no_of_initiators].name, list[i]);
			params.user_config_list[params.no_of_initiators++].id = initiators[index].id;
			break;
		}
	}

	if(index == STATCOL_MAX) {
		printf("ERROR: Unknown initiator \n");
		exit(0);
	}
        i++;
    }

    printf("Total configured initiators = %d\n", params.no_of_initiators);
	

    fd = open("/dev/mem", O_RDWR);
    if (fd == -1){
       printf("error fd=open() \n");     
       return -1;
    }
    statcoll_base_mem = mmap(NULL, STATCOLL_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, STATCOLL_BASE);
    
   if (statcoll_base_mem == MAP_FAILED){
        printf("ERROR: mmap failed \n");
        return;
    }
    close(fd);

    fd = open("/dev/mem", O_RDWR);
    if (fd == -1){
       printf("error fd=open() \n");     
       return -1;
    }
    l3_3_clkctrl = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, CM_L3INSTR_REGISTER_BASE);
    if (l3_3_clkctrl == MAP_FAILED){
        printf("ERROR: mmap failed for CM_L3INSTR_REGISTER_BASE\n");
        return;
    }
    close(fd);

    printf("SUCCESS: Mapped 0x%x to user space address 0x%x\n", STATCOLL_BASE, statcoll_base_mem);
    printf("INTERVAL = %d usecs\n", INTERVAL_US);
    printf("TOTAL TIME = %d seconds\n", TOTAL_TIME);
    TRACE_SZ = (TOTAL_TIME * 1000000)/INTERVAL_US;
    printf("TRACE SIZE = %d samples\n", TRACE_SZ);

    printf("**************************************\n");
    printf("Going to initialize the L3 clocks \n"); 
    l3_3_clkctrl[CM_L3INSTR_L3INSTR_CLKSTCTRL_OFFSET >> 2] = 0x2;
    l3_3_clkctrl[CM_L3INSTR_L3_MAIN_2_CLKCTRL_OFFSET >> 2] = 0x1;
    printf("**************************************\n");

    while( (l3_3_clkctrl[CM_L3INSTR_L3_MAIN_2_CLKCTRL_OFFSET >> 2] & 0x30000) != 0x0) 
    {
	printf("Waiting on module to be functional\n");
    }

    statCollectorInit();

    printf("SUCCESS: Initialized STAT COLLECTOR\n");
    /* Initialize all enabled initiators */
    for(index =0; index < params.no_of_initiators; index++) {
        printf("\t\t Initialized %s\n", params.user_config_list[index].name);
        statCollectorControlInitialize(params.user_config_list[index].id);
    }

    while(statCountIdx != (TRACE_SZ - 1))
    {
        usleep(INTERVAL_US);
        int group;
	for(group = 1; group<11; group++)
		statCollectorReadGroup(group);

	statCountIdx++;
    }
    
    printf("------------------------------------------------\n\n");
    printf("SUCCESS: Stat collection completed... Writing into file now\n");
    FILE *outfile = fopen("statcollector.csv", "w+");
    if (!outfile) {
        printf("\n ERROR: Error opening file");
    }

    /* Ignore the first index at 0 */
    for(index=1; index<statCountIdx; index++) {
	    for(i=0; i<params.no_of_initiators; i++) {
		    fprintf(outfile,"%s = %d,", params.user_config_list[i].name, global_object[params.user_config_list[i].id].readings[index]);
	    }
	    fprintf(outfile,"\n"); 
    }
    fclose(outfile);

    gettimeofday(&tv2, NULL);
    //printf("End time = %d\n", time(NULL));
    //printf("Time seconds = %d, usecs = %d\n", tv.tv_sec, tv.tv_usec);
    printf("Total execution time = %d secs, %d usecs\n\n", (tv2.tv_sec - tv1.tv_sec), (tv2.tv_usec - tv2.tv_usec));

    return 0;
}


