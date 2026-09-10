#!/usr/bin/env python3
import argparse
import os
import struct
import tempfile
import zlib


MAGIC = 0x41544F4D
FORMAT_VERSION = 1
HEADER_FORMAT = "<9I"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)


def integer(value: str) -> int:
    return int(value, 0)


def main() -> None:
    parser = argparse.ArgumentParser(description="Build a Mosaic MCU OTA image")
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--target", required=True, type=integer)
    parser.add_argument("--address", required=True, type=integer)
    parser.add_argument("--max-size", required=True, type=integer)
    parser.add_argument("--version", required=True, type=integer)
    args = parser.parse_args()

    with open(args.input, "rb") as source:
        image = source.read()
    if not image or len(image) > args.max_size:
        raise SystemExit(
            f"application size {len(image)} is outside 1..{args.max_size} bytes"
        )

    image_crc = zlib.crc32(image) & 0xFFFFFFFF
    header_without_crc = struct.pack(
        "<8I",
        MAGIC,
        FORMAT_VERSION,
        HEADER_SIZE,
        args.target,
        args.address,
        len(image),
        image_crc,
        args.version,
    )
    header_crc = zlib.crc32(header_without_crc) & 0xFFFFFFFF
    header = header_without_crc + struct.pack("<I", header_crc)

    output_dir = os.path.dirname(os.path.abspath(args.output))
    descriptor, temporary = tempfile.mkstemp(prefix=".ota-", dir=output_dir)
    try:
        with os.fdopen(descriptor, "wb") as destination:
            destination.write(header)
            destination.write(image)
        os.replace(temporary, args.output)
    except BaseException:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass
        raise


if __name__ == "__main__":
    main()
