
# r means read only, w means write only, b means both, x means neither

registers = {
    0x00: ('xxbbrrrr', 'JOYP', 0xCF),
    0x01: ('bbbbbbbb', 'SB', 0x00),
    0x02: ('bxxxxxxb', 'SC', 0x7E),
    0x04: ('bbbbbbbb', 'DIV', 0xAB),
    0x05: ('bbbbbbbb', 'TIMA', 0x00),
    0x06: ('bbbbbbbb', 'TMA', 0x00),
    0x07: ('xxxxxbbb', 'TAC', 0xF8),
    0x0F: ('xxxbbbbb', 'IF', 0xE1),
    0x10: ('xbbbbbbb', 'NR10', 0x80),
    0x11: ('bbwwwwww', 'NR11', 0xBF),
    0x12: ('bbbbbbbb', 'NR12', 0xF3),
    0x13: ('wwwwwwww', 'NR13', 0xFF),
    0x14: ('wbxxxwww', 'NR14', 0xBF),
    0x16: ('bbwwwwww', 'NR21', 0x3F),
    0x17: ('bbbbbbbb', 'NR22', 0x00),
    0x18: ('wwwwwwww', 'NR23', 0xFF),
    0x19: ('wbxxxwww', 'NR24', 0xBF),
    0x1A: ('bxxxxxxx', 'NR30', 0x7F),
    0x1B: ('wwwwwwww', 'NR31', 0xFF),
    0x1C: ('xbbxxxxx', 'NR32', 0x9F),
    0x1D: ('wwwwwwww', 'NR33', 0xFF),
    0x1E: ('wbxxxwww', 'NR34', 0xBF),
    0x20: ('wwwwwwww', 'NR41', 0xFF),
    0x21: ('bbbbbbbb', 'NR42', 0x00),
    0x22: ('bbbbbbbb', 'NR43', 0x00),
    0x23: ('wbxxxxxx', 'NR44', 0xBF),
    0x24: ('bbbbbbbb', 'NR50', 0x77),
    0x25: ('bbbbbbbb', 'NR51', 0xF3),
    0x26: ('bxxxrrrr', 'NR52', 0xF1),
    0x30: ('bbbbbbbb', 'WAVE1', 0xFF),
    0x31: ('bbbbbbbb', 'WAVE2', 0xFF),
    0x32: ('bbbbbbbb', 'WAVE3', 0xFF),
    0x33: ('bbbbbbbb', 'WAVE4', 0xFF),
    0x34: ('bbbbbbbb', 'WAVE5', 0xFF),
    0x35: ('bbbbbbbb', 'WAVE6', 0xFF),
    0x36: ('bbbbbbbb', 'WAVE7', 0xFF),
    0x37: ('bbbbbbbb', 'WAVE8', 0xFF),
    0x38: ('bbbbbbbb', 'WAVE9', 0xFF),
    0x39: ('bbbbbbbb', 'WAVE10', 0xFF),
    0x3A: ('bbbbbbbb', 'WAVE11', 0xFF),
    0x3B: ('bbbbbbbb', 'WAVE12', 0xFF),
    0x3C: ('bbbbbbbb', 'WAVE13', 0xFF),
    0x3D: ('bbbbbbbb', 'WAVE14', 0xFF),
    0x3E: ('bbbbbbbb', 'WAVE15', 0xFF),
    0x3F: ('bbbbbbbb', 'WAVE16', 0xFF),
    0x40: ('bbbbbbbb', 'LCDC', 0x91),
    0x41: ('xbbbbrrr', 'STAT', 0x85),
    0x42: ('bbbbbbbb', 'SCY', 0x00),
    0x43: ('bbbbbbbb', 'SCX', 0x00),
    0x44: ('rrrrrrrr', 'LY', 0x00),
    0x45: ('bbbbbbbb', 'LYC', 0x00),
    0x46: ('bbbbbbbb', 'DMA', 0xFF),
    0x47: ('bbbbbbbb', 'BGP', 0xFC),
    0x48: ('bbbbbbbb', 'OBP0', 0xFF),
    0x49: ('bbbbbbbb', 'OBP1', 0xFF),
    0x4A: ('bbbbbbbb', 'WX', 0x00),
    0x4B: ('bbbbbbbb', 'WY', 0x00)
#    0xFF: ('bbbbbbbb', 'IE', 0xE0)
}

SIZE = 128

outfile = open("registers.h", 'w')

outfile.write("""/* Header file to encode which DMG registers are read and write only, by defining bitmasks
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#ifndef REGISTERS_H
#define REGISTERS_H

""")

# for i in range(SIZE):
#     _, regname, _ = registers.get(i, ("xxxxxxxx", None, None))
#     if regname is not None:
#         outfile.write(f"#define R_{regname.upper():<6} 0x{i:0>2x}\n")

outfile.write(f"""
uint8_t write_masks[{SIZE}] = {{ // for each byte, a '1' means a bit is writable
""")

for i in range(SIZE):
    bitmask, regname, _ = registers.get(i, ("xxxxxxxx", None, None))
    bitmask = bitmask.replace('b', '1')
    bitmask = bitmask.replace('w', '1')
    bitmask = bitmask.replace('r', '0')
    bitmask = bitmask.replace('x', '0')
    outfile.write(f"    0b{bitmask}{'' if i==SIZE-1 else ','}{(' // ' + regname) if regname is not None else ''}\n")

outfile.write(f"}};\n\nuint8_t read_masks[{SIZE}] = {{ // for each byte, a '0' means a bit is readable\n")

for i in range(SIZE):
    bitmask, regname, _ = registers.get(i, ("xxxxxxxx", None, None))
    bitmask = bitmask.replace('b', '0')
    bitmask = bitmask.replace('w', '1')
    bitmask = bitmask.replace('r', '0')
    bitmask = bitmask.replace('x', '1')
    outfile.write(f"    0b{bitmask}{'' if i==SIZE-1 else ','}{(' // ' + regname) if regname is not None else ''}\n")
outfile.write("};\n#endif //REGISTERS\n")

print(f"uint8_t initial_registers[{SIZE}] = {{")
for i in range(SIZE):
    if i % 16 == 0:
        print("   ", end='')
    bitmask, _, initial =  registers.get(i, ("xxxxxxxx", None, 0xFF));
    bitmask = bitmask.replace('b', '0')
    bitmask = bitmask.replace('w', '1')
    bitmask = bitmask.replace('r', '0')
    bitmask = bitmask.replace('x', '1')
    bit = int(bitmask, base=2)
    print(f" 0x{(bit | initial):0>2x}{' ' if i == SIZE-1 else ','}", end='')
    if i % 16 == 15:
        print(f" // 0x0{i//16:x}")
print('};')


print("\n\n\ntypedef enum registers{")

for i in range(SIZE):
    _, regname, addr = registers.get(i, ("xxxxxxxx", None, None))
    if regname is not None:
        print(f"    REG_{regname.upper():<6} = 0x{i+0xFF00:0>4x},")
print(f"    REG_{'IE':<6} = 0x{65535:0>4x},")
print('}registers;')


outfile.close()