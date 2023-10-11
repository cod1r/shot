#!/usr/bin/env sh
mkdir build
echo "Compiling shot.c..."
cc src/shot.c -o build/shot -Wall -Wpedantic -Wextra
echo "Done!"
