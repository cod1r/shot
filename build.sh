#!/usr/bin/env sh
mkdir build
echo "Compiling shot.c..."
gcc src/shot.c -o build/shot -Wall -pedantic -Werror -Wextra -Wdiv-by-zero
echo "Done!"
