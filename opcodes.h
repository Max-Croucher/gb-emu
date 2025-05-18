/* Header file for opcodes.c, implementing the gameboy's opcodes
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#ifndef OPCODES_H
#define OPCODES_H

typedef struct
{
    bool halt;
    bool eiset;
    uint16_t new_pc;
    uint8_t machine_cycles;
} InstructionResult;

InstructionResult block00(uint8_t* ram, Registers* reg, uint8_t opcode);
InstructionResult block01(uint8_t* ram, Registers* reg, uint8_t opcode);
InstructionResult block10(uint8_t* ram, Registers* reg, uint8_t opcode);
InstructionResult block11(uint8_t* ram, Registers* reg, uint8_t opcode);
InstructionResult prefixCB(uint8_t* ram, Registers* reg, uint8_t opcode);
InstructionResult run_instruction(uint8_t* ram, Registers* reg);

#endif // OPCODES_H