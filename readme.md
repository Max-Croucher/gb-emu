# Reasonably functional gameboy emulator written in C
This project is for my own learning and experience, and is therefore written without any use of AI. Written for Linux, and completely untested on Windows or devices with a different byte order. This project is very much a work-in-progress.

## Things that work:
 - Many games, including Tetris, Kirby and Link's Awakening
 - Intructions are implemented for machine cycle accuracy
 - Blargg CPU instruction tests
 - Common MBANK controllers (MBC1, MBC2, MBC3 (without clock), MBC5)
 - Saving game progress by reading/writing external RAM to a save file. This file defaults to a `.sav` and is compatible with other emulators
 - Interrupts and timers
 - Graphics, including the Acid2 test
 - Screenshots can be saved with ctrl+f
 - Sound, although sometimes the noise channel sounds wrong
 - Framerate limiting to 59.7Hz
 - Tilemap viewer and debugger

## Next Steps:
 - Pass more Blargg and Mooneye tests
 - Implement accurate ppu scanline timing
 - Correctly mask register reads and writes
 - Add the ability to output a screenshot on breakpoint
 - Refactor objects on the address bus
 - Fix some dma edge cases
 - Implement MBC3 clock support
 - Implement the ability to create/use save states

## Lofty Goals:
 - GBC support
 - Serial I/O
 - Do some serious optimisation to achieve >500 FPS

# How to Use
 Build the project using the makefile, and run with `./gbemu <rom-filename>`
 
 The following command line arguments are also available:
 - `--halt-on-breakpoint` will cause the emulator to stop when the instruction `LD B B` is encountered and print the contents of the CPU's registers.
 - `--print-breakpoint` will cause the emulator to print the contents of the CPU's registers when the instruction `LD B B` is encountered, but not stop.
 - `--no-save` will prevent the emulator from saving the contents of external RAM (if applicable) to a file
 - `--custom-filename <filename>` will cause the emulator to save the contents of external RAM to the specified file. This file defaults to `rom-filename.sav`
 - `--max-speed` removes the 59.7Hz limit, allowing the emulator to run as fast as it can.
 - `--windowless` will stop OpenGL from initialising, and stops the PPU from ticking. Useful for automating tests that don't need graphics.
 - `--debug` will cause the emulator to write a detailed log of the CPU state before every instruction is executed.
 - `--tilemap` will open a second window that displays the contents of VRAM, tilemaps and OAM. This window is updated every frame.
 - `--scanline` will also open the second window, but will update the window every scanline. Waits for newlines in STDIN to draw the next scanline.
 - `--green` will swap the screen's palette for the original gameboy's universally loved puke green colours.
 - `--no-audio` will completely disable the audio engine.
 - `--export-wav` will enable the output of the gameboy's four audio channels to a 4-channel wav file