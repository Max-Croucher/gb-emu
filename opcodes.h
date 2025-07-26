/* Header file for opcodes.c, implementing the gameboy's opcodes
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#ifndef OPCODES_H
#define OPCODES_H

void queue_instruction(void);
void load_interrupt_instructions(uint8_t isr);

#endif // OPCODES_H