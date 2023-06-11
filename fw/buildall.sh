#!/bin/sh
set -e
for BOARD in twinkie twonkie twonkiev2; do
    make BOARD=$BOARD clean
    make BOARD=$BOARD
    cp build/$BOARD/ec.bin $BOARD-$(date +%Y%m%d).bin
done
