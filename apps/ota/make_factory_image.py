#!/usr/bin/env python3
import argparse
import os
import tempfile


def integer(value: str) -> int:
    return int(value, 0)


def main() -> None:
    parser = argparse.ArgumentParser(description="Combine bootloader and application")
    parser.add_argument("--bootloader", required=True)
    parser.add_argument("--application", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--application-address", required=True, type=integer)
    parser.add_argument("--flash-size", required=True, type=integer)
    args = parser.parse_args()

    with open(args.bootloader, "rb") as source:
        bootloader = source.read()
    with open(args.application, "rb") as source:
        application = source.read()

    if len(bootloader) > args.application_address:
        raise SystemExit(
            f"bootloader size {len(bootloader)} exceeds its "
            f"{args.application_address}-byte partition"
        )
    if len(application) > args.flash_size - args.application_address:
        raise SystemExit("application exceeds program Flash")

    factory = (
        bootloader
        + bytes([0xFF]) * (args.application_address - len(bootloader))
        + application
    )
    output_dir = os.path.dirname(os.path.abspath(args.output))
    descriptor, temporary = tempfile.mkstemp(prefix=".factory-", dir=output_dir)
    try:
        with os.fdopen(descriptor, "wb") as destination:
            destination.write(factory)
        os.replace(temporary, args.output)
    except BaseException:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass
        raise


if __name__ == "__main__":
    main()
