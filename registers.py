
# r means read only, w means write only, b means both, x means neither

registers = {
    0x00: ('xxbbrrrr', 'JOYP'),
    0x01: ('bbbbbbbb', 'SB'),
    0x02: ('bxxxxxxb', 'SC'),
    0x04: ('bbbbbbbb', 'DIV'),
    0x05: ('bbbbbbbb', 'TIMA'),
    0x06: ('bbbbbbbb', 'TMA'),
    0x07: ('bbbbbbbb', 'TAC'),
    0x08: ('bbbbbbbb', 'IF'),
    0x10: ('xbbbbbbb', 'NR10'),
    0x11: ('bbwwwwww', 'NR11'),
    0x12: ('bbbbbbbb', 'NR12'),
    0x13: ('wwwwwwww', 'NR13'),
    0x14: ('wbxxxwww', 'NR14'),
    0x16: ('bbwwwwww', 'NR21'),
    0x17: ('bbbbbbbb', 'NR22'),
    0x18: ('wwwwwwww', 'NR23'),
    0x19: ('wbxxxwww', 'NR24'),
    0x1A: ('bbbbbbbb', 'NR30'),
    0x1B: ('wwwwwwww', 'NR31'),
    0x1C: ('bbbbbbbb', 'NR32'),
    0x1D: ('wwwwwwww', 'NR33'),
    0x1E: ('wbxxxwww', 'NR34'),
    0x20: ('wwwwwwww', 'NR41'),
    0x21: ('bbbbbbbb', 'NR42'),
    0x22: ('bbbbbbbb', 'NR43'),
    0x23: ('wbxxxxxx', 'NR44'),
    0x24: ('bbbbbbbb', 'NR50'),
    0x25: ('bbbbbbbb', 'NR51'),
    0x26: ('bxxxrrrr', 'NR52'),
    0x30: ('bbbbbbbb', 'WAVE 1'),
    0x31: ('bbbbbbbb', 'WAVE 2'),
    0x32: ('bbbbbbbb', 'WAVE 3'),
    0x33: ('bbbbbbbb', 'WAVE 4'),
    0x34: ('bbbbbbbb', 'WAVE 5'),
    0x35: ('bbbbbbbb', 'WAVE 6'),
    0x36: ('bbbbbbbb', 'WAVE 7'),
    0x37: ('bbbbbbbb', 'WAVE 8'),
    0x38: ('bbbbbbbb', 'WAVE 9'),
    0x39: ('bbbbbbbb', 'WAVE 10'),
    0x3A: ('bbbbbbbb', 'WAVE 11'),
    0x3B: ('bbbbbbbb', 'WAVE 12'),
    0x3C: ('bbbbbbbb', 'WAVE 13'),
    0x3D: ('bbbbbbbb', 'WAVE 14'),
    0x3E: ('bbbbbbbb', 'WAVE 15'),
    0x3F: ('bbbbbbbb', 'WAVE 16'),
    0x40: ('bbbbbbbb', 'LCDC'),
    0x41: ('xbbbbrrr', 'STAT'),
    0x42: ('bbbbbbbb', 'SCY'),
    0x43: ('bbbbbbbb', 'SCX'),
    0x44: ('rrrrrrrr', 'LY'),
    0x45: ('bbbbbbbb', 'LYC'),
    0x46: ('bbbbbbbb', 'DMA'),
    0x47: ('bbbbbbbb', 'BGP'),
    0x48: ('bbbbbbbb', 'OBP0'),
    0x49: ('bbbbbbbb', 'OBP1'),
    0x4A: ('bbbbbbbb', 'WX'),
    0x4B: ('bbbbbbbb', 'WY'),
    0x72: ('bbbbbbbb', '???'),
    0x73: ('bbbbbbbb', '???'),
    0x75: ('xbbbxxxx', '???'),
    0xFF: ('bbbbbbbb', 'IE')
}

outfile = open("registers.h", 'w')

outfile.write("""/* Header file to encode which DMG registers are read and write only, by defining bitmasks
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#ifndef REGISTERS_H
#define REGISTERS_H
uint8_t write_masks[256] = { // for each byte, a '1' means a bit is writable
""")

for i in range(256):
    bitmask, regname = registers.get(i, ("xxxxxxxx", None))
    bitmask = bitmask.replace('b', '1')
    bitmask = bitmask.replace('w', '1')
    bitmask = bitmask.replace('r', '0')
    bitmask = bitmask.replace('x', '0')
    outfile.write(f"    0b{bitmask}{'' if i==255 else ','}{(' // ' + regname) if regname is not None else ''}\n")

outfile.write("};\n\nuint8_t read_masks[256] = { // for each byte, a '0' means a bit is readable\n")

for i in range(256):
    bitmask, regname = registers.get(i, ("xxxxxxxx", None))
    bitmask = bitmask.replace('b', '0')
    bitmask = bitmask.replace('w', '1')
    bitmask = bitmask.replace('r', '0')
    bitmask = bitmask.replace('x', '1')
    outfile.write(f"    0b{bitmask}{'' if i==255 else ','}{(' // ' + regname) if regname is not None else ''}\n")
outfile.write("};\n#endif //REGISTERS\n")

outfile.close()