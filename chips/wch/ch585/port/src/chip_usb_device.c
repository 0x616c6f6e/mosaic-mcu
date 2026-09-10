#include <chip_usb_device.h>

#include <string.h>

#include <chip_system.h>
#include <chip_time.h>

#include "CH58x_common.h"

enum {
    USB_DIRECTION_OUT = 0,
    USB_DIRECTION_IN = 1,
};

typedef struct {
    bool open;
    chip_usb_endpoint_type_t type;
    uint8_t max_packet_size;
} usb_endpoint_config_t;

typedef struct {
    uint8_t *buffer;
    size_t total;
    size_t transferred;
    size_t queued;
    bool active;
} usb_transfer_t;

typedef struct {
    chip_usb_event_callback_t callback;
    void *callback_context;
    usb_endpoint_config_t endpoint[CHIP_USB_DEVICE_ENDPOINT_COUNT][2];
    usb_transfer_t transfer[CHIP_USB_DEVICE_ENDPOINT_COUNT][2];
    uint8_t endpoint0_max_packet_size;
    uint8_t pending_address;
    bool address_pending;
    bool initialized;
    bool connected;
    bool suspended;
} usb_device_state_t;

static usb_device_state_t usb_device;

static uint8_t usb_ep0_4_buffer[192] __attribute__((aligned(4)));
static uint8_t usb_ep1_buffer[128] __attribute__((aligned(4)));
static uint8_t usb_ep2_buffer[128] __attribute__((aligned(4)));
static uint8_t usb_ep3_buffer[128] __attribute__((aligned(4)));
static uint8_t usb_ep5_buffer[128] __attribute__((aligned(4)));
static uint8_t usb_ep6_buffer[128] __attribute__((aligned(4)));
static uint8_t usb_ep7_buffer[128] __attribute__((aligned(4)));

static bool usb_endpoint0_size_valid(uint8_t size)
{
    return (size == 8U) || (size == 16U) || (size == 32U) || (size == 64U);
}

static bool usb_endpoint_size_valid(chip_usb_endpoint_type_t type, uint16_t size)
{
    if ((size == 0U) || (size > CHIP_USB_DEVICE_MAX_PACKET_SIZE)) {
        return false;
    }
    if (type == CHIP_USB_ENDPOINT_BULK) {
        return (size == 8U) || (size == 16U) || (size == 32U) || (size == 64U);
    }
    return true;
}

static bool usb_endpoint_address_valid(uint8_t address)
{
    return ((address & UINT8_C(0x70)) == 0U) &&
           (CHIP_USB_ENDPOINT_NUMBER(address) < CHIP_USB_DEVICE_ENDPOINT_COUNT);
}

static uint8_t usb_direction(uint8_t endpoint_address)
{
    return CHIP_USB_ENDPOINT_IS_IN(endpoint_address) ? USB_DIRECTION_IN : USB_DIRECTION_OUT;
}

static volatile uint8_t *usb_endpoint_control(uint8_t endpoint)
{
    switch (endpoint) {
    case 0:
        return &R8_UEP0_CTRL;
    case 1:
        return &R8_UEP1_CTRL;
    case 2:
        return &R8_UEP2_CTRL;
    case 3:
        return &R8_UEP3_CTRL;
    case 4:
        return &R8_UEP4_CTRL;
    case 5:
        return &R8_UEP5_CTRL;
    case 6:
        return &R8_UEP6_CTRL;
    case 7:
        return &R8_UEP7_CTRL;
    default:
        return NULL;
    }
}

static volatile uint8_t *usb_endpoint_tx_length(uint8_t endpoint)
{
    switch (endpoint) {
    case 0:
        return &R8_UEP0_T_LEN;
    case 1:
        return &R8_UEP1_T_LEN;
    case 2:
        return &R8_UEP2_T_LEN;
    case 3:
        return &R8_UEP3_T_LEN;
    case 4:
        return &R8_UEP4_T_LEN;
    case 5:
        return &R8_UEP5_T_LEN;
    case 6:
        return &R8_UEP6_T_LEN;
    case 7:
        return &R8_UEP7_T_LEN;
    default:
        return NULL;
    }
}

static uint8_t *usb_endpoint_buffer(uint8_t endpoint, uint8_t direction)
{
    uint8_t *base;

    switch (endpoint) {
    case 0:
        return usb_ep0_4_buffer;
    case 1:
        base = usb_ep1_buffer;
        break;
    case 2:
        base = usb_ep2_buffer;
        break;
    case 3:
        base = usb_ep3_buffer;
        break;
    case 4:
        return &usb_ep0_4_buffer[(direction == USB_DIRECTION_IN) ? 128U : 64U];
    case 5:
        base = usb_ep5_buffer;
        break;
    case 6:
        base = usb_ep6_buffer;
        break;
    case 7:
        base = usb_ep7_buffer;
        break;
    default:
        return NULL;
    }

    return &base[(direction == USB_DIRECTION_IN) ? 64U : 0U];
}

static void usb_endpoint_update_mode(uint8_t endpoint)
{
    volatile uint8_t *mode;
    uint8_t mask;
    /* Enabling both directions keeps OUT at DMA and IN at DMA + 64. */
    bool enabled = usb_device.endpoint[endpoint][USB_DIRECTION_OUT].open ||
                   usb_device.endpoint[endpoint][USB_DIRECTION_IN].open;

    switch (endpoint) {
    case 1:
        mode = &R8_UEP4_1_MOD;
        mask = (uint8_t)(RB_UEP1_RX_EN | RB_UEP1_TX_EN);
        break;
    case 2:
        mode = &R8_UEP2_3_MOD;
        mask = (uint8_t)(RB_UEP2_RX_EN | RB_UEP2_TX_EN);
        break;
    case 3:
        mode = &R8_UEP2_3_MOD;
        mask = (uint8_t)(RB_UEP3_RX_EN | RB_UEP3_TX_EN);
        break;
    case 4:
        mode = &R8_UEP4_1_MOD;
        mask = (uint8_t)(RB_UEP4_RX_EN | RB_UEP4_TX_EN);
        break;
    case 5:
        mode = &R8_UEP567_MOD;
        mask = (uint8_t)(RB_UEP5_RX_EN | RB_UEP5_TX_EN);
        break;
    case 6:
        mode = &R8_UEP567_MOD;
        mask = (uint8_t)(RB_UEP6_RX_EN | RB_UEP6_TX_EN);
        break;
    case 7:
        mode = &R8_UEP567_MOD;
        mask = (uint8_t)(RB_UEP7_RX_EN | RB_UEP7_TX_EN);
        break;
    default:
        return;
    }

    if (enabled) {
        *mode |= mask;
    } else {
        *mode &= (uint8_t)~mask;
    }
}

static void usb_set_response(uint8_t endpoint, uint8_t direction, uint8_t response)
{
    volatile uint8_t *control = usb_endpoint_control(endpoint);
    uint8_t mask = (direction == USB_DIRECTION_IN) ? MASK_UEP_T_RES : MASK_UEP_R_RES;

    *control = (uint8_t)((*control & (uint8_t)~mask) | response);
}

static void usb_toggle_manual_endpoint(uint8_t endpoint, uint8_t direction)
{
    volatile uint8_t *control = usb_endpoint_control(endpoint);

    /* CH58x AUTO_TOG loses synchronization during multi-packet transfers. */
    *control ^= (direction == USB_DIRECTION_IN) ? RB_UEP_T_TOG : RB_UEP_R_TOG;
}

static void usb_emit(const chip_usb_event_t *event)
{
    if (usb_device.callback != NULL) {
        usb_device.callback(event, usb_device.callback_context);
    }
}

static void usb_emit_transfer(uint8_t endpoint, uint8_t direction, size_t transferred)
{
    chip_usb_event_t event = {
        .type = CHIP_USB_EVENT_TRANSFER_COMPLETE,
        .data.transfer = {
            .endpoint_address = CHIP_USB_ENDPOINT_ADDRESS(
                endpoint, (direction == USB_DIRECTION_IN) ? CHIP_USB_ENDPOINT_DIRECTION_IN : 0U),
            .transferred = transferred,
        },
    };
    usb_emit(&event);
}

static void usb_emit_error(chip_usb_error_t code, uint8_t endpoint, uint8_t direction)
{
    chip_usb_event_t event = {
        .type = CHIP_USB_EVENT_ERROR,
        .data.error = {
            .code = code,
            .endpoint_address = CHIP_USB_ENDPOINT_ADDRESS(
                endpoint, (direction == USB_DIRECTION_IN) ? CHIP_USB_ENDPOINT_DIRECTION_IN : 0U),
        },
    };
    usb_emit(&event);
}

static void usb_configure_dma(void)
{
    R32_UEP0_DMA = (uint32_t)(uintptr_t)usb_ep0_4_buffer;
    R32_UEP1_DMA = (uint32_t)(uintptr_t)usb_ep1_buffer;
    R32_UEP2_DMA = (uint32_t)(uintptr_t)usb_ep2_buffer;
    R32_UEP3_DMA = (uint32_t)(uintptr_t)usb_ep3_buffer;
    R32_UEP5_DMA = (uint32_t)(uintptr_t)usb_ep5_buffer;
    R32_UEP6_DMA = (uint32_t)(uintptr_t)usb_ep6_buffer;
    R32_UEP7_DMA = (uint32_t)(uintptr_t)usb_ep7_buffer;
}

static void usb_reset_endpoints(void)
{
    uint8_t endpoint;

    memset(usb_device.endpoint, 0, sizeof(usb_device.endpoint));
    memset(usb_device.transfer, 0, sizeof(usb_device.transfer));
    usb_device.endpoint[0][USB_DIRECTION_OUT].open = true;
    usb_device.endpoint[0][USB_DIRECTION_OUT].type = CHIP_USB_ENDPOINT_CONTROL;
    usb_device.endpoint[0][USB_DIRECTION_OUT].max_packet_size =
        usb_device.endpoint0_max_packet_size;
    usb_device.endpoint[0][USB_DIRECTION_IN] =
        usb_device.endpoint[0][USB_DIRECTION_OUT];
    usb_device.address_pending = false;
    usb_device.pending_address = 0;

    R8_UEP4_1_MOD = 0;
    R8_UEP2_3_MOD = 0;
    R8_UEP567_MOD = 0;
    usb_configure_dma();

    for (endpoint = 0; endpoint < CHIP_USB_DEVICE_ENDPOINT_COUNT; ++endpoint) {
        volatile uint8_t *control = usb_endpoint_control(endpoint);
        volatile uint8_t *tx_length = usb_endpoint_tx_length(endpoint);
        *tx_length = 0;
        *control = (uint8_t)(UEP_R_RES_NAK | UEP_T_RES_NAK);
    }
    usb_set_response(0, USB_DIRECTION_OUT, UEP_R_RES_ACK);
}

static void usb_load_next_in_packet(uint8_t endpoint)
{
    usb_transfer_t *transfer = &usb_device.transfer[endpoint][USB_DIRECTION_IN];
    usb_endpoint_config_t *config = &usb_device.endpoint[endpoint][USB_DIRECTION_IN];
    size_t remaining = transfer->total - transfer->transferred;
    size_t packet_size = (remaining > config->max_packet_size) ?
                         config->max_packet_size : remaining;

    if (packet_size > 0U) {
        memcpy(usb_endpoint_buffer(endpoint, USB_DIRECTION_IN),
               &transfer->buffer[transfer->transferred], packet_size);
    }
    transfer->queued = packet_size;
    *usb_endpoint_tx_length(endpoint) = (uint8_t)packet_size;
    usb_set_response(endpoint, USB_DIRECTION_IN, UEP_T_RES_ACK);
}

static void usb_process_in(uint8_t endpoint)
{
    usb_transfer_t *transfer;

    if (endpoint >= CHIP_USB_DEVICE_ENDPOINT_COUNT) {
        return;
    }
    transfer = &usb_device.transfer[endpoint][USB_DIRECTION_IN];
    if (!transfer->active) {
        usb_set_response(endpoint, USB_DIRECTION_IN, UEP_T_RES_NAK);
        return;
    }

    usb_toggle_manual_endpoint(endpoint, USB_DIRECTION_IN);
    transfer->transferred += transfer->queued;
    transfer->queued = 0;
    if (transfer->transferred < transfer->total) {
        usb_load_next_in_packet(endpoint);
        return;
    }

    transfer->active = false;
    usb_set_response(endpoint, USB_DIRECTION_IN, UEP_T_RES_NAK);
    if ((endpoint == 0U) && usb_device.address_pending) {
        R8_USB_DEV_AD = usb_device.pending_address;
        usb_device.address_pending = false;
    }
    usb_emit_transfer(endpoint, USB_DIRECTION_IN, transfer->transferred);
}

static void usb_process_out(uint8_t endpoint, uint8_t received)
{
    usb_transfer_t *transfer;
    usb_endpoint_config_t *config;
    size_t remaining;

    if (endpoint >= CHIP_USB_DEVICE_ENDPOINT_COUNT) {
        return;
    }
    transfer = &usb_device.transfer[endpoint][USB_DIRECTION_OUT];
    config = &usb_device.endpoint[endpoint][USB_DIRECTION_OUT];
    if (!transfer->active) {
        usb_set_response(endpoint, USB_DIRECTION_OUT, UEP_R_RES_NAK);
        return;
    }

    remaining = transfer->total - transfer->transferred;
    if (((size_t)received > remaining) || (received > config->max_packet_size)) {
        transfer->active = false;
        usb_set_response(endpoint, USB_DIRECTION_OUT, UEP_R_RES_NAK);
        usb_emit_error(CHIP_USB_ERROR_INVALID_TRANSACTION, endpoint, USB_DIRECTION_OUT);
        return;
    }

    if (received > 0U) {
        memcpy(&transfer->buffer[transfer->transferred],
               usb_endpoint_buffer(endpoint, USB_DIRECTION_OUT), received);
    }
    usb_toggle_manual_endpoint(endpoint, USB_DIRECTION_OUT);
    transfer->transferred += received;

    if ((transfer->transferred == transfer->total) ||
        (received < config->max_packet_size)) {
        transfer->active = false;
        usb_set_response(endpoint, USB_DIRECTION_OUT, UEP_R_RES_NAK);
        usb_emit_transfer(endpoint, USB_DIRECTION_OUT, transfer->transferred);
    } else {
        usb_set_response(endpoint, USB_DIRECTION_OUT, UEP_R_RES_ACK);
    }
}

static void usb_process_setup(void)
{
    chip_usb_event_t event = {
        .type = CHIP_USB_EVENT_SETUP_RECEIVED,
    };

    usb_device.transfer[0][USB_DIRECTION_OUT].active = false;
    usb_device.transfer[0][USB_DIRECTION_IN].active = false;
    usb_device.address_pending = false;
    R8_UEP0_CTRL = (uint8_t)(RB_UEP_R_TOG | RB_UEP_T_TOG |
                             UEP_R_RES_NAK | UEP_T_RES_NAK);
    memcpy(event.data.setup.bytes, usb_ep0_4_buffer, sizeof(event.data.setup.bytes));
    usb_emit(&event);
}

static void usb_process_bus_reset(void)
{
    chip_usb_event_t event = {
        .type = CHIP_USB_EVENT_BUS_RESET,
        .data.bus_reset = {
            .speed_mbps = 12,
        },
    };

    R8_USB_DEV_AD = 0;
    usb_device.suspended = false;
    usb_reset_endpoints();
    usb_emit(&event);
}

static void usb_interrupt_handler(void)
{
    uint8_t flags = R8_USB_INT_FG;

    if ((flags & RB_UIF_BUS_RST) != 0U) {
        usb_process_bus_reset();
        R8_USB_INT_FG = RB_UIF_BUS_RST;
        flags &= (uint8_t)~RB_UIF_TRANSFER;
    }

    if ((flags & RB_UIF_TRANSFER) != 0U) {
        uint8_t status = R8_USB_INT_ST;
        uint8_t endpoint = status & MASK_UIS_ENDP;

        if ((status & RB_UIS_SETUP_ACT) != 0U) {
            usb_process_setup();
        } else if ((status & MASK_UIS_TOKEN) == UIS_TOKEN_IN) {
            usb_process_in(endpoint);
        } else if ((status & MASK_UIS_TOKEN) == UIS_TOKEN_OUT) {
            if ((status & RB_UIS_TOG_OK) != 0U) {
                usb_process_out(endpoint, R8_USB_RX_LEN);
            }
        }
        R8_USB_INT_FG = RB_UIF_TRANSFER;
    }

    if ((flags & RB_UIF_SUSPEND) != 0U) {
        chip_usb_event_t event;
        usb_device.suspended = (R8_USB_MIS_ST & RB_UMS_SUSPEND) != 0U;
        event.type = usb_device.suspended ? CHIP_USB_EVENT_SUSPEND : CHIP_USB_EVENT_RESUME;
        usb_emit(&event);
        R8_USB_INT_FG = RB_UIF_SUSPEND;
    }

    if ((flags & RB_UIF_FIFO_OV) != 0U) {
        usb_emit_error(CHIP_USB_ERROR_FIFO_OVERFLOW, 0, USB_DIRECTION_OUT);
        R8_USB_INT_FG = RB_UIF_FIFO_OV;
    }
}

chip_status_t chip_usb_device_init(const chip_usb_device_config_t *config)
{
    uint32_t state;

    if ((config == NULL) || (config->callback == NULL) ||
        !usb_endpoint0_size_valid(config->endpoint0_max_packet_size)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (usb_device.initialized) {
        return CHIP_ERROR_BUSY;
    }

    state = chip_system_critical_enter();
    memset(&usb_device, 0, sizeof(usb_device));
    usb_device.callback = config->callback;
    usb_device.callback_context = config->callback_context;
    usb_device.endpoint0_max_packet_size = config->endpoint0_max_packet_size;

    PWR_PeriphClkCfg(ENABLE, BIT_SLP_CLK_USB);
    R8_USB_CTRL = 0;
    R8_UDEV_CTRL = RB_UD_PD_DIS;
    R8_USB_DEV_AD = 0;
    usb_reset_endpoints();
    R16_PIN_CONFIG = (uint16_t)((R16_PIN_CONFIG | RB_PIN_USB_EN) &
                                (uint16_t)~RB_UDP_PU_EN);
    R8_USB_INT_FG = UINT8_MAX;
    R8_USB_INT_EN = (uint8_t)(RB_UIE_BUS_RST | RB_UIE_TRANSFER |
                              RB_UIE_SUSPEND | RB_UIE_FIFO_OV);
    R8_USB_CTRL = (uint8_t)(RB_UC_SYS_CTRL0 | RB_UC_INT_BUSY | RB_UC_DMA_EN);
    usb_device.initialized = true;
    chip_system_critical_exit(state);

    PFIC_ClearPendingIRQ(USB_IRQn);
    PFIC_EnableIRQ(USB_IRQn);
    return CHIP_OK;
}

chip_status_t chip_usb_device_deinit(void)
{
    uint32_t state;

    if (!usb_device.initialized) {
        return CHIP_ERROR_NOT_READY;
    }

    PFIC_DisableIRQ(USB_IRQn);
    state = chip_system_critical_enter();
    R8_USB_INT_EN = 0;
    R8_UDEV_CTRL = 0;
    R8_USB_CTRL = 0;
    R16_PIN_CONFIG &= (uint16_t)~(RB_PIN_USB_EN | RB_UDP_PU_EN);
    PWR_PeriphClkCfg(DISABLE, BIT_SLP_CLK_USB);
    memset(&usb_device, 0, sizeof(usb_device));
    chip_system_critical_exit(state);
    return CHIP_OK;
}

chip_status_t chip_usb_device_connect(void)
{
    if (!usb_device.initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    R8_USB_CTRL = (uint8_t)((R8_USB_CTRL & (uint8_t)~MASK_UC_SYS_CTRL) |
                            RB_UC_DEV_PU_EN);
    R8_UDEV_CTRL = (uint8_t)(RB_UD_PD_DIS | RB_UD_PORT_EN);
    usb_device.connected = true;
    return CHIP_OK;
}

chip_status_t chip_usb_device_disconnect(void)
{
    if (!usb_device.initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    R8_UDEV_CTRL = RB_UD_PD_DIS;
    R8_USB_CTRL = (uint8_t)((R8_USB_CTRL & (uint8_t)~MASK_UC_SYS_CTRL) |
                            RB_UC_SYS_CTRL0);
    usb_device.connected = false;
    return CHIP_OK;
}

chip_status_t chip_usb_device_set_interrupt_enabled(bool enabled)
{
    if (!usb_device.initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    if (enabled) {
        PFIC_EnableIRQ(USB_IRQn);
    } else {
        PFIC_DisableIRQ(USB_IRQn);
    }
    return CHIP_OK;
}

bool chip_usb_device_is_suspended(void)
{
    return usb_device.initialized && usb_device.suspended;
}

chip_status_t chip_usb_device_set_address(uint8_t address)
{
    if (address > MASK_USB_ADDR) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!usb_device.initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    usb_device.pending_address = address;
    usb_device.address_pending = true;
    return CHIP_OK;
}

chip_status_t chip_usb_device_remote_wakeup(void)
{
    if (!usb_device.initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    if (!usb_device.connected || !usb_device.suspended) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!chip_time_is_initialized()) {
        return CHIP_ERROR_NOT_READY;
    }

    R8_UDEV_CTRL |= RB_UD_LOW_SPEED;
    chip_delay_ms(2);
    R8_UDEV_CTRL &= (uint8_t)~RB_UD_LOW_SPEED;
    return CHIP_OK;
}

chip_status_t chip_usb_device_endpoint_open(uint8_t endpoint_address,
                                            chip_usb_endpoint_type_t type,
                                            uint16_t max_packet_size)
{
    uint8_t endpoint;
    uint8_t direction;
    uint32_t state;
    usb_endpoint_config_t *endpoint_config;

    if (!usb_endpoint_address_valid(endpoint_address) ||
        (type > CHIP_USB_ENDPOINT_INTERRUPT) ||
        !usb_endpoint_size_valid(type, max_packet_size)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!usb_device.initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    endpoint = CHIP_USB_ENDPOINT_NUMBER(endpoint_address);
    if (endpoint == 0U) {
        return CHIP_ERROR_BUSY;
    }
    if ((type == CHIP_USB_ENDPOINT_CONTROL) || (type == CHIP_USB_ENDPOINT_ISOCHRONOUS)) {
        return CHIP_ERROR_UNSUPPORTED;
    }

    direction = usb_direction(endpoint_address);
    endpoint_config = &usb_device.endpoint[endpoint][direction];
    state = chip_system_critical_enter();
    if (endpoint_config->open) {
        chip_system_critical_exit(state);
        return CHIP_ERROR_BUSY;
    }
    endpoint_config->open = true;
    endpoint_config->type = type;
    endpoint_config->max_packet_size = (uint8_t)max_packet_size;
    usb_device.transfer[endpoint][direction].active = false;
    usb_endpoint_update_mode(endpoint);
    *usb_endpoint_control(endpoint) &= (uint8_t)~((direction == USB_DIRECTION_IN) ?
                                                 RB_UEP_T_TOG : RB_UEP_R_TOG);
    usb_set_response(endpoint, direction,
                     (direction == USB_DIRECTION_IN) ? UEP_T_RES_NAK : UEP_R_RES_NAK);
    chip_system_critical_exit(state);
    return CHIP_OK;
}

chip_status_t chip_usb_device_endpoint_close(uint8_t endpoint_address)
{
    uint8_t endpoint;
    uint8_t direction;
    uint32_t state;

    if (!usb_endpoint_address_valid(endpoint_address)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!usb_device.initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    endpoint = CHIP_USB_ENDPOINT_NUMBER(endpoint_address);
    if (endpoint == 0U) {
        return CHIP_ERROR_BUSY;
    }
    direction = usb_direction(endpoint_address);

    state = chip_system_critical_enter();
    memset(&usb_device.transfer[endpoint][direction], 0, sizeof(usb_transfer_t));
    memset(&usb_device.endpoint[endpoint][direction], 0, sizeof(usb_endpoint_config_t));
    if (direction == USB_DIRECTION_IN) {
        *usb_endpoint_tx_length(endpoint) = 0;
    }
    *usb_endpoint_control(endpoint) &= (uint8_t)~((direction == USB_DIRECTION_IN) ?
                                                 RB_UEP_T_TOG : RB_UEP_R_TOG);
    usb_set_response(endpoint, direction,
                     (direction == USB_DIRECTION_IN) ? UEP_T_RES_NAK : UEP_R_RES_NAK);
    usb_endpoint_update_mode(endpoint);
    chip_system_critical_exit(state);
    return CHIP_OK;
}

static chip_status_t usb_endpoint_set_stall(uint8_t endpoint_address, bool stalled)
{
    uint8_t endpoint;
    uint8_t direction;
    volatile uint8_t *control;
    uint8_t response;
    uint32_t state;

    if (!usb_endpoint_address_valid(endpoint_address)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!usb_device.initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    endpoint = CHIP_USB_ENDPOINT_NUMBER(endpoint_address);
    direction = usb_direction(endpoint_address);
    if (!usb_device.endpoint[endpoint][direction].open) {
        return CHIP_ERROR_NOT_READY;
    }

    state = chip_system_critical_enter();
    control = usb_endpoint_control(endpoint);
    if (stalled) {
        memset(&usb_device.transfer[endpoint][direction], 0, sizeof(usb_transfer_t));
        if (direction == USB_DIRECTION_IN) {
            *usb_endpoint_tx_length(endpoint) = 0;
        }
    }
    if (!stalled) {
        *control &= (uint8_t)~((direction == USB_DIRECTION_IN) ?
                              RB_UEP_T_TOG : RB_UEP_R_TOG);
    }
    response = (direction == USB_DIRECTION_IN) ?
               (stalled ? UEP_T_RES_STALL : UEP_T_RES_NAK) :
               (stalled ? UEP_R_RES_STALL : UEP_R_RES_NAK);
    usb_set_response(endpoint, direction, response);
    chip_system_critical_exit(state);
    return CHIP_OK;
}

chip_status_t chip_usb_device_endpoint_stall(uint8_t endpoint_address)
{
    return usb_endpoint_set_stall(endpoint_address, true);
}

chip_status_t chip_usb_device_endpoint_clear_stall(uint8_t endpoint_address)
{
    return usb_endpoint_set_stall(endpoint_address, false);
}

chip_status_t chip_usb_device_endpoint_cancel(uint8_t endpoint_address)
{
    uint8_t endpoint;
    uint8_t direction;
    uint32_t state;

    if (!usb_endpoint_address_valid(endpoint_address)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!usb_device.initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    endpoint = CHIP_USB_ENDPOINT_NUMBER(endpoint_address);
    direction = usb_direction(endpoint_address);
    if (!usb_device.endpoint[endpoint][direction].open) {
        return CHIP_ERROR_NOT_READY;
    }

    state = chip_system_critical_enter();
    memset(&usb_device.transfer[endpoint][direction], 0, sizeof(usb_transfer_t));
    if (direction == USB_DIRECTION_IN) {
        *usb_endpoint_tx_length(endpoint) = 0;
    }
    usb_set_response(endpoint, direction,
                     (direction == USB_DIRECTION_IN) ? UEP_T_RES_NAK : UEP_R_RES_NAK);
    chip_system_critical_exit(state);
    return CHIP_OK;
}

chip_status_t chip_usb_device_submit_in(uint8_t endpoint_number,
                                        const void *data,
                                        size_t size)
{
    usb_transfer_t *transfer;
    uint32_t state;

    if ((endpoint_number >= CHIP_USB_DEVICE_ENDPOINT_COUNT) ||
        ((data == NULL) && (size != 0U))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!usb_device.initialized ||
        !usb_device.endpoint[endpoint_number][USB_DIRECTION_IN].open) {
        return CHIP_ERROR_NOT_READY;
    }

    state = chip_system_critical_enter();
    transfer = &usb_device.transfer[endpoint_number][USB_DIRECTION_IN];
    if (transfer->active) {
        chip_system_critical_exit(state);
        return CHIP_ERROR_BUSY;
    }
    transfer->buffer = (uint8_t *)(uintptr_t)data;
    transfer->total = size;
    transfer->transferred = 0;
    transfer->queued = 0;
    transfer->active = true;
    usb_load_next_in_packet(endpoint_number);
    chip_system_critical_exit(state);
    return CHIP_OK;
}

chip_status_t chip_usb_device_submit_out(uint8_t endpoint_number,
                                         void *data,
                                         size_t size)
{
    usb_transfer_t *transfer;
    uint32_t state;

    if ((endpoint_number >= CHIP_USB_DEVICE_ENDPOINT_COUNT) ||
        ((data == NULL) && (size != 0U))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!usb_device.initialized ||
        !usb_device.endpoint[endpoint_number][USB_DIRECTION_OUT].open) {
        return CHIP_ERROR_NOT_READY;
    }

    state = chip_system_critical_enter();
    transfer = &usb_device.transfer[endpoint_number][USB_DIRECTION_OUT];
    if (transfer->active) {
        chip_system_critical_exit(state);
        return CHIP_ERROR_BUSY;
    }
    transfer->buffer = (uint8_t *)data;
    transfer->total = size;
    transfer->transferred = 0;
    transfer->queued = 0;
    transfer->active = true;
    usb_set_response(endpoint_number, USB_DIRECTION_OUT, UEP_R_RES_ACK);
    chip_system_critical_exit(state);
    return CHIP_OK;
}

__HIGH_CODE
__INTERRUPT
void USB_IRQHandler(void)
{
    usb_interrupt_handler();
}
