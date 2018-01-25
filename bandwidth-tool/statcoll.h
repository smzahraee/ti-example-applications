#ifndef __STATCOLL_H 
#define __STATCOLL_H
 

#define CM_L3INSTR_REGISTER_BASE           (0x4A008000)

#define CM_L3INSTR_L3INSTR_CLKSTCTRL_OFFSET     (0xE00)
#define CM_L3INSTR_L3_MAIN_2_CLKCTRL_OFFSET     (0xE20)

#define STATCOLL_SIZE 40960
#define STATCOLL_BASE (0x45001000)

#define stat_coll0_base_address (0x45001000)
#define stat_coll1_base_address (0x45002000)
#define stat_coll2_base_address (0x45003000)
#define stat_coll3_base_address (0x45004000)
#define stat_coll4_base_address (0x45005000)
#define stat_coll5_base_address (0x45006000)
#define stat_coll6_base_address (0x45007000)
#define stat_coll7_base_address (0x45008000)
#define stat_coll8_base_address (0x45009000)
#define stat_coll9_base_address (0x4500a000)

#define printd(fmt, ...) \
	do { if (debug) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

typedef unsigned int UInt32;


typedef enum
{
    STATCOL_EMIF1_SYS,
    STATCOL_EMIF2_SYS,
    STATCOL_MA_MPU_P1,
    STATCOL_MA_MPU_P2,
    STATCOL_MPU1,
    STATCOL_MMU1,
    STATCOL_TPTC_RD1,
    STATCOL_TPTC_WR1,
    STATCOL_TPTC_RD2,
    STATCOL_TPTC_WR2,
    STATCOL_VIP1_P1,
    STATCOL_VIP1_P2,
    STATCOL_VIP2_P1,
    STATCOL_VIP2_P2,
    STATCOL_VIP3_P1,
    STATCOL_VIP3_P2,
    STATCOL_VPE_P1,
    STATCOL_VPE_P2,
    STATCOL_EVE1_TC0,
    STATCOL_EVE1_TC1,
    STATCOL_EVE2_TC0,
    STATCOL_EVE2_TC1,
    STATCOL_EVE3_TC0,
    STATCOL_EVE3_TC1,
    STATCOL_EVE4_TC0,
    STATCOL_EVE4_TC1,
    STATCOL_DSP1_MDMA,
    STATCOL_DSP1_EDMA,
    STATCOL_DSP2_MDMA,
    STATCOL_DSP2_EDMA,
    STATCOL_IVA,
    STATCOL_GPU_P1,
    STATCOL_GPU_P2,
    STATCOL_BB2D_P1,
    STATCOL_DSS,
    STATCOL_CSI2_2,
    STATCOL_MMU2,
    STATCOL_IPU1,
    STATCOL_IPU2,
    STATCOL_DMA_SYSTEM_RD,
    STATCOL_DMA_SYSTEM_WR,
    STATCOL_CSI2_1,
    STATCOL_USB3_SS,
    STATCOL_USB2_SS,
    STATCOL_USB2_ULPI_SS1,
    STATCOL_USB2_ULPI_SS2,
    STATCOL_PCIE_SS1,
    STATCOL_PCIE_SS2,
    STATCOL_DSP1_CFG,
    STATCOL_DSP2_CFG,
    STATCOL_GMAC_SW,
    STATCOL_PRUSS1_P1,
    STATCOL_PRUSS1_P2,
    STATCOL_PRUSS2_P1,
    STATCOL_PRUSS2_P2,
    STATCOL_DMA_CRYPTO_RD,
    STATCOL_DMA_CRYPTO_WR,
    STATCOL_MPU2,
    STATCOL_MMC1,
    STATCOL_MMC2,
    STATCOL_SATA,
    STATCOL_MLBSS,
    STATCOL_BB2D_P2,
    STATCOL_IEEE1500,
    STATCOL_DBG,
    STATCOL_VCP1,
    STATCOL_OCMC_RAM1,
    STATCOL_OCMC_RAM2,
    STATCOL_OCMC_RAM3,
    STATCOL_GPMC,
    STATCOL_MCASP1,
    STATCOL_MCASP2,
    STATCOL_MCASP3,
    STATCOL_VCP2,
    STATCOL_MAX
} STATCOL_ID;



typedef struct
{
    UInt32 stat0_filter_cnt;
    UInt32 stat1_filter_cnt;
    UInt32 stat2_filter_cnt;
    UInt32 stat3_filter_cnt;
    UInt32 stat4_filter_cnt;
    UInt32 stat5_filter_cnt;
    UInt32 stat6_filter_cnt;
    UInt32 stat7_filter_cnt;
    UInt32 stat8_filter_cnt;
    UInt32 stat9_filter_cnt;
} StatCollectorObj;
 
struct list_of_initiators
{
   STATCOL_ID id;
   char name[50];    
};

typedef struct 
{
    UInt32 INTERVAL_US;
    UInt32 TOTAL_TIME;
    UInt32 no_of_initiators;
    struct list_of_initiators user_config_list[STATCOL_MAX];
} statcoll_params;

typedef struct
{
    UInt32 b_enabled;
    char name[100];
    UInt32 *readings;
    UInt32 *timestamp;
    UInt32 group_id;
    UInt32 counter_id;
    UInt32 base_address;
    UInt32 mux_req;
}statcoll_initiators_object;

UInt32 statcoll_start(UInt32 TOTAL_TIME, UInt32 INTERVAL_US, char list[][50]);

#endif
