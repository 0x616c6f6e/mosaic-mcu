# OTA Component

`platform::ota` contains update mechanisms that are independent of a product:

- OTA image headers, CRC32, and image validation.
- A cached SPI NOR block device used by FatFs and USB MSC.
- Configurable external-Flash staging and committed-image validation.
- TinyUSB MSC and WebUSB update transports.
- Host-side package and factory-image generation tools.

Products provide policy and hardware layout. In particular, an application
must configure `ota_staging_t` with its target ID, application address, image
limit, erase geometry, and external-Flash offsets. USB descriptors, VID/PID,
product configuration schemas, board pins, signing keys, rollback rules, and
the final bootloader executable remain owned by the product.

Use `target_add_platform_ota_usb(target, config_directory)` to add the TinyUSB
MSC/WebUSB transport to an executable. `config_directory` must provide the
product's `tusb_config.h`. Pass `WEBUSB_CONFIG` when the product implements the
optional configuration commands, and `HID` when the same TinyUSB device is a
composite HID product.

The files in `tools/` implement the shared `FIRMWARE.OTA` package format and
factory-image assembly. CRC32 only detects corruption; production products
still require signature verification and an anti-rollback policy.
