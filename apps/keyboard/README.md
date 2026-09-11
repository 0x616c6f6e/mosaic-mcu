# Mosaic Keyboard Firmware

This directory is the product assembly point for the CH585 keyboard. It owns
the board layout, USB identity, product configuration, application, recovery
bootloader, and release images. Reusable update mechanics come from
`platform::ota` instead of being copied into the product.

The current application is an update-capable keyboard example with a 5x14 GPIO
matrix and a standard Boot Keyboard HID interface.

The product uses a two-stage update flow with CH585 program Flash and an 8 MiB
SPI NOR Flash formatted with FAT:

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
`board/keyboard_board.c` and `board/keyboard_board.h` for another board or
Flash size.
The development default uses a conservative 500 kHz SPI clock; increase it only
after validating the board-level signal integrity.
Bootloader and application logs use UART0 on PB7/TX and PB4/RX at 115200 8N1.
Their startup records include the local build time.
The TinyUSB device uses the USBFS pins PB10/UDM and PB11/UDP. A connector wired
to the separate USBHS/USB2 pins PB12/PB13 will not enumerate with this firmware.

## Directory layout

| Directory | Responsibility |
| --- | --- |
| `bootloader/` | Recovery mode, update validation, and application programming |
| `application/` | Product application and persistent system configuration |
| `board/` | Keyboard PCB resources, Flash layout, and UART logging |
| `core/` | Matrix scan scheduling, debouncing, and Boot HID report generation |
| `usb/` | Product USB descriptors, VID/PID, and TinyUSB configuration |
| `cmake/` | Product firmware rules and factory-image orchestration |
| `web/` | Browser-based WebUSB updater |

Generic OTA image, storage, MSC, WebUSB, staging, and packaging code is in
`components/ota/`. Keyboard behavior must not be added to that component.
`application/keyboard_layout.c` is the product configuration point for row and
column pins, scan polarity, timing, and HID bindings. Matrix positions use
row-major order: `index = row * column_count + column`.

- Rows 0-4: PB5, PB3, PB2, PB15, PB9.
- Columns 0-13: PA7, PA8, PA9, PB8, PB17, PB16, PB14, PB6, PB1, PB0, PB21,
  PB20, PB19, PB18.
- Row 0: Escape, 1-0, Minus, Equal, Backspace.
- Row 1: Tab, Q-P, brackets, Backslash.
- Row 2: Caps Lock, A-L, Semicolon, Apostrophe, Enter at C12, C13 unused.
- Row 3: Left Shift, Z-M, punctuation, Right Shift, Up, Delete.
- Row 4: Left modifiers, Space at C3, direct-Alt hole at C4, Fn placeholder at
  C5, then Left, Down, Right at C6-C8; C9-C13 are unused.
- Direct key: PB22 with a 3.3 V pull-up, active low, mapped to Right Alt.

Rows are pulled up; inactive columns are high impedance and the active column
is driven low.

To adapt the matrix, edit `row_pins`, `column_pins`, `ROW_COUNT`,
`COLUMN_COUNT`, and the row-major `bindings` array in
`application/keyboard_layout.c`. Keep the matrix plus direct-key count at or
below `KEYBOARD_ENGINE_MAX_KEYS` (currently 80). The active-low example requires
switch/diode orientation that allows a pulled-up row to be pulled down through
the selected column. PB22 is polled as a separate 71st key and uses the same
five-scan debounce path as matrix keys; no GPIO interrupt is enabled.

## Build and first installation

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wch-riscv.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ch585_keyboard_factory
```

Program `build/apps/keyboard/FACTORY.hex` (or `FACTORY.bin` at address zero) once
with the hardware programmer. It contains both the bootloader and the initial
application with erased padding between them. This avoids a second ISP erase
accidentally removing the bootloader.

With the board in the WCH ROM ISP mode, the repository can build, flash and
verify the combined image in one command:

```sh
cmake --build build --target flash_ch585_keyboard_factory
```

Do not program the application `.bin` at address zero. The application ELF/HEX
is linked at `0x10000`; its `.bin` is intended only as input to the packer.

The build creates `build/apps/keyboard/FIRMWARE.OTA`. Copy that exact file to
the root of the `Keyboard Update` USB drive. After approximately two seconds
without further writes (or after eject), the application validates the complete
file and resets. The bootloader validates it again, installs it, and starts the
new application. An invalid or incomplete file is never written to program
Flash.

Set `-DKEYBOARD_FIRMWARE_VERSION=<number>` while configuring to populate the
package version field. Version rollback is not rejected in this first
implementation.

## WebUSB update

The application always exposes keyboard HID and WebUSB. With the disk visible,
it also exposes MSC (`VID:PID CAFE:4113`); disk-hidden mode uses `CAFE:4114`.
The recovery bootloader omits HID. On Windows 8 and later, the Microsoft OS 2.0
descriptor binds only the vendor interface to WinUSB while HID and the optional
disk use their system drivers.

Flash the new `FACTORY.hex` once when moving from the MSC-only firmware so both
the application and bootloader understand the staging format. On the first
boot, an existing 8 MiB FAT volume is reformatted to 7.5 MiB; files previously
stored on the OTA disk are removed during this one-time layout migration.

Serve the browser tool from localhost, then open it in Chrome or Edge:

```sh
python3 -m http.server 8000 --directory apps/keyboard/web
```

Open `http://localhost:8000` to update firmware or edit the system configuration.
Firmware is written in 512-byte blocks to a raw SPI Flash staging partition with
an acknowledgement after every block. The device checks the OTA header and
payload CRC before committing the package. It then resets, and the bootloader
validates and installs it again.

WebUSB requires a secure context; browsers treat `http://localhost` as secure.
For a remote web server, use HTTPS. Linux may also require a udev rule granting
the browser process access to USB VID `CAFE`.

The vendor interface uses bulk OUT endpoint 2 and bulk IN endpoint 2. Each
little-endian request is `"WOTA"`, command byte, reserved byte, `uint16`
payload length, `uint32` argument, then payload. Commands are `1=BEGIN`
(argument is package size), `2=DATA` (argument is sequential byte offset),
`3=END`, `4=ABORT`, `5=STATUS`, `6=GET_CONFIG`, and `7=SET_CONFIG`. Every
request returns a 16-byte `"ROTA"` response containing command, status, received
byte count, and total byte count. A successful `GET_CONFIG` response appends a
44-byte versioned configuration snapshot; `SET_CONFIG` accepts a schema 2 JSON
document of at most 512 bytes.

## System configuration

The application creates `SYSTEM.JSON` in the root of the USB disk when the file
does not exist. It has the same content as
`application/SYSTEM.JSON.example`:

```json
{
  "schema_version": 2,
  "device_name": "Mosaic Keyboard",
  "usb_disk_visible": true,
  "log_level": "debug",
  "usb_idle_timeout_ms": 2000,
  "webusb_reboot_delay_ms": 500
}
```

The configuration can be saved from the WebUSB tool or by editing this file on
the USB disk. The device disconnects, synchronizes SPI Flash, validates and
replaces the JSON document, applies it, and reconnects. A malformed or partially
written document is rejected and the current configuration remains active.

`device_name` accepts 1 to 31 ASCII letters, digits, spaces, `_`, `-`, and `.`;
it becomes the USB product string after reconnect. Set `usb_disk_visible` to
`false` to keep HID and WebUSB available without exposing a disk to the host.
`log_level` accepts `debug`, `info`, `warn`, `error`, or `none`. The idle
timeout range is 500 to 30000 ms, and the WebUSB reboot delay range is 100 to
5000 ms. All fields are required, unknown or duplicate fields are rejected, and
changes do not require a reboot.
The development default is `debug`; use `info` or a stricter level for normal
operation after bring-up.

Schema 1 files remain visible but are rejected until updated to schema 2. When
`usb_disk_visible` is `false`, both firmware updates and configuration remain
available through WebUSB, including the option to show the disk again.

## Production requirements

CRC32 detects incomplete or corrupted copies but does not authenticate an
image. Before production, add a signed manifest/public-key verification and a
monotonic anti-rollback policy. Raw NOR hosting FAT also has no wear leveling;
the write-back cache limits sequential firmware writes to roughly one erase per
4 KiB block, but frequent general-purpose file writes are not recommended.
