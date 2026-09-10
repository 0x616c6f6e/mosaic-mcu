#ifndef TEST_FAKE_CH58X_COMMON_H
#define TEST_FAKE_CH58X_COMMON_H

#include <stdint.h>

typedef enum {
    DISABLE = 0,
    ENABLE = 1,
} FunctionalState;

typedef enum {
    USB_IRQn = 22,
} IRQn_Type;

#define __HIGH_CODE
#define __INTERRUPT

#define MASK_UC_SYS_CTRL  UINT8_C(0x30)
#define RB_UC_SYS_CTRL0   UINT8_C(0x10)
#define RB_UC_DEV_PU_EN   UINT8_C(0x20)
#define RB_UC_INT_BUSY    UINT8_C(0x08)
#define RB_UC_DMA_EN      UINT8_C(0x01)

#define RB_UD_PD_DIS      UINT8_C(0x80)
#define RB_UD_LOW_SPEED   UINT8_C(0x04)
#define RB_UD_PORT_EN     UINT8_C(0x01)

#define RB_UIE_FIFO_OV    UINT8_C(0x10)
#define RB_UIE_SUSPEND    UINT8_C(0x04)
#define RB_UIE_TRANSFER   UINT8_C(0x02)
#define RB_UIE_BUS_RST    UINT8_C(0x01)

#define RB_UIF_FIFO_OV    UINT8_C(0x10)
#define RB_UIF_SUSPEND    UINT8_C(0x04)
#define RB_UIF_TRANSFER   UINT8_C(0x02)
#define RB_UIF_BUS_RST    UINT8_C(0x01)

#define RB_UMS_SUSPEND    UINT8_C(0x04)

#define RB_UIS_SETUP_ACT  UINT8_C(0x80)
#define RB_UIS_TOG_OK     UINT8_C(0x40)
#define MASK_UIS_TOKEN    UINT8_C(0x30)
#define UIS_TOKEN_OUT     UINT8_C(0x00)
#define UIS_TOKEN_IN      UINT8_C(0x20)
#define MASK_UIS_ENDP     UINT8_C(0x0F)

#define RB_UEP1_RX_EN     UINT8_C(0x80)
#define RB_UEP1_TX_EN     UINT8_C(0x40)
#define RB_UEP4_RX_EN     UINT8_C(0x08)
#define RB_UEP4_TX_EN     UINT8_C(0x04)
#define RB_UEP3_RX_EN     UINT8_C(0x80)
#define RB_UEP3_TX_EN     UINT8_C(0x40)
#define RB_UEP2_RX_EN     UINT8_C(0x08)
#define RB_UEP2_TX_EN     UINT8_C(0x04)
#define RB_UEP7_RX_EN     UINT8_C(0x20)
#define RB_UEP7_TX_EN     UINT8_C(0x10)
#define RB_UEP6_RX_EN     UINT8_C(0x08)
#define RB_UEP6_TX_EN     UINT8_C(0x04)
#define RB_UEP5_RX_EN     UINT8_C(0x02)
#define RB_UEP5_TX_EN     UINT8_C(0x01)

#define RB_UEP_R_TOG      UINT8_C(0x80)
#define RB_UEP_T_TOG      UINT8_C(0x40)
#define RB_UEP_AUTO_TOG   UINT8_C(0x10)
#define MASK_UEP_R_RES    UINT8_C(0x0C)
#define UEP_R_RES_ACK     UINT8_C(0x00)
#define UEP_R_RES_NAK     UINT8_C(0x08)
#define UEP_R_RES_STALL   UINT8_C(0x0C)
#define MASK_UEP_T_RES    UINT8_C(0x03)
#define UEP_T_RES_ACK     UINT8_C(0x00)
#define UEP_T_RES_NAK     UINT8_C(0x02)
#define UEP_T_RES_STALL   UINT8_C(0x03)

#define MASK_USB_ADDR     UINT8_C(0x7F)
#define RB_PIN_USB_EN     UINT16_C(0x20)
#define RB_UDP_PU_EN      UINT16_C(0x40)
#define BIT_SLP_CLK_USB   UINT16_C(0x1000)

extern volatile uint8_t fake_usb_ctrl;
extern volatile uint8_t fake_udev_ctrl;
extern volatile uint8_t fake_usb_int_en;
extern volatile uint8_t fake_usb_dev_ad;
extern volatile uint8_t fake_usb_mis_st;
extern volatile uint8_t fake_usb_int_fg;
extern volatile uint8_t fake_usb_int_st;
extern volatile uint8_t fake_usb_rx_len;
extern volatile uint8_t fake_uep4_1_mod;
extern volatile uint8_t fake_uep2_3_mod;
extern volatile uint8_t fake_uep567_mod;
extern volatile uint32_t fake_uep_dma[8];
extern volatile uint8_t fake_uep_tx_len[8];
extern volatile uint8_t fake_uep_ctrl[8];
extern volatile uint16_t fake_pin_config;
extern volatile uint8_t fake_irq_enabled;
extern volatile uint16_t fake_power_clock_mask;
extern volatile uint32_t fake_delay_ms;
extern volatile uint8_t fake_time_initialized;

#define R8_USB_CTRL       fake_usb_ctrl
#define R8_UDEV_CTRL      fake_udev_ctrl
#define R8_USB_INT_EN     fake_usb_int_en
#define R8_USB_DEV_AD     fake_usb_dev_ad
#define R8_USB_MIS_ST     fake_usb_mis_st
#define R8_USB_INT_FG     fake_usb_int_fg
#define R8_USB_INT_ST     fake_usb_int_st
#define R8_USB_RX_LEN     fake_usb_rx_len
#define R8_UEP4_1_MOD     fake_uep4_1_mod
#define R8_UEP2_3_MOD     fake_uep2_3_mod
#define R8_UEP567_MOD     fake_uep567_mod

#define R32_UEP0_DMA      fake_uep_dma[0]
#define R32_UEP1_DMA      fake_uep_dma[1]
#define R32_UEP2_DMA      fake_uep_dma[2]
#define R32_UEP3_DMA      fake_uep_dma[3]
#define R32_UEP5_DMA      fake_uep_dma[5]
#define R32_UEP6_DMA      fake_uep_dma[6]
#define R32_UEP7_DMA      fake_uep_dma[7]

#define R8_UEP0_T_LEN     fake_uep_tx_len[0]
#define R8_UEP1_T_LEN     fake_uep_tx_len[1]
#define R8_UEP2_T_LEN     fake_uep_tx_len[2]
#define R8_UEP3_T_LEN     fake_uep_tx_len[3]
#define R8_UEP4_T_LEN     fake_uep_tx_len[4]
#define R8_UEP5_T_LEN     fake_uep_tx_len[5]
#define R8_UEP6_T_LEN     fake_uep_tx_len[6]
#define R8_UEP7_T_LEN     fake_uep_tx_len[7]

#define R8_UEP0_CTRL      fake_uep_ctrl[0]
#define R8_UEP1_CTRL      fake_uep_ctrl[1]
#define R8_UEP2_CTRL      fake_uep_ctrl[2]
#define R8_UEP3_CTRL      fake_uep_ctrl[3]
#define R8_UEP4_CTRL      fake_uep_ctrl[4]
#define R8_UEP5_CTRL      fake_uep_ctrl[5]
#define R8_UEP6_CTRL      fake_uep_ctrl[6]
#define R8_UEP7_CTRL      fake_uep_ctrl[7]

#define R16_PIN_CONFIG    fake_pin_config

void PFIC_ClearPendingIRQ(IRQn_Type irq);
void PFIC_EnableIRQ(IRQn_Type irq);
void PFIC_DisableIRQ(IRQn_Type irq);
void PWR_PeriphClkCfg(FunctionalState state, uint16_t peripheral);

#endif
