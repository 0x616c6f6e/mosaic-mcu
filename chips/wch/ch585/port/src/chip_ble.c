#include <chip_ble.h>

#include <limits.h>
#include <string.h>

#include <chip_system.h>

#include "CH58xBLE_LIB.h"
#include "CH58x_common.h"

#define CH585_BLE_HEAP_SIZE             (6U * 1024U)
#define CH585_BLE_BUFFER_MAX_LENGTH     27U
#define CH585_BLE_BUFFER_COUNT          5U
#define CH585_BLE_TX_PACKETS_PER_EVENT  1U
#define CH585_BLE_START_EVENT           UINT16_C(0x0001)
#define CH585_BLE_CALIBRATION_EVENT     UINT16_C(0x0002)
#define CH585_BLE_CALIBRATION_DELAY_MS  UINT32_C(500)
#define CH585_BLE_CALIBRATION_PERIOD_MS UINT32_C(120000)
#define CH585_BLE_DEFAULT_ADV_INTERVAL  UINT16_C(160)
#define CH585_BLE_DEFAULT_CONN_MIN      UINT16_C(8)
#define CH585_BLE_DEFAULT_CONN_MAX      UINT16_C(80)

static uint32_t ble_heap[CH585_BLE_HEAP_SIZE / sizeof(uint32_t)]
    __attribute__((aligned(4)));
static uint8_t ble_default_advertising_data[CHIP_BLE_LEGACY_ADV_MAX_SIZE];
static chip_ble_peripheral_config_t ble_config;
static chip_ble_state_t ble_state;
static uint16_t ble_connection_handle = CHIP_BLE_CONN_HANDLE_INVALID;
static tmosTaskID ble_task_id = INVALID_TASK_ID;

uint32_t g_LLE_IRQLibHandlerLocation;

__attribute__((weak))
void ch585_ble_disconnected_hook(uint16_t connection_handle)
{
    (void)connection_handle;
}

static uint32_t ble_random_seed(void)
{
    uint32_t rtc_count;

    do {
        rtc_count = R32_RTC_CNT_32K;
    } while (rtc_count != R32_RTC_CNT_32K);
    return rtc_count ^ SYS_GetSysTickCnt();
}

static uint32_t ble_rtc_count(void)
{
    uint32_t first;

    do {
        first = R32_RTC_CNT_32K;
    } while (first != R32_RTC_CNT_32K);
    return first;
}

static void ble_rtc_set_pending(void)
{
    PFIC_SetPendingIRQ(RTC_IRQn);
}

static void ble_calibrate_lsi(void)
{
    Calibration_LSI(Level_64);
}

static uint16_t ble_temperature_sample(void)
{
    uint8_t sensor = R8_TEM_SENSOR;
    uint8_t channel = R8_ADC_CHANNEL;
    uint8_t config = R8_ADC_CFG;
    uint8_t touch_key_config = R8_TKEY_CFG;
    uint16_t sample;

    ADC_InterTSSampInit();
    R8_ADC_CONVERT |= RB_ADC_START;
    while ((R8_ADC_CONVERT & RB_ADC_START) != 0U) {
    }
    sample = R16_ADC_DATA;
    R8_TEM_SENSOR = sensor;
    R8_ADC_CHANNEL = channel;
    R8_ADC_CFG = config;
    R8_TKEY_CFG = touch_key_config;
    return sample;
}

static void ble_emit(chip_ble_event_type_t type, uint8_t reason,
                     uint8_t status)
{
    chip_ble_event_t event = {
        .type = type,
        .state = ble_state,
        .connection_handle = ble_connection_handle,
        .reason = reason,
        .status = status,
    };

    if (ble_config.event_callback != NULL) {
        ble_config.event_callback(&event, ble_config.event_context);
    }
}

static void ble_emit_disconnected(uint16_t connection_handle, uint8_t reason,
                                  uint8_t status)
{
    chip_ble_event_t event = {
        .type = CHIP_BLE_EVENT_DISCONNECTED,
        .state = ble_state,
        .connection_handle = connection_handle,
        .reason = reason,
        .status = status,
    };

    if (ble_config.event_callback != NULL) {
        ble_config.event_callback(&event, ble_config.event_context);
    }
}

static void ble_set_state(chip_ble_state_t state)
{
    if (ble_state != state) {
        ble_state = state;
        ble_emit(CHIP_BLE_EVENT_STATE_CHANGED, 0U, SUCCESS);
    }
}

static uint8_t ble_tx_power_code(int8_t dbm)
{
    switch (dbm) {
    case -20: return LL_TX_PWR_MINUS_20_DBM;
    case -15: return LL_TX_PWR_MINUS_15_DBM;
    case -10: return LL_TX_PWR_MINUS_10_DBM;
    case -8: return LL_TX_PWR_MINUS_8_DBM;
    case -5: return LL_TX_PWR_MINUS_5_DBM;
    case -3: return LL_TX_PWR_MINUS_3_DBM;
    case -1: return LL_TX_PWR_MINUS_1_DBM;
    case 0: return LL_TX_PWR_0_DBM;
    case 1: return LL_TX_PWR_1_DBM;
    case 2: return LL_TX_PWR_2_DBM;
    case 3: return LL_TX_PWR_3_DBM;
    case 4: return LL_TX_PWR_4_DBM;
    default: return UINT8_MAX;
    }
}

static chip_status_t ble_validate_config(const chip_ble_peripheral_config_t *config)
{
    if ((config == NULL) || (config->device_name == NULL) ||
        (strlen(config->device_name) > CHIP_BLE_DEVICE_NAME_MAX_SIZE) ||
        (config->advertising_data_size > CHIP_BLE_LEGACY_ADV_MAX_SIZE) ||
        (config->scan_response_data_size > CHIP_BLE_LEGACY_ADV_MAX_SIZE) ||
        ((config->advertising_data == NULL) &&
         (config->advertising_data_size != 0U)) ||
        ((config->scan_response_data == NULL) &&
         (config->scan_response_data_size != 0U)) ||
        (config->advertising_interval < UINT16_C(32)) ||
        (config->advertising_interval > UINT16_C(16384)) ||
        (config->min_connection_interval < UINT16_C(6)) ||
        (config->max_connection_interval > UINT16_C(3200)) ||
        (config->min_connection_interval > config->max_connection_interval) ||
        (ble_tx_power_code(config->tx_power_dbm) == UINT8_MAX)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    return CHIP_OK;
}

static void ble_role_state_changed(gapRole_States_t vendor_state,
                                   gapRoleEvent_t *vendor_event)
{
    uint8_t state = (uint8_t)(vendor_state & GAPROLE_STATE_ADV_MASK);
    uint8_t opcode = (vendor_event != NULL) ? vendor_event->gap.opcode : 0U;
    uint8_t status = (vendor_event != NULL) ? vendor_event->gap.hdr.status : SUCCESS;

    if ((opcode == GAP_LINK_ESTABLISHED_EVENT) && (status == SUCCESS)) {
        ble_connection_handle = vendor_event->linkCmpl.connectionHandle;
        ble_set_state(CHIP_BLE_STATE_CONNECTED);
        ble_emit(CHIP_BLE_EVENT_CONNECTED, 0U, status);
        return;
    }
    if (opcode == GAP_LINK_TERMINATED_EVENT) {
        uint16_t connection_handle = vendor_event->linkTerminate.connectionHandle;
        uint8_t reason = vendor_event->linkTerminate.reason;

        ble_set_state((state == GAPROLE_ADVERTISING) ?
                      CHIP_BLE_STATE_ADVERTISING : CHIP_BLE_STATE_READY);
        ble_emit_disconnected(connection_handle, reason, status);
        ch585_ble_disconnected_hook(connection_handle);
        ble_connection_handle = CHIP_BLE_CONN_HANDLE_INVALID;
        if (ble_config.advertise_on_start) {
            uint8_t enabled = TRUE;
            (void)GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED,
                                       sizeof(enabled), &enabled);
        }
        return;
    }

    switch (state) {
    case GAPROLE_STARTED:
    case GAPROLE_WAITING:
        ble_set_state(CHIP_BLE_STATE_READY);
        break;
    case GAPROLE_ADVERTISING:
        ble_set_state(CHIP_BLE_STATE_ADVERTISING);
        break;
    case GAPROLE_CONNECTED:
    case GAPROLE_CONNECTED_ADV:
        ble_set_state(CHIP_BLE_STATE_CONNECTED);
        break;
    case GAPROLE_ERROR:
        ble_set_state(CHIP_BLE_STATE_ERROR);
        ble_emit(CHIP_BLE_EVENT_ERROR, 0U, status);
        break;
    default:
        break;
    }
}

static gapRolesCBs_t ble_role_callbacks = {
    .pfnStateChange = ble_role_state_changed,
    .pfnRssiRead = NULL,
    .pfnParamUpdate = NULL,
};

static gapBondCBs_t ble_bond_callbacks = {
    .passcodeCB = NULL,
    .pairStateCB = NULL,
    .oobCB = NULL,
};

static tmosEvents ble_process_event(tmosTaskID task_id, tmosEvents events)
{
    if ((events & SYS_EVENT_MSG) != 0U) {
        uint8_t *message = tmos_msg_receive(task_id);

        if (message != NULL) {
            (void)tmos_msg_deallocate(message);
        }
        return (tmosEvents)(events ^ SYS_EVENT_MSG);
    }
    if ((events & CH585_BLE_START_EVENT) != 0U) {
        bStatus_t status = GAPRole_PeripheralStartDevice(
            task_id, &ble_bond_callbacks, &ble_role_callbacks);

        if (status != SUCCESS) {
            ble_set_state(CHIP_BLE_STATE_ERROR);
            ble_emit(CHIP_BLE_EVENT_ERROR, 0U, status);
        }
        return (tmosEvents)(events ^ CH585_BLE_START_EVENT);
    }
    if ((events & CH585_BLE_CALIBRATION_EVENT) != 0U) {
        BLE_RegInit();
        ble_calibrate_lsi();
        (void)tmos_start_task(task_id, CH585_BLE_CALIBRATION_EVENT,
                              MS1_TO_SYSTEM_TIME(CH585_BLE_CALIBRATION_PERIOD_MS));
        return (tmosEvents)(events ^ CH585_BLE_CALIBRATION_EVENT);
    }
    return 0U;
}

static chip_status_t ble_timer_init(void)
{
    bleClockConfig_t clock_config;

    sys_safe_access_enable();
    R8_CK32K_CONFIG &= (uint8_t)~(RB_CLK_OSC32K_XT | RB_CLK_XT32K_PON);
    sys_safe_access_disable();
    sys_safe_access_enable();
    R8_CK32K_CONFIG |= RB_CLK_INT32K_PON;
    sys_safe_access_disable();
    LSECFG_Current(LSE_RCur_100);
    ble_calibrate_lsi();
    RTC_InitTime(2020U, 1U, 1U, 0U, 0U, 0U);

    memset(&clock_config, 0, sizeof(clock_config));
    clock_config.ClockAccuracy = 1000U;
    clock_config.ClockFrequency = CAB_LSIFQ;
    clock_config.ClockMaxCount = RTC_MAX_COUNT;
    clock_config.getClockValue = ble_rtc_count;
    clock_config.SetPendingIRQ = ble_rtc_set_pending;

    return (TMOS_TimerInit(&clock_config) == SUCCESS) ? CHIP_OK : CHIP_ERROR_IO;
}

void chip_ble_peripheral_config_default(chip_ble_peripheral_config_t *config)
{
    if (config != NULL) {
        memset(config, 0, sizeof(*config));
        config->device_name = "CH585 BLE";
        config->advertising_interval = CH585_BLE_DEFAULT_ADV_INTERVAL;
        config->min_connection_interval = CH585_BLE_DEFAULT_CONN_MIN;
        config->max_connection_interval = CH585_BLE_DEFAULT_CONN_MAX;
        config->tx_power_dbm = 0;
        config->advertise_on_start = true;
    }
}

chip_status_t chip_ble_peripheral_init(const chip_ble_peripheral_config_t *config)
{
    bleConfig_t vendor_config;
    uint32_t mac_words[2] = {0U, 0U};
    uint8_t advertising_enabled;
    uint8_t pairing_mode = GAPBOND_PAIRING_MODE_NO_PAIRING;
    bStatus_t status;

    if (ble_state != CHIP_BLE_STATE_UNINITIALIZED) {
        return CHIP_ERROR_BUSY;
    }
    if (chip_system_clock_source() != CHIP_CLOCK_EXTERNAL) {
        return CHIP_ERROR_UNSUPPORTED;
    }
    if (ble_validate_config(config) != CHIP_OK) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (memcmp(VER_LIB, VER_FILE, strlen(VER_FILE)) != 0) {
        return CHIP_ERROR_UNSUPPORTED;
    }

    ble_config = *config;
    if (config->advertising_data_size == 0U) {
        size_t name_size = strlen(config->device_name);

        ble_default_advertising_data[0] = 2U;
        ble_default_advertising_data[1] = GAP_ADTYPE_FLAGS;
        ble_default_advertising_data[2] =
            GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;
        ble_default_advertising_data[3] = (uint8_t)(name_size + 1U);
        ble_default_advertising_data[4] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
        memcpy(&ble_default_advertising_data[5], config->device_name, name_size);
        ble_config.advertising_data = ble_default_advertising_data;
        ble_config.advertising_data_size = name_size + 5U;
    }
    memset(&vendor_config, 0, sizeof(vendor_config));
    vendor_config.MEMAddr = (uint32_t)ble_heap;
    vendor_config.MEMLen = CH585_BLE_HEAP_SIZE;
    vendor_config.BufMaxLen = CH585_BLE_BUFFER_MAX_LENGTH;
    vendor_config.BufNumber = CH585_BLE_BUFFER_COUNT;
    vendor_config.TxNumEvent = CH585_BLE_TX_PACKETS_PER_EVENT;
    vendor_config.TxPower = ble_tx_power_code(config->tx_power_dbm);
    vendor_config.ConnectNumber = 1U;
    vendor_config.srandCB = ble_random_seed;
    vendor_config.tsCB = ble_temperature_sample;
    vendor_config.rcCB = ble_calibrate_lsi;
    (void)GetMACAddress((uint8_t *)mac_words);
    memcpy(vendor_config.MacAddr, mac_words, sizeof(vendor_config.MacAddr));

    g_LLE_IRQLibHandlerLocation = (uint32_t)LLE_IRQLibHandler;
    PFIC_SetPriority(BLEL_IRQn, UINT8_C(0xF0));
    status = BLE_LibInit(&vendor_config);
    if (status != SUCCESS) {
        ble_state = CHIP_BLE_STATE_ERROR;
        return CHIP_ERROR_IO;
    }
    ble_state = CHIP_BLE_STATE_READY;
    if (ble_timer_init() != CHIP_OK) {
        ble_state = CHIP_BLE_STATE_ERROR;
        return CHIP_ERROR_IO;
    }
    status = GAPRole_PeripheralInit();
    if (status != SUCCESS) {
        ble_state = CHIP_BLE_STATE_ERROR;
        return CHIP_ERROR_IO;
    }

    advertising_enabled = config->advertise_on_start ? TRUE : FALSE;
    if ((GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED,
                              sizeof(advertising_enabled),
                              &advertising_enabled) != SUCCESS) ||
        (GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL,
                              sizeof(config->min_connection_interval),
                              (void *)&config->min_connection_interval) != SUCCESS) ||
        (GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL,
                              sizeof(config->max_connection_interval),
                              (void *)&config->max_connection_interval) != SUCCESS) ||
        ((ble_config.advertising_data_size != 0U) &&
         (GAPRole_SetParameter(GAPROLE_ADVERT_DATA,
                               (uint8_t)ble_config.advertising_data_size,
                               (void *)ble_config.advertising_data) != SUCCESS)) ||
        ((config->scan_response_data_size != 0U) &&
         (GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA,
                               (uint8_t)config->scan_response_data_size,
                               (void *)config->scan_response_data) != SUCCESS))) {
        ble_state = CHIP_BLE_STATE_ERROR;
        return CHIP_ERROR_IO;
    }

    GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, config->advertising_interval);
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, config->advertising_interval);
    (void)GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE,
                                  sizeof(pairing_mode), &pairing_mode);
    (void)GGS_AddService(GATT_ALL_SERVICES);
    (void)GATTServApp_AddService(GATT_ALL_SERVICES);
    if (GGS_SetParameter(GGS_DEVICE_NAME_ATT,
                         (uint8_t)strlen(config->device_name),
                         (void *)config->device_name) != SUCCESS) {
        ble_state = CHIP_BLE_STATE_ERROR;
        return CHIP_ERROR_IO;
    }

    ble_task_id = TMOS_ProcessEventRegister(ble_process_event);
    if (ble_task_id == INVALID_TASK_ID) {
        ble_state = CHIP_BLE_STATE_ERROR;
        return CHIP_ERROR_IO;
    }
    status = tmos_set_event(ble_task_id, CH585_BLE_START_EVENT);
    if (status != SUCCESS) {
        ble_state = CHIP_BLE_STATE_ERROR;
        return CHIP_ERROR_IO;
    }
    (void)tmos_start_task(ble_task_id, CH585_BLE_CALIBRATION_EVENT,
                          MS1_TO_SYSTEM_TIME(CH585_BLE_CALIBRATION_DELAY_MS));
    return CHIP_OK;
}

void chip_ble_process(void)
{
    if (ble_state != CHIP_BLE_STATE_UNINITIALIZED) {
        TMOS_SystemProcess();
    }
}

chip_status_t chip_ble_set_advertising(bool enabled)
{
    uint8_t vendor_enabled = enabled ? TRUE : FALSE;

    if (ble_state == CHIP_BLE_STATE_UNINITIALIZED) {
        return CHIP_ERROR_NOT_READY;
    }
    return (GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED,
                                 sizeof(vendor_enabled),
                                 &vendor_enabled) == SUCCESS) ?
           CHIP_OK : CHIP_ERROR_IO;
}

chip_ble_state_t chip_ble_state(void)
{
    return ble_state;
}

uint16_t chip_ble_connection_handle(void)
{
    return ble_connection_handle;
}

__INTERRUPT
__HIGH_CODE
void RTC_IRQHandler(void)
{
    R8_RTC_FLAG_CTRL = RB_RTC_TMR_CLR | RB_RTC_TRIG_CLR;
}
