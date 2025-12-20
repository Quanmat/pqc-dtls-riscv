#!/bin/bash

set -eo pipefail

echo "Generating sim build files (first run)..."

. ./litex-env/bin/activate

TARGET_WORD="=== Boot ==="
LOGFILE="tmp.log"
PROCESS_NAME="litex_sim"

# litex_Sim requires tty so using script
script -q -c "
    litex_sim --csr-json csr.json \
              --cpu-type=vexriscv \
              --cpu-variant=full \
              --integrated-main-ram-size=0x06400000 \
              --with-ethernet \
              --sys-clk-freq 100000000
" $LOGFILE &
SCRIPT_PID=$!

LITEX_PID=""

# wait until script creates log file and litex_sim starts
while [ -z "$LITEX_PID" ]; do
    for p in /proc/[0-9]*; do
        pid="${p#/proc/}"
        if [ -r "/proc/$pid/comm" ]; then
            name=$(cat "/proc/$pid/comm" 2>/dev/null)
            if [ "$name" = "$PROCESS_NAME" ]; then
                LITEX_PID=$pid
                break
            fi
        fi
    done
done

# monitor log output in real time
(
    # use script again to capture streaming logs
    script -q /dev/null <<EOF
while true; do
    if [ -f "$LOGFILE" ]; then
        # read new lines
        while read line; do
            echo "\$line"
        done < "$LOGFILE"
    fi
    sleep 1
done
EOF
) | while read line; do
    case "$line" in
        *"$TARGET_WORD"*)
            kill "$LITEX_PID"
            kill "$SCRIPT_PID"
            exit 0
            ;;
    case "$line" in
        *"[sudo] password for"*)
            kill "$LITEX_PID"
            kill "$SCRIPT_PID"
            exit 0
            ;;
    esac
done

rm -f "$LOGFILE" 2> /dev/null
