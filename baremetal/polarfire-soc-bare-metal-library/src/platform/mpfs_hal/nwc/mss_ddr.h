/*******************************************************************************
 * Copyright 2019-2020 Microchip FPGA Embedded Systems Solutions.
 *
 * SPDX-License-Identifier: MIT
 *
 * MPFS HAL Embedded Software
 *
 */

/*******************************************************************************
 * @file mss_ddr.h
 * @author Microchip-FPGA Embedded Systems Solutions
 * @brief DDR related defines
 *
 */

/*=========================================================================*//**
  @page DDR setup and monitoring
  ==============================================================================
  @section intro_sec Introduction
  ==============================================================================
  The MPFS microcontroller subsystem (MSS) includes a number of hard core
  components physically located in the north west corner of the MSS on the die.
  This includes the DDR Phy.

  ==============================================================================
  @section Items located in the north west corner
  ==============================================================================
  - MSS PLL
  - SGMII
  - DDR phy
  - MSSIO

  ==============================================================================
  @section Overview of DDR related hardware
  ==============================================================================

  Simplified IP diagram


                                                  +--+
                  +--++ +-----+        +---v--+   |o |
                  |H0 | |H1-H4+--------> AXI  |   |t |
                  ++--+ +-+---+        |switch<---+h |
                   |      |            |      |   |e |
                   |      |            +--+---+   |r |
                  +v------v-------+       |       |  |
                  |L2 cache       |     non-cache |m |
                  +------------+--+       |       |a |
                           +---v----+ +---v---+   |s |
                           |seg 0   | | seg 1 |   |t |
                           +----^---+ +---^---+   |e |
                                |         |       |r |
        +-----------+------+----v---------v---+   |s |
        |Training IP|MTC   |DDR controller    |   +--+
        +-----------+------+--------+---------+
                                    |DFI
                                    |
                          +---------v--------+
                          | DDR Phy          |
                          +------------------+
                          | Bank 6 I/O       |
                          +-------+----------+
                                  |
                       +----------v---------------+
                       | +--+ +--+ +--+ +--+ +--+ |
                       | |D | |D | |R | |  | |  | |
                       | +--+ +--+ +--+ +--+ +--+ |
                       +--------------------------+


  -----------
  Hart0 E51
  -----------
  In most systems, the E51 will will setup and monitor the DDR

  -----------
  L2 Cache
  -----------
  Specific address range is used to access DDR via cache

  -----------
  AXI switch
  -----------
  DDR access via AXI switch for non-cached read/write

  -----------
  SEG regs
  -----------
  Used to map internal DDR address range to external fixed mapping.
  Note: DDR address ranges are at 32 bit and 64 bits

  -----------
  DDR controller
  -----------
  Manages DDR, refresh rates etc

  -----------
  DDR Training IP
  -----------
  Used to carry out training using IP state machines
   - BCLKSCLK_TIP_TRAINING .
   - addcmd_TIP_TRAINING
   - wrlvl_TIP_TRAINING
   - rdgate_TIP_TRAINING
   - dq_dqs_opt_TIP_TRAINING

  -----------
  DDR MTC - Memory test controller
  -----------
  Sends/receives test patterns to DDR. More efficient than using software.
  Used during write calibration and in DDR test routines.

  -----------
  DFI
  -----------
  Industry standard interface between phy, DDRC

  -----------
  DDR phy
  -----------
  PolarFire-SoC DDR phy manges data paath between pins and DFI

  ==============================================================================
  @section Overview of DDR embedded software
  ==============================================================================

  -----------
  Setup
  -----------
      - phy and IO
      - DDRC

  -----------
  Use Training IP
  -----------
      - kick-off RTL training IP state machine
      - Verify all training complete
            - BCLKSCLK_TIP_TRAINING .
            - addcmd_TIP_TRAINING
              This is a coarse training that moves the DDRCLK with PLL phase
              rotations in relation to the Address/Command bits to achieve the
              desired offset on the FPGA side.
            - wrlvl_TIP_TRAINING
            - rdgate_TIP_TRAINING
            - dq_dqs_opt_TIP_TRAINING

  Test this reg to determine training status:
  DDRCFG->DFI.STAT_DFI_TRAINING_COMPLETE.STAT_DFI_TRAINING_COMPLETE;

  -----------
  Write Calibration
  -----------
  The Memory Test Core plugged in to the front end of the DDR controller is used
  to perform lane-based writes and read backs and increment write calibration
  offset for each lane until data match occurs. The settings are recorded by the
  driver and available to be read using by an API function call.

  -----------
  VREF Calibration (optional)
  -----------
  VREF (DDR4 + LPDDR4 only) Set Remote VREF via mode register writes (MRW).
  In DDR4 and LPDDR4, VREF training may be done by writing to Mode Register 6 as
  defined by the JEDEC spec and, for example, Micron’s datasheet for its 4Gb
  DDR4 RAM’s:

  MR6 register definition from DDR4 datasheet

  | Mode Reg      | Description                                         |
  | ------------- |:---------------------------------------------------:|
  | 13,9,8        | DQ RX EQ must be 000                                |
  | 7             | Vref calib enable = => disables, 1 ==> enabled      |
  | 6             | Vref Calibration range 0 = Range 0, 1 - Range 2     |
  | 5:0           | Vref Calibration value                              |

  This step is not implemented in the current driver. It can be implemented in
  the same way as write Calibration and will be added during board verification.

  -----------
  FPGA VREF (Local VREF training) (optional)
  -----------
  In addition to memory VREFDQ training, or remote training, it is possible to
  train the VREFDQ on the FPGA device. WE refer to this training as local VREF
  training.
  Train FPGA VREF using the vrgen_h and vrgen_v registers
  To manipulate the FPGA VREF value, firmware must write to the DPC_BITS
  register, located at physical address 0x2000 7184.
        CFG_DDR_SGMII_PHY->DPC_BITS.bitfield.dpc_vrgen_h;
        CFG_DDR_SGMII_PHY->DPC_BITS.bitfield.dpc_vrgen_v;
  Like memory VREFDQ training, FPGA VREFDQ training seeks to find an average/
  optimal VREF value by sweeping across the full range and finding a left edge
  and a right edge.
  This step is not implemented in the current driver. It can be implemented in
  the same way as write Calibration and will be added during board verification.

  -----------
  DQ Write Offset
  -----------
  (LPDDR4 only) ), there must be an offset at the input to the LPDDR4 memory
  device between DQ and DQS. Unlike other flavors of DDR, which match DQ and DQS
  at the SDRAM, for LPDDR4 this relationship must be trained, because it will
  vary between 200ps and 600ps, which, depending on the data rate, could be as
  much as one whole bit period.
  This training is integrated with write calibration, because it, too, is done
  on a per-lane basis. That is, each lane is trained separately by sweeping the
  DQ output delay to find a valid range and of DQ output delays and center it.
  DQ output delays are swept using the expert_dlycnt_move_reg0 register located
  in the MSS DDR TIP.


  -----------
  Overview Flow diagram of Embedded software setup
  -----------

               +--------------------------------------------+
               |      Some  Preconditions                   |
               |   DCE, CORE_UP, FLASH VALID, MSS_IO_EN     |
               |   MSS PLL setup, Clks to MSS setup         |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |   Check if in off mode, ret if so          |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  set ddr mode and VS bits                  |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  soft reset I/O decoders                   |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Set RPC registers that need manual setup  |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Soft reset IP- to load RPC ->SCB regs     |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Calibrate I/O - as they are now setup     |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Configure the DDR PLL - Using SCB writes  |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Setup the SEG regs - NB May move this down|
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Set-up the DDRC - Using Libero values     |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Reset training IP                         |
               +--------------------+-----------------------+
                                    |
               +----------------- --v-----------------------+
               |  Rotate BCLK by programmed amount (degrees)|
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Set training parameters                   |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Assert traing reset                       |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Wait until traing complete                |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Write calibrate                           |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  If LPDDR4, calibrate DQ                   |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Sanity check training                     |
               +--------------------+-----------------------+
                                    |
               +--------------------v-----------------------+
               |  Return 0 if all went OK                   |
               +--------------------------------------------+

 *//*=========================================================================*/


#ifndef __MSS_DDRC_H_
#define __MSS_DDRC_H_ 1

#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**

 */
typedef enum DDR_TYPE_
{

    DDR3                                = 0x00,         /*!< 0 DDR3          */
    DDR3L                               = 0x01,         /*!< 1 DDR3L         */
    DDR4                                = 0x02,         /*!< 2 DDR4          */
    LPDDR3                              = 0x03,         /*!< 3 LPDDR3        */
    LPDDR4                              = 0x04,         /*!< 4 LPDDR4        */
    DDR_OFF_MODE                        = 0x07          /*!< 4 LPDDR4        */
} DDR_TYPE;

typedef enum DDR_MEMORY_ACCESS_
{
    DDR_NC_256MB,
    DDR_NC_WCB_256MB,
    DDR_NC_2GB,
    DDR_NC_WCB_2GB,
} DDR_MEMORY_ACCESS;

/* this is a fixed value, currently only 5 supported in the TIP  */
#define MAX_POSSIBLE_TIP_TRAININGS    0x05U

/* LIBERO_SETTING_TIP_CFG_PARAMS
 *     ADDCMD_OFFSET                     [0:3]   RW value */
#define ADDRESS_CMD_OFFSETT_MASK        (0x7U<<0U)

#define BCLK_SCLK_OFFSET_SHIFT                (3U)
#define BCLK_SCLK_OFFSET_MASK           (0x7U<<3U)

#define BCLK_DPC_VRGEN_V_SHIFT                (12U)
#define BCLK_DPC_VRGEN_V_MASK          (0x3FU<<12U)

#define BCLK_DPC_VRGEN_H_SHIFT                (4U)
#define BCLK_DPC_VRGEN_H_MASK           (0xFU<<4U)

#define BCLK_DPC_VRGEN_VS_SHIFT                (0U)
#define BCLK_DPC_VRGEN_VS_MASK           (0xFU<<0U)


/* masks and associated values used with  DDRPHY_MODE register */
#define DDRPHY_MODE_MASK                0x7U
/* ECC */
#define DDRPHY_MODE_ECC_MASK            (0x1U<<3U)
#define DDRPHY_MODE_ECC_ON              (0x1U<<3U)
/* Bus width */
#define DDRPHY_MODE_BUS_WIDTH_4_LANE    (0x1U<<5U)
#define DDRPHY_MODE_BUS_WIDTH_MASK      (0x7U<<5U)
/* Number of ranks, 1 or 2 supported */
#define DDRPHY_MODE_RANK_MASK           (0x1U<<26U)
#define DDRPHY_MODE_ONE_RANK            (0x0U<<26U)
#define DDRPHY_MODE_TWO_RANKS           (0x1U<<26U)

#define DMI_DBI_MASK                    (~(0x1U<<8U))

/* Write latency min/max settings If write calibration fails
 * For Libero setting, we iterate through these values looking for a
 * Calibration pass */
#define MIN_LATENCY                     0UL
#define MAX_LATENCY                     8UL

#define DDR_MODE_REG_VREF               0xCU

#define CALIBRATION_PASSED              0xFF
#define CALIBRATION_FAILED              0xFE
#define CALIBRATION_SUCCESS             0xFC

/*
 * Some settings that are only used during testing in new DDR setup
 */
/* #define LANE_ALIGNMENT_RESET_REQUIRED leave commented, not required */
#define ABNORMAL_RETRAIN_CA_DECREASE_COUNT          2U
#define ABNORMAL_RETRAIN_CA_DLY_DECREASE_COUNT      2U
#define DQ_DQS_NUM_TAPS                             5U
//#define SW_CONFIG_LPDDR_WR_CALIB_FN

#if !defined (LIBERO_SETTING_MANUAL_REF_CLK_PHASE_OFFSET)
/* If skipping add/cmd training, this value is used */
/* The value used may be trained. The value here should be determined */
/* for the board design by performing a manual sweep. */
#define LIBERO_SETTING_MANUAL_REF_CLK_PHASE_OFFSET    0x00000006UL
    /* CA_BUS_RX_OFF_POST_TRAINING       [0:1]   RW value= 0x1 */
#endif

/*
 * We currently need at least one retrain, otherwise driver can get stuck in
 * sanity check state
 */
#if !defined (EN_RETRY_ON_FIRST_TRAIN_PASS)
#define EN_RETRY_ON_FIRST_TRAIN_PASS    0
#endif

#if !defined (DDR_FULL_32BIT_NC_CHECK_EN)
#define DDR_FULL_32BIT_NC_CHECK_EN  1
#endif

/* This is a fixed setting, will move into driver in next commit */
#if !defined (SW_TRAING_BCLK_SCLK_OFFSET)
#define SW_TRAING_BCLK_SCLK_OFFSET                  0x00000000UL
#endif
/*
 * 0x6DU => setting vref_ca to 40%
 * This (0x6DU) is the default setting.
 * Currently not being used, here for possible future use.
 * */
#if !defined (DDR_MODE_REG_VREF_VALUE)
#define DDR_MODE_REG_VREF_VALUE       0x6DU
#endif

/* number of test writes to perform */
#if !defined (SW_CFG_NUM_READS_WRITES)
#define SW_CFG_NUM_READS_WRITES        0x20000U
#endif
/*
 * what test patterns to write/read on start-up
 * */
#if !defined (SW_CONFIG_PATTERN)
#define SW_CONFIG_PATTERN (PATTERN_INCREMENTAL|\
                                        PATTERN_WALKING_ONE|\
                                        PATTERN_WALKING_ZERO|\
                                        PATTERN_RANDOM|\
                                        PATTERN_0xCCCCCCCC|\
                                        PATTERN_0x55555555)
#endif


/***************************************************************************//**

 */
typedef enum DDR_SM_STATES_
{

    DDR_STATE_INIT         = 0x00,     /*!< 0 DDR_STATE_INIT*/
    DDR_STATE_MONITOR      = 0x01,     /*!< 1 DDR_STATE_MONITOR */
    DDR_STATE_TRAINING     = 0x02,     /*!< 2 DDR_STATE_TRAINING */
    DDR_STATE_VERIFY       = 0x03,     /*!< 3 DDR_STATE_VERIFY */
} DDR_SM_STATES;

/***************************************************************************//**

 */
typedef enum DDR_SS_COMMAND_
{

    DDR_SS__INIT        = 0x00,     /*!< 0 DDR_SS__INIT */
    DDR_SS_MONITOR      = 0x01,     /*!< 1 DDR_SS_MONITOR */
} DDR_SS_COMMAND;


/***************************************************************************//**

 */
typedef enum DDR_SS_STATUS_
{

    DDR_SETUP_DONE      = 0x01,      /*!< 0 DDR_SETUP_DONE */
    DDR_SETUP_FAIL      = 0x02,      /*!< 1 DDR_SETUP_FAIL */
    DDR_SETUP_SUCCESS   = 0x04,      /*!< 2 DDR_SETUP_SUCCESS */
    DDR_SETUP_OFF_MODE  = 0x08,      /*!< 4 DDR_SETUP_OFF_MODE */
} DDR_SS_STATUS;


/***************************************************************************//**

 */
typedef enum DDR_TRAINING_SM_
{

    DDR_TRAINING_INIT,              /*!< DDR_TRAINING_INIT */
    DDR_TRAINING_FAIL,
    DDR_CHECK_TRAINING_SWEEP,
    DDR_TRAINING_SWEEP,
    DDR_TRAINING_CHECK_FOR_OFFMODE, /*!< DDR_TRAINING_OFFMODE */
    DDR_TRAINING_SET_MODE_VS_BITS,
    DDR_TRAINING_FLASH_REGS,
    DDR_TRAINING_CORRECT_RPC,
    DDR_TRAINING_SOFT_RESET,
    DDR_TRAINING_CALIBRATE_IO,
    DDR_TRAINING_CONFIG_PLL,
    DDR_TRAINING_SETUP_SEGS,
    DDR_TRAINING_VERIFY_PLL_LOCK,
    DDR_TRAINING_SETUP_DDRC,
    DDR_TRAINING_RESET,
    DDR_TRAINING_ROTATE_CLK,
    DDR_TRAINING_SET_TRAINING_PARAMETERS,
    DDR_TRAINING_IP_SM_BCLKSCLK_SW,
    DDR_TRAINING_IP_SM_START,
    DDR_TRAINING_IP_SM_START_CHECK,
    DDR_TRAINING_IP_SM_BCLKSCLK,
    DDR_TRAINING_IP_SM_ADDCMD,
    DDR_TRAINING_IP_SM_WRLVL,
    DDR_TRAINING_IP_SM_RDGATE,
    DDR_TRAINING_IP_SM_DQ_DQS,
    DDR_TRAINING_IP_SM_VERIFY,
    DDR_TRAINING_SET_FINAL_MODE,
    DDR_TRAINING_WRITE_CALIBRATION,
    DDR_TRAINING_WRITE_CALIBRATION_RETRY, /*!< Retry on calibration fail */
    DDR_SWEEP_CHECK,
    DDR_SANITY_CHECKS,
    DDR_FULL_MTC_CHECK,
    DDR_FULL_32BIT_NC_CHECK,
    DDR_FULL_32BIT_CACHE_CHECK,
    DDR_FULL_32BIT_WRC_CHECK,
    DDR_FULL_64BIT_NC_CHECK,
    DDR_FULL_64BIT_CACHE_CHECK,
    DDR_FULL_64BIT_WRC_CHECK,
    DDR_TRAINING_VREFDQ_CALIB,
    DDR_TRAINING_FPGA_VREFDQ_CALIB,
    DDR_TRAINING_FINISH_CHECK,
    DDR_TRAINING_FINISHED,
    DDR_TRAINING_FAIL_SM2_VERIFY,
    DDR_TRAINING_FAIL_SM_VERIFY,
    DDR_TRAINING_FAIL_SM_DQ_DQS,
    DDR_TRAINING_FAIL_SM_RDGATE,
    DDR_TRAINING_FAIL_SM_WRLVL,
    DDR_TRAINING_FAIL_SM_ADDCMD,
    DDR_TRAINING_FAIL_SM_BCLKSCLK,
    DDR_TRAINING_FAIL_BCLKSCLK_SW,
    DDR_TRAINING_FAIL_FULL_32BIT_NC_CHECK,
    DDR_TRAINING_FAIL_MIN_LATENCY,
    DDR_TRAINING_FAIL_START_CHECK,
    DDR_TRAINING_FAIL_PLL_LOCK,
    DDR_TRAINING_FAIL_DDR_SANITY_CHECKS,
    DDR_SWEEP_AGAIN
} DDR_TRAINING_SM;


/***************************************************************************//**

 */
typedef enum {

    USR_CMD_GET_DDR_STATUS      = 0x00,    //!< USR_CMD_GET_DDR_STATUS
    USR_CMD_GET_MODE_SETTING    = 0x01,    //!< USR_CMD_GET_MODE_SETTING
    USR_CMD_GET_W_CALIBRATION   = 0x02,    //!< USR_CMD_GET_W_CALIBRATION
    USR_CMD_GET_GREEN_ZONE      = 0x03,    //!< USR_CMD_GET_GREEN_ZONE
    USR_CMD_GET_REG             = 0x04     //!< USR_CMD_GET_REG
} DDR_USER_GET_COMMANDS_t;

/***************************************************************************//**

 */
typedef enum {
    USR_CMD_SET_GREEN_ZONE_DQ        = 0x80,   //!< USR_CMD_SET_GREEN_ZONE_DQ
    USR_CMD_SET_GREEN_ZONE_DQS       = 0x81,   //!< USR_CMD_SET_GREEN_ZONE_DQS
    USR_CMD_SET_GREEN_ZONE_VREF_MAX  = 0x82,   //!< USR_CMD_SET_GREEN_ZONE_VREF
    USR_CMD_SET_GREEN_ZONE_VREF_MIN  = 0x83,   //!< USR_CMD_SET_GREEN_ZONE_VREF
    USR_CMD_SET_RETRAIN              = 0x84,   //!< USR_CMD_SET_RETRAIN
    USR_CMD_SET_REG                  = 0x85    //!< USR_CMD_SET_REG
} DDR_USER_SET_COMMANDS_t;

/***************************************************************************//**

 */
typedef enum SWEEP_STATES_{
    INIT_SWEEP,                     //!< start the sweep
    ADDR_CMD_OFFSET_SWEEP,          //!< sweep address command
    BCLK_SCLK_OFFSET_SWEEP,         //!< sweep bclk sclk
    DPC_VRGEN_V_SWEEP,              //!< sweep vgen_v
    DPC_VRGEN_H_SWEEP,              //!< sweep vgen_h
    DPC_VRGEN_VS_SWEEP,             //!< VS sweep
    FINISHED_SWEEP,                 //!< finished sweep
} SWEEP_STATES;

/***************************************************************************//**

 */
typedef enum {
  USR_OPTION_tip_register_dump    = 0x00     //!< USR_OPTION_tip_register_dump
} USR_STATUS_OPTION_t;


#define MAX_LANES  5

/***************************************************************************//**

 */
typedef struct mss_ddr_fpga_vref_{
    uint32_t    status_lower;
    uint32_t    status_upper;
  uint32_t  lower;
  uint32_t  upper;
  uint32_t    vref_result;
} mss_ddr_vref;

/***************************************************************************//**

 */
typedef struct mss_ddr_write_calibration_{
    uint32_t    status_lower;
    uint32_t    lower[MAX_LANES];
    uint32_t    lane_calib_result;
} mss_ddr_write_calibration;

/***************************************************************************//**

 */
typedef struct mss_lpddr4_dq_calibration_{
    uint32_t    lower[MAX_LANES];
    uint32_t    upper[MAX_LANES];
    uint32_t    calibration_found[MAX_LANES];
} mss_lpddr4_dq_calibration;


/***************************************************************************//**
  Calibration settings derived during write training
 */
typedef struct mss_ddr_calibration_{
  /* CMSIS related defines identifying the UART hardware. */
    mss_ddr_write_calibration write_cal;
    mss_lpddr4_dq_calibration dq_cal;
  mss_ddr_vref fpga_vref;
  mss_ddr_vref mem_vref;
} mss_ddr_calibration;

/***************************************************************************//**
  sweep index's
 */
typedef struct sweep_index_{
    uint8_t cmd_index;
    uint8_t bclk_sclk_index;
    uint8_t dpc_vgen_index;
    uint8_t dpc_vgen_h_index;
    uint8_t dpc_vgen_vs_index;
} sweep_index;

/***************************************************************************//**

 */
uint8_t
MSS_DDR_init_simulation
(
    void
);

/***************************************************************************//**

 */
uint8_t
MSS_DDR_training
(
    uint8_t ddr_type
);

/***************************************************************************//**
  The ddr_state_machine() function runs a state machine which initializes and
  monitors the DDR

  @return
    This function returns status, see DDR_SS_STATUS enum

  Example:
  @code

        uint32_t  ddr_status;
        ddr_status = ddr_state_machine(DDR_SS__INIT);

        while((ddr_status & DDR_SETUP_DONE) != DDR_SETUP_DONE)
        {
            ddr_status = ddr_state_machine(DDR_SS_MONITOR);
        }
        if ((ddr_status & DDR_SETUP_FAIL) != DDR_SETUP_FAIL)
        {
            error |= (0x1U << 2U);
        }

  @endcode

 */
uint32_t
ddr_state_machine
(
    DDR_SS_COMMAND command
);

/***************************************************************************//**
  The debug_read_ddrcfg() prints out the ddrcfg register values

  @return
    This function returns status, see DDR_SS_STATUS enum

  Example:
  @code

      debug_read_ddrcfg();

  @endcode

 */
void
debug_read_ddrcfg
(
    void
);


#ifdef __cplusplus
}
#endif

#endif /* __MSS_DDRC_H_ */


