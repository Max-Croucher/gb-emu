/* Main source file for the gbemu gameboy emulator.
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "rom.h"

int main(int argc, char *argv[]) {
    gbRom rom = load_rom(argv[1]);
    return 0;
}