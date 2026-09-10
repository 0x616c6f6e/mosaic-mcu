#include <chip_ble_hid_keyboard.h>

#include <string.h>

#include <chip_ble.h>

#include "CH58xBLE_LIB.h"

#define HID_PROTOCOL_BOOT       0U
#define HID_PROTOCOL_REPORT     1U
#define HID_REPORT_TYPE_INPUT   1U
#define HID_REPORT_TYPE_OUTPUT  2U
#define HID_COMMAND_SUSPEND     0U
#define HID_COMMAND_EXIT_SUSPEND 1U
#define HID_REPORT_SIZE         8U

static const uint8_t hid_service_uuid[] = {
    LO_UINT16(HID_SERV_UUID), HI_UINT16(HID_SERV_UUID)
};
static const uint8_t hid_info_uuid[] = {
    LO_UINT16(HID_INFORMATION_UUID), HI_UINT16(HID_INFORMATION_UUID)
};
static const uint8_t hid_report_map_uuid[] = {
    LO_UINT16(REPORT_MAP_UUID), HI_UINT16(REPORT_MAP_UUID)
};
static const uint8_t hid_control_point_uuid[] = {
    LO_UINT16(HID_CTRL_PT_UUID), HI_UINT16(HID_CTRL_PT_UUID)
};
static const uint8_t hid_protocol_mode_uuid[] = {
    LO_UINT16(PROTOCOL_MODE_UUID), HI_UINT16(PROTOCOL_MODE_UUID)
};
static const uint8_t hid_report_uuid[] = {
    LO_UINT16(REPORT_UUID), HI_UINT16(REPORT_UUID)
};
static const uint8_t hid_boot_input_uuid[] = {
    LO_UINT16(BOOT_KEY_INPUT_UUID), HI_UINT16(BOOT_KEY_INPUT_UUID)
};
static const uint8_t hid_boot_output_uuid[] = {
    LO_UINT16(BOOT_KEY_OUTPUT_UUID), HI_UINT16(BOOT_KEY_OUTPUT_UUID)
};

static const gattAttrType_t hid_service = {
    ATT_BT_UUID_SIZE, hid_service_uuid
};
static const uint8_t hid_information[] = {
    LO_UINT16(0x0111), HI_UINT16(0x0111), 0U, 0x01U
};
static const uint8_t hid_report_map[] = {
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01,
    0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7,
    0x15, 0x00, 0x25, 0x01, 0x75, 0x01,
    0x95, 0x08, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
    0x95, 0x05, 0x75, 0x01, 0x05, 0x08,
    0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
    0x95, 0x01, 0x75, 0x03, 0x91, 0x01,
    0x95, 0x06, 0x75, 0x08, 0x15, 0x00,
    0x25, 0x65, 0x05, 0x07, 0x19, 0x00,
    0x29, 0x65, 0x81, 0x00, 0xC0,
};

static uint8_t hid_info_properties = GATT_PROP_READ;
static uint8_t hid_control_properties = GATT_PROP_WRITE_NO_RSP;
static uint8_t hid_protocol_properties = GATT_PROP_READ | GATT_PROP_WRITE_NO_RSP;
static uint8_t hid_map_properties = GATT_PROP_READ;
static uint8_t hid_input_properties = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t hid_output_properties =
    GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
static uint8_t hid_boot_input_properties = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t hid_boot_output_properties =
    GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;

static uint8_t hid_control_point;
static uint8_t hid_protocol_mode = HID_PROTOCOL_REPORT;
static uint8_t hid_input_report[HID_REPORT_SIZE];
static uint8_t hid_output_report;
static uint8_t hid_boot_input_report[HID_REPORT_SIZE];
static uint8_t hid_boot_output_report;
static uint8_t hid_input_report_reference[] = {0U, HID_REPORT_TYPE_INPUT};
static uint8_t hid_output_report_reference[] = {0U, HID_REPORT_TYPE_OUTPUT};
static gattCharCfg_t hid_input_cccd[GATT_MAX_NUM_CONN];
static gattCharCfg_t hid_boot_input_cccd[GATT_MAX_NUM_CONN];
static chip_ble_hid_keyboard_config_t hid_config;
static bool hid_initialized;

enum {
    HID_SERVICE_INDEX,
    HID_INFO_DECL_INDEX,
    HID_INFO_INDEX,
    HID_CONTROL_DECL_INDEX,
    HID_CONTROL_INDEX,
    HID_PROTOCOL_DECL_INDEX,
    HID_PROTOCOL_INDEX,
    HID_MAP_DECL_INDEX,
    HID_MAP_INDEX,
    HID_INPUT_DECL_INDEX,
    HID_INPUT_INDEX,
    HID_INPUT_CCCD_INDEX,
    HID_INPUT_REFERENCE_INDEX,
    HID_OUTPUT_DECL_INDEX,
    HID_OUTPUT_INDEX,
    HID_OUTPUT_REFERENCE_INDEX,
    HID_BOOT_INPUT_DECL_INDEX,
    HID_BOOT_INPUT_INDEX,
    HID_BOOT_INPUT_CCCD_INDEX,
    HID_BOOT_OUTPUT_DECL_INDEX,
    HID_BOOT_OUTPUT_INDEX,
};

static uint8_t hid_read_attribute(uint16_t connection_handle,
                                  gattAttribute_t *attribute, uint8_t *value,
                                  uint16_t *length, uint16_t offset,
                                  uint16_t max_length, uint8_t method);
static uint8_t hid_write_attribute(uint16_t connection_handle,
                                   gattAttribute_t *attribute, uint8_t *value,
                                   uint16_t length, uint16_t offset,
                                   uint8_t method);

static gattServiceCBs_t hid_callbacks = {
    .pfnReadAttrCB = hid_read_attribute,
    .pfnWriteAttrCB = hid_write_attribute,
    .pfnAuthorizeAttrCB = NULL,
};

static gattAttribute_t hid_attributes[] = {
    {{ATT_BT_UUID_SIZE, primaryServiceUUID}, GATT_PERMIT_READ, 0U,
     (uint8_t *)&hid_service},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0U,
     &hid_info_properties},
    {{ATT_BT_UUID_SIZE, hid_info_uuid}, GATT_PERMIT_ENCRYPT_READ, 0U,
     (uint8_t *)hid_information},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0U,
     &hid_control_properties},
    {{ATT_BT_UUID_SIZE, hid_control_point_uuid}, GATT_PERMIT_ENCRYPT_WRITE, 0U,
     &hid_control_point},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0U,
     &hid_protocol_properties},
    {{ATT_BT_UUID_SIZE, hid_protocol_mode_uuid},
     GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0U,
     &hid_protocol_mode},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0U,
     &hid_map_properties},
    {{ATT_BT_UUID_SIZE, hid_report_map_uuid}, GATT_PERMIT_ENCRYPT_READ, 0U,
     (uint8_t *)hid_report_map},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0U,
     &hid_input_properties},
    {{ATT_BT_UUID_SIZE, hid_report_uuid}, GATT_PERMIT_ENCRYPT_READ, 0U,
     hid_input_report},
    {{ATT_BT_UUID_SIZE, clientCharCfgUUID},
     GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0U,
     (uint8_t *)hid_input_cccd},
    {{ATT_BT_UUID_SIZE, reportRefUUID}, GATT_PERMIT_READ, 0U,
     hid_input_report_reference},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0U,
     &hid_output_properties},
    {{ATT_BT_UUID_SIZE, hid_report_uuid},
     GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0U,
     &hid_output_report},
    {{ATT_BT_UUID_SIZE, reportRefUUID}, GATT_PERMIT_READ, 0U,
     hid_output_report_reference},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0U,
     &hid_boot_input_properties},
    {{ATT_BT_UUID_SIZE, hid_boot_input_uuid}, GATT_PERMIT_ENCRYPT_READ, 0U,
     hid_boot_input_report},
    {{ATT_BT_UUID_SIZE, clientCharCfgUUID},
     GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0U,
     (uint8_t *)hid_boot_input_cccd},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0U,
     &hid_boot_output_properties},
    {{ATT_BT_UUID_SIZE, hid_boot_output_uuid},
     GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0U,
     &hid_boot_output_report},
};

static uint16_t hid_uuid(const gattAttribute_t *attribute)
{
    return BUILD_UINT16(attribute->type.uuid[0], attribute->type.uuid[1]);
}

static uint8_t hid_copy_read(uint8_t *destination, uint16_t *output_length,
                             uint16_t max_length, const uint8_t *source,
                             uint16_t source_length, uint16_t offset)
{
    uint16_t copy_length;

    if (offset > source_length) {
        return ATT_ERR_INVALID_OFFSET;
    }
    copy_length = (uint16_t)(source_length - offset);
    if (copy_length > max_length) {
        copy_length = max_length;
    }
    memcpy(destination, &source[offset], copy_length);
    *output_length = copy_length;
    return SUCCESS;
}

static uint8_t hid_read_attribute(uint16_t connection_handle,
                                  gattAttribute_t *attribute, uint8_t *value,
                                  uint16_t *length, uint16_t offset,
                                  uint16_t max_length, uint8_t method)
{
    uint16_t uuid = hid_uuid(attribute);

    (void)method;
    if (uuid == REPORT_MAP_UUID) {
        return hid_copy_read(value, length, max_length, hid_report_map,
                             sizeof(hid_report_map), offset);
    }
    if (offset != 0U) {
        return ATT_ERR_ATTR_NOT_LONG;
    }

    switch (uuid) {
    case HID_INFORMATION_UUID:
        return hid_copy_read(value, length, max_length, hid_information,
                             sizeof(hid_information), 0U);
    case PROTOCOL_MODE_UUID:
        return hid_copy_read(value, length, max_length, &hid_protocol_mode,
                             1U, 0U);
    case REPORT_UUID:
        if (attribute->handle == hid_attributes[HID_INPUT_INDEX].handle) {
            return hid_copy_read(value, length, max_length, hid_input_report,
                                 HID_REPORT_SIZE, 0U);
        }
        return hid_copy_read(value, length, max_length, &hid_output_report,
                             1U, 0U);
    case BOOT_KEY_INPUT_UUID:
        return hid_copy_read(value, length, max_length, hid_boot_input_report,
                             HID_REPORT_SIZE, 0U);
    case BOOT_KEY_OUTPUT_UUID:
        return hid_copy_read(value, length, max_length,
                             &hid_boot_output_report, 1U, 0U);
    case GATT_REPORT_REF_UUID:
        return hid_copy_read(value, length, max_length, attribute->pValue,
                             2U, 0U);
    case GATT_CLIENT_CHAR_CFG_UUID: {
        uint16_t configuration = GATTServApp_ReadCharCfg(
            connection_handle, (gattCharCfg_t *)attribute->pValue);
        uint8_t encoded[] = {
            (uint8_t)LO_UINT16(configuration),
            (uint8_t)HI_UINT16(configuration)
        };
        return hid_copy_read(value, length, max_length, encoded, 2U, 0U);
    }
    default:
        *length = 0U;
        return SUCCESS;
    }
}

static void hid_output_changed(uint8_t leds)
{
    if (hid_config.led_callback != NULL) {
        hid_config.led_callback(leds, hid_config.led_context);
    }
}

static uint8_t hid_write_attribute(uint16_t connection_handle,
                                   gattAttribute_t *attribute, uint8_t *value,
                                   uint16_t length, uint16_t offset,
                                   uint8_t method)
{
    uint16_t uuid = hid_uuid(attribute);

    (void)method;
    if (offset != 0U) {
        return ATT_ERR_ATTR_NOT_LONG;
    }

    switch (uuid) {
    case REPORT_UUID:
        if ((attribute->handle != hid_attributes[HID_OUTPUT_INDEX].handle) ||
            (length != 1U)) {
            return ATT_ERR_INVALID_VALUE_SIZE;
        }
        hid_output_report = value[0];
        hid_output_changed(value[0]);
        return SUCCESS;
    case BOOT_KEY_OUTPUT_UUID:
        if (length != 1U) {
            return ATT_ERR_INVALID_VALUE_SIZE;
        }
        hid_boot_output_report = value[0];
        hid_output_changed(value[0]);
        return SUCCESS;
    case HID_CTRL_PT_UUID:
        if ((length != 1U) ||
            ((value[0] != HID_COMMAND_SUSPEND) &&
             (value[0] != HID_COMMAND_EXIT_SUSPEND))) {
            return ATT_ERR_INVALID_VALUE;
        }
        hid_control_point = value[0];
        return SUCCESS;
    case PROTOCOL_MODE_UUID:
        if ((length != 1U) ||
            ((value[0] != HID_PROTOCOL_BOOT) &&
             (value[0] != HID_PROTOCOL_REPORT))) {
            return ATT_ERR_INVALID_VALUE;
        }
        hid_protocol_mode = value[0];
        return SUCCESS;
    case GATT_CLIENT_CHAR_CFG_UUID:
        return GATTServApp_ProcessCCCWriteReq(connection_handle, attribute,
                                              value, length, offset,
                                              GATT_CLIENT_CFG_NOTIFY);
    default:
        return ATT_ERR_ATTR_NOT_FOUND;
    }
}

static chip_status_t hid_notify(uint16_t connection_handle, uint16_t handle,
                                const uint8_t *report)
{
    attHandleValueNoti_t notification;
    bStatus_t status;

    notification.pValue = GATT_bm_alloc(connection_handle,
                                        ATT_HANDLE_VALUE_NOTI,
                                        HID_REPORT_SIZE, NULL, 0U);
    if (notification.pValue == NULL) {
        return CHIP_ERROR_BUSY;
    }
    notification.handle = handle;
    notification.len = HID_REPORT_SIZE;
    memcpy(notification.pValue, report, HID_REPORT_SIZE);
    status = GATT_Notification(connection_handle, &notification, FALSE);
    if (status != SUCCESS) {
        GATT_bm_free((gattMsg_t *)&notification, ATT_HANDLE_VALUE_NOTI);
        return (status == bleNoResources) ? CHIP_ERROR_BUSY : CHIP_ERROR_IO;
    }
    return CHIP_OK;
}

chip_status_t chip_ble_hid_keyboard_init(
    const chip_ble_hid_keyboard_config_t *config)
{
    uint32_t passkey = 0U;
    uint8_t pairing_mode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
    uint8_t mitm = FALSE;
    uint8_t io_capability = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t bonding = FALSE;
    uint16_t appearance = GAP_APPEARE_HID_KEYBOARD;

    if (hid_initialized) {
        return CHIP_ERROR_BUSY;
    }
    if (config == NULL) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (chip_ble_state() == CHIP_BLE_STATE_UNINITIALIZED) {
        return CHIP_ERROR_NOT_READY;
    }

    hid_config = *config;
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hid_input_cccd);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hid_boot_input_cccd);
    if ((GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE,
                                 sizeof(passkey), &passkey) != SUCCESS) ||
        (GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE,
                                 sizeof(pairing_mode),
                                 &pairing_mode) != SUCCESS) ||
        (GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION,
                                 sizeof(mitm), &mitm) != SUCCESS) ||
        (GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES,
                                 sizeof(io_capability),
                                 &io_capability) != SUCCESS) ||
        (GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED,
                                 sizeof(bonding), &bonding) != SUCCESS) ||
        (GGS_SetParameter(GGS_APPEARANCE_ATT, sizeof(appearance),
                          &appearance) != SUCCESS) ||
        (GATTServApp_RegisterService(hid_attributes,
                                     GATT_NUM_ATTRS(hid_attributes),
                                     GATT_MAX_ENCRYPT_KEY_SIZE,
                                     &hid_callbacks) != SUCCESS)) {
        return CHIP_ERROR_IO;
    }
    hid_initialized = true;
    return CHIP_OK;
}

bool chip_ble_hid_keyboard_ready(void)
{
    uint16_t connection_handle = chip_ble_connection_handle();
    gattCharCfg_t *cccd = (hid_protocol_mode == HID_PROTOCOL_BOOT) ?
        hid_boot_input_cccd : hid_input_cccd;

    return hid_initialized &&
           (connection_handle != CHIP_BLE_CONN_HANDLE_INVALID) &&
           ((GATTServApp_ReadCharCfg(connection_handle, cccd) &
             GATT_CLIENT_CFG_NOTIFY) != 0U);
}

chip_status_t chip_ble_hid_keyboard_send(
    uint8_t modifiers,
    const uint8_t keycodes[CHIP_BLE_HID_KEYBOARD_KEY_COUNT])
{
    uint16_t connection_handle = chip_ble_connection_handle();
    uint8_t *report = (hid_protocol_mode == HID_PROTOCOL_BOOT) ?
        hid_boot_input_report : hid_input_report;
    uint16_t report_handle = (hid_protocol_mode == HID_PROTOCOL_BOOT) ?
        hid_attributes[HID_BOOT_INPUT_INDEX].handle :
        hid_attributes[HID_INPUT_INDEX].handle;

    if (!hid_initialized || (keycodes == NULL)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!chip_ble_hid_keyboard_ready()) {
        return CHIP_ERROR_NOT_READY;
    }
    report[0] = modifiers;
    report[1] = 0U;
    memcpy(&report[2], keycodes, CHIP_BLE_HID_KEYBOARD_KEY_COUNT);
    return hid_notify(connection_handle, report_handle, report);
}

void ch585_ble_disconnected_hook(uint16_t connection_handle)
{
    if (hid_initialized) {
        GATTServApp_InitCharCfg(connection_handle, hid_input_cccd);
        GATTServApp_InitCharCfg(connection_handle, hid_boot_input_cccd);
        hid_protocol_mode = HID_PROTOCOL_REPORT;
    }
}
