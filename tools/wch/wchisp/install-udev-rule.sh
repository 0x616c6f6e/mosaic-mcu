#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
rule_name=50-wchisp.rules

sudo install -m 0644 "$script_dir/$rule_name" "/etc/udev/rules.d/$rule_name"
sudo udevadm control --reload-rules
sudo udevadm trigger --subsystem-match=usb \
    --attr-match=idVendor=1a86 --attr-match=idProduct=55e0
sudo udevadm settle

echo "Installed /etc/udev/rules.d/$rule_name"
