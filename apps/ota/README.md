# CH585 USB MSC + WebUSB OTA

This application implements a two-stage update flow using the CH585 program
Flash and an 8 MiB SPI NOR Flash formatted with FAT:

```text
host copies FIRMWARE.OTA -> USB MSC/FAT -> SPI NOR -> reset
or browser sends FIRMWARE.OTA -> WebUSB staging area -> reset
    -> bootloader validates CRC32 -> erases/programs/verifies application
    -> deletes FIRMWARE.OTA -> reset -> application at 0x10000
```

## Flash layout

| Region | Address | Size |
| --- | ---: | ---: |
| Bootloader | `0x00000000` | 64 KiB |
| Application | `0x00010000` | 384 KiB |
| Install marker (Data-Flash) | `0x00007F00` | 256 bytes |
| FAT volume (external SPI NOR) | `0x000000` | 7.5 MiB |
| WebUSB commit metadata | `0x780000` | 4 KiB |
| WebUSB OTA package | `0x781000` | 508 KiB |

The install marker is programmed before the application is erased and cleared
only after every programmed block has been verified. If power is lost during an
update, the bootloader stays in USB recovery mode and the same image can be
copied again. An erased marker also permits a factory-programmed application to
boot.

The default SPI1 pins are PA0/SCK, PA1/MOSI, PA2/MISO and PA3/CS. Change
`common/ota_board.c` and `common/ota_board.h` for another board or Flash size.
Bootloader and application logs use UART0 on PB7/TX and PB4/RX at 115200 8N1.
The TinyUSB device uses the USBFS pins PB10/UDM and PB11/UDP. A connector wired
to the separate USBHS/USB2 pins PB12/PB13 will not enumerate with this firmware.

## Build and first installation

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wch-riscv.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ch585_ota_factory
```

Program `build/apps/ota/FACTORY.hex` (or `FACTORY.bin` at address zero) once
with the hardware programmer. It contains both the bootloader and the initial
application with erased padding between them. This avoids a second ISP erase
accidentally removing the bootloader.

With the board in the WCH ROM ISP mode, the repository can build, flash and
verify the combined image in one command:

```sh
cmake --build build --target flash_ch585_ota_factory
```

Do not program the application `.bin` at address zero. The application ELF/HEX
is linked at `0x10000`; its `.bin` is intended only as input to the packer.

The build creates `build/apps/ota/FIRMWARE.OTA`. Copy that exact file to the
root of the `CH585 OTA Disk` USB drive. After approximately two seconds without
further writes (or after eject), the application validates the complete file
and resets. The bootloader validates it again, installs it, and starts the new
application. An invalid or incomplete file is never written to program Flash.

Set `-DOTA_FIRMWARE_VERSION=<number>` while configuring to populate the package
version field. Version rollback is not rejected in this first implementation.

## WebUSB update

The same USB connection enumerates as a composite device with an MSC interface
and a vendor interface (`VID:PID CAFE:4113`). On Windows 8 and later, its
Microsoft OS 2.0 descriptor binds only the vendor interface to WinUSB while the
disk continues to use the system mass-storage driver.

Flash the new `FACTORY.hex` once when moving from the MSC-only firmware so both
the application and bootloader understand the staging format. On the first
boot, an existing 8 MiB FAT volume is reformatted to 7.5 MiB; files previously
stored on the OTA disk are removed during this one-time layout migration.

Serve the browser tool from localhost, then open it in Chrome or Edge:

```sh
python3 -m http.server 8000 --directory apps/ota/web
```

Open `http://localhost:8000`, select **Connect device**, choose
`FIRMWARE.OTA`, and start the update. The browser writes 512-byte blocks to a
raw SPI Flash staging partition and waits for an acknowledgement after every
block. The device checks the OTA header and payload CRC before committing the
package. It then resets, and the bootloader validates and installs it again.

WebUSB requires a secure context; browsers treat `http://localhost` as secure.
For a remote web server, use HTTPS. Linux may also require a udev rule granting
the browser process access to USB VID `CAFE`.

The vendor interface uses bulk OUT endpoint 2 and bulk IN endpoint 2. Each
little-endian request is `"WOTA"`, command byte, reserved byte, `uint16`
payload length, `uint32` argument, then payload. Commands are `1=BEGIN`
(argument is package size), `2=DATA` (argument is sequential byte offset),
`3=END`, `4=ABORT`, and `5=STATUS`. Every request returns a 16-byte `"ROTA"`
response containing command, status, received byte count, and total byte count.

## System configuration

The application creates `SYSTEM.JSON` in the root of the USB disk when the file
does not exist. It has the same content as `SYSTEM.JSON.example`:

```json
{
  "schema_version": 1,
  "device_name": "CH585 OTA",
  "log_level": "info",
  "usb_idle_timeout_ms": 2000,
  "webusb_reboot_delay_ms": 500
}
```

Edit and save this file directly on the USB disk. After writes have stopped for
`usb_idle_timeout_ms` (or after safe eject), the device disconnects the disk,
synchronizes SPI Flash, validates the complete JSON document, applies it, and
reconnects. A malformed or partially written file is rejected and the current
in-memory configuration remains active.

`device_name` accepts 1 to 31 ASCII letters, digits, spaces, `_`, `-`, and `.`;
it becomes the USB product string after reconnect. `log_level` accepts `debug`,
`info`, `warn`, `error`, or `none`. The idle timeout range is 500 to 30000 ms,
and the WebUSB reboot delay range is 100 to 5000 ms. All fields are required,
unknown or duplicate fields are rejected, and changes do not require a reboot.

## Production requirements

CRC32 detects incomplete or corrupted copies but does not authenticate an
image. Before production, add a signed manifest/public-key verification and a
monotonic anti-rollback policy. Raw NOR hosting FAT also has no wear leveling;
the write-back cache limits sequential firmware writes to roughly one erase per
4 KiB block, but frequent general-purpose file writes are not recommended.
