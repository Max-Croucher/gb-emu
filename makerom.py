from sys import argv

HEADER = [0x00]*256 + [
    0x00, #NOP
    0b11000011, # JP
    0x00,
    0x15,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, # Boot Logo
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    ord("N"),ord("O"),ord("P"),ord("R"),ord("O"),ord("M"),0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, # Title
    0x00, # CGB
    0x00,0x00, # New Licensee
    0x00, # Cart type
    0x00, # ROM size
    0x00, # RAM size
    0x01, # Destination
    0x00, # Old Licensee
    0x00, # Version
    0x00, # Header Checksum
    0x00, 0x00 # Body Checksum
]

LAUNCHER = [
    #standard jump
    0b00000000,
    0b11000011,
    0x50,
    0x01,
]

# Body starts at address 0x0150, where the program counter sits
BODY = [
    # set 0xFFFF to 4 (enable timer interrupts)
    0b00111110, #LD r8, imm8
    0x04,
    0b11100000, #LDH [imm8], A
    0xFF, #IE reg
    # set 0xFF07 to 0b00000110 (set timer clock)
    0b00111110, #LD r8, imm8
    0b00000110,
    0b11100000, #LDH [imm8], A
    0x07, #TAC reg
    0b00000001, # LD BC imm16
    0x34,
    0x12,
    0b00010001, # LD DE imm16
    0x89,
    0xab,
    0b00100001, # LD HL imm16
    0xfe,
    0xef,
    0b00111110, #LD r8, imm8
    0x00,
    #Stack thing
    0b11000101, #push BC
    0b11010101, #push DE
    0b11100101, #push HL
    0b00000001, #Load to BC
    0b00000000, #0x0000
    0b00000000,
    0b00010001, #Load to DE
    0b00000000, #0x0000
    0b00000000,
    0b00100001, #Load to HL
    0b00000000, #0x0000
    0b00000000,
    0b11111011, #EI
    0b11000001, #Pop BC
    0b11010001, #Pop DE
    0b11100001, #Pop HL
    0b11110011, #DI
    0b00000000, #NOP
    0b11111011, #EI
    0b00000000, #NOP
    #Set TMA to 0xf0,
    0b11110101, #PUSH AF
    0b00111110, #LD r8, imm8
    0xF0,
    0b11100000, #LD [imm8], A
    0x06, #TMC reg
    0b11110001 #POP AF
]

def main():

    if len(argv) != 2:
        print(f"Usage: {argv[0]} <filename>")

    chk = 0
    for i in range(0x0134, 0x014c+1):
        chk -= HEADER[i] + 1
    HEADER[0x014D] = chk%256
    print("Checksum set to", hex(chk%256))
    cart_size = 2**(HEADER[0x0148]+15)

    if len(LAUNCHER) > 4:
        raise ValueError("Launcher can be at most 4 bytes")
    for i in range(len(LAUNCHER)):
        HEADER[0x0100+i] = LAUNCHER[i]

    cart_padding = cart_size - len(HEADER) - len(BODY)
    if cart_padding < 0:
        raise ValueError("cart size is too small")


    # add ISR
    #ISR entry
    starting_index = 0x0050
    data = [
        0b00111100, #INC A
        0b11110101, #PUSH AF
        0b00111110, #LD r8, imm8
        0b00000100,
        0b11100000, #LD [imm8], A
        0xFF, #TMC reg
        0b11110001, #POP AF
        0b11011001 #RETI
    ]
    for i in range(len(data)):
        HEADER[starting_index+i] = data[i]

    with open(argv[1], 'wb') as outfile:
        outfile.write(bytes(HEADER))
        outfile.write(bytes(BODY))
        outfile.write(bytes([0x00]*cart_padding))

    print(f"Written to {argv[1]}")

if __name__ == "__main__":
    main()