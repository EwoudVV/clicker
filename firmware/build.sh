#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p ../verification/2026-10-03
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashSize=4M \
  --libraries libraries --build-path ../work/build-remote remote \
  > ../verification/2026-10-03/remote-build.txt 2>&1
cat ../verification/2026-10-03/remote-build.txt
arduino-cli compile --fqbn esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,FlashSize=4M,PSRAM=disabled \
  --libraries libraries --build-path ../work/build-receiver receiver \
  > ../verification/2026-10-03/receiver-build.txt 2>&1
cat ../verification/2026-10-03/receiver-build.txt
