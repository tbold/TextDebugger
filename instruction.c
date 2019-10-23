#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>

#include "instruction.h"
#include "printRoutines.h"

/* Reads one byte from memory, at the specified address. Stores the
   read value into *value. Returns 1 in case of success, or 0 in case
   of failure (e.g., if the address is beyond the limit of the memory
   size). */
int memReadByte(machine_state_t *state,	uint64_t address, uint8_t *value) {
  if (address >= state->programSize)
  {
    return 0;
  }
  else
  {
    *value = state->programMap[address];
    return 1;
  }
}

/* Reads one quad-word (64-bit number) from memory in little-endian
   format, at the specified starting address. Stores the read value
   into *value. Returns 1 in case of success, or 0 in case of failure
   (e.g., if the address is beyond the limit of the memory size). */
int memReadQuadLE(machine_state_t *state, uint64_t address, uint64_t *value) {
  if ((address + 7) >= state->programSize) // address byte and 7 more bytes for one quad-word value
  {
    return 0;
  }
  else
  {
    uint64_t secondByte = state->programMap[address + 1];
    uint64_t thirdByte = state->programMap[address + 2];
    uint64_t fourthByte = state->programMap[address + 3];
    uint64_t fifthByte = state->programMap[address + 4];
    uint64_t sixthByte = state->programMap[address + 5];
    uint64_t seventhByte = state->programMap[address + 6];
    uint64_t eighthByte = state->programMap[address + 7];
    uint64_t newValue = state->programMap[address];

    newValue = secondByte << 8 | newValue;
    newValue = thirdByte << 16 | newValue;
    newValue = fourthByte << 24 | newValue;
    newValue = fifthByte << 32 | newValue;
    newValue = sixthByte << 40 | newValue;
    newValue = seventhByte << 48 | newValue;
    newValue = eighthByte << 56 | newValue;

    *value = newValue;
    return 1;
  }
}

/* Stores the specified one-byte value into memory, at the specified
   address. Returns 1 in case of success, or 0 in case of failure
   (e.g., if the address is beyond the limit of the memory size). */
int memWriteByte(machine_state_t *state,  uint64_t address, uint8_t value) {
  if (address >= state->programSize)
  {
    return 0;
  }
  else
  {
    state->programMap[address] = value;
    return 1;
  }
}

/* Stores the specified quad-word (64-bit) value into memory, at the
   specified start address, using little-endian format. Returns 1 in
   case of success, or 0 in case of failure (e.g., if the address is
   beyond the limit of the memory size). */
int memWriteQuadLE(machine_state_t *state, uint64_t address, uint64_t value) {
  if ((address + 7) >= state->programSize) // address byte and 7 more bytes for one quad-word value
  {
    return 0;
  }
  else
  {
    state->programMap[address] = (value)&0xFF; // little endian
    state->programMap[address + 1] = (value >> 8) & 0xFF;
    state->programMap[address + 2] = (value >> 16) & 0xFF;
    state->programMap[address + 3] = (value >> 24) & 0xFF;
    state->programMap[address + 4] = (value >> 32) & 0xFF;
    state->programMap[address + 5] = (value >> 40) & 0xFF;
    state->programMap[address + 6] = (value >> 48) & 0xFF;
    state->programMap[address + 7] = (value >> 56) & 0xFF;
    return 1;
  }

}

/*  return 0 if invalid instruction
    return 1 if correct instruction
    check for ifun
    check for proper register values (e.g. rB of pushq must be F, rA of pushq is not F)
    check icode and corresponding ifun */
int isValidInstruction(machine_state_t *state, y86_instruction_t *instr)
{
  // invalid ifun and out of bound registers
  if (instr->ifun > C_G || instr->ifun < C_NC)
  {
    return 0;
  }
  // I_HALT and I_NOP does not need rA, rB, can skip check
  if (instr->icode != I_HALT && instr->icode != I_NOP)
  {
    if (instr->rA > R_NONE || instr->rA < R_RAX)
    {
      return 0;
    }
    if (instr->rB > R_NONE || instr->rB < R_RAX)
    {
      return 0;
    }
  }
  if (instr->rA > R_NONE || instr->rA < R_RAX)
  {
    return 0;
  }
  if (instr->rB > R_NONE || instr->rB < R_RAX)
  {
    return 0;
  }

  // invalid icode, ifun, register cases
  switch (instr->icode)
  {
  case I_HALT:
    if (instr->ifun != 0)
    {
      return 0;
    }
    break;
  case I_NOP:
    if (instr->ifun != 0)
    {
      return 0;
    }
    break;
  case I_RRMVXX:
    if (instr->rA == R_NONE || instr->rB == R_NONE)
    {
      return 0;
    }
    break;
  case I_IRMOVQ:
    if ((instr->ifun != 0) || (instr->rA != R_NONE) || instr->rB == R_NONE)
    {
      return 0;
    }
    break;
  case I_RMMOVQ:
    if (instr->ifun != 0 || instr->rA == R_NONE || instr->rB == R_NONE)
    {
      return 0;
    }
    break;
  case I_MRMOVQ:
    if (instr->ifun != 0 || instr->rA == R_NONE || instr->rB == R_NONE)
    {
      return 0;
    }
    break;
  case I_OPQ:
    if (instr->rA == R_NONE || instr->rB == R_NONE)
    {
      return 0;
    }
    break;
  case I_JXX:
    break; //do nothing
  case I_CALL:
    if (instr->ifun != 0)
    {
      return 0;
    }
    break;
  case I_RET:
    if (instr->ifun != 0)
    {
      return 0;
    }
    break;
  case I_PUSHQ:
    if ((instr->ifun != 0) || (instr->rA == R_NONE) || (instr->rB != R_NONE))
    {
      return 0;
    }
    break;
  case I_POPQ:
    if ((instr->ifun != 0) || (instr->rA == R_NONE) || (instr->rB != R_NONE))
    {
      return 0;
    }
    break;
  case I_INVALID:
    return 0;
    break;
  case I_TOO_SHORT:
    return 0;
    break;
  }

  // valid case
  return 1;
}

/* Fetches one instruction from memory, at the address specified by
   the program counter. Does not modify the machine's state. The
   resulting instruction is stored in *instr. Returns 1 if the
   instruction is a valid non-halt instruction, or 0 (zero)
   otherwise. */
int fetchInstruction(machine_state_t *state, y86_instruction_t *instr) {
  instr->location = state->programCounter;

  uint8_t firstByte;
  uint8_t secondByte;
  int checkInstrTooShort;

  if (!memReadByte(state, state->programCounter, &firstByte))
  {
    instr->icode = I_TOO_SHORT;
    return 0;
  }

  instr->icode = firstByte >> 4;
  instr->ifun = firstByte & 0x0F;

  if(instr->icode != I_HALT && instr->icode != I_NOP){
    if (!memReadByte(state, state->programCounter + 1, &secondByte))
    {
      instr->icode = I_TOO_SHORT;
      return 0;
    }
    instr->rA = secondByte >> 4;
    instr->rB = secondByte & 0x0F;
  }

  if (!isValidInstruction(state, instr)){
    instr->icode = I_INVALID;
  }

  // valid non-halt instructions
  switch (instr->icode)
  {
  case I_HALT:
    instr->valP = state->programCounter + 1;
    return 0;
    break;
  case I_NOP:
    instr->valP = state->programCounter + 1;
    return 1;
    break;
  case I_RRMVXX:
    instr->valP = state->programCounter + 2;
    return 1;
    break;
  case I_IRMOVQ:
    checkInstrTooShort = memReadQuadLE(state, state->programCounter + 2, &instr->valC);
    if (checkInstrTooShort == 0)
    {
      instr->icode = I_TOO_SHORT;
      return 0;
    }
    instr->valP = state->programCounter + 10;
    return 1;
    break;
  case I_RMMOVQ:
    checkInstrTooShort = memReadQuadLE(state, state->programCounter + 2, &instr->valC);
    if (checkInstrTooShort == 0)
    {
      instr->icode = I_TOO_SHORT;
      return 0;
    }
    instr->valP = state->programCounter + 10;
    return 1;
    break;
  case I_MRMOVQ:
    checkInstrTooShort = memReadQuadLE(state, state->programCounter + 2, &instr->valC);
    if (checkInstrTooShort == 0)
    {
      instr->icode = I_TOO_SHORT;
      return 0;
    }
    instr->valP = state->programCounter + 10;
    return 1;
    break;
  case I_OPQ:
    instr->valP = state->programCounter + 2;
    return 1;
    break;
  case I_JXX:
    checkInstrTooShort = memReadQuadLE(state, state->programCounter + 1, &instr->valC);
    if (checkInstrTooShort == 0)
    {
      instr->icode = I_TOO_SHORT;
      return 0;
    }
    instr->valP = state->programCounter + 9;
    return 1;
    break;
  case I_CALL:
    checkInstrTooShort = memReadQuadLE(state, state->programCounter + 1, &instr->valC);
    if (checkInstrTooShort == 0)
    {
      instr->icode = I_TOO_SHORT;
      return 0;
    }
    instr->valP = state->programCounter + 9;
    return 1;
    break;
  case I_RET:
    instr->valP = state->programCounter + 1;
    return 1;
    break;
  case I_PUSHQ:
    instr->valP = state->programCounter + 2;
    return 1;
    break;
  case I_POPQ:
    instr->valP = state->programCounter + 2;
    return 1;
    break;
  case I_INVALID:
    return 0;
    break;
  case I_TOO_SHORT:
    return 0;
    break;
  default:
    instr->icode = I_INVALID;
    return 0;
    break;
  }
}

/* Executes the instruction specified by *instr, modifying the
   machine's state (memory, registers, condition codes, program
   counter) in the process. Returns 1 if the instruction was executed
   successfully, or 0 if there was an error. Typical errors include an
   invalid instruction or a memory access to an invalid address. */
int executeInstruction(machine_state_t *state, y86_instruction_t *instr) {
  switch (instr->icode)
  {
  case I_HALT:
    return 1;
    break;
  case I_NOP:
    state->programCounter = instr->valP;
    return 1;
    break;
  case I_RRMVXX:
    switch (instr->ifun)
    {
    case C_NC:
      // no condition case or simply the RRMVXX case
      state->registerFile[instr->rB] = state->registerFile[instr->rA];
      state->programCounter = instr->valP;
      break;
    case C_LE:
      if ((state->conditionCodes & CC_ZERO_MASK) != 0 ||
          (state->conditionCodes & CC_SIGN_MASK) != 0)
      {
        state->registerFile[instr->rB] = state->registerFile[instr->rA];
      }
      state->programCounter = instr->valP;
      break;
    case C_L:
      if ((state->conditionCodes & CC_SIGN_MASK) != 0)
      {
        state->registerFile[instr->rB] = state->registerFile[instr->rA];
      };
      state->programCounter = instr->valP;
      break;
    case C_E:
      if ((state->conditionCodes & CC_ZERO_MASK) != 0)
      {
        state->registerFile[instr->rB] = state->registerFile[instr->rA];
      };
      state->programCounter = instr->valP;
      break;
    case C_NE:
      if ((state->conditionCodes & CC_ZERO_MASK) == 0)
      {
        state->registerFile[instr->rB] = state->registerFile[instr->rA];
      };
      state->programCounter = instr->valP;
      break;
    case C_GE:
      if ((state->conditionCodes & CC_SIGN_MASK) == 0)
      {
        state->registerFile[instr->rB] = state->registerFile[instr->rA];
      };
      state->programCounter = instr->valP;
      break;
    case C_G:
      if (((state->conditionCodes & CC_ZERO_MASK) == 0) && ((state->conditionCodes & CC_SIGN_MASK) == 0))
      {
        state->registerFile[instr->rB] = state->registerFile[instr->rA];
      };
      state->programCounter = instr->valP;
      break;
    }
    return 1;
    break;
  case I_IRMOVQ:
    state->registerFile[instr->rB] = instr->valC;
    state->programCounter = instr->valP;
    return 1;
    break;
  case I_RMMOVQ:
    if(memWriteQuadLE(state, state->registerFile[instr->rB] + instr->valC, state->registerFile[instr->rA]) == 0){
      instr->icode = I_INVALID;
      return 0;
    }
    state->programCounter = instr->valP;
    return 1;
    break;
  case I_MRMOVQ:
    if(memReadQuadLE(state, state->registerFile[instr->rB] + instr->valC, &state->registerFile[instr->rA]) == 0) {
      instr->icode = I_INVALID;
      return 0;
    }
    state->programCounter = instr->valP;
    return 1;
    break;
  case I_OPQ:
    switch (instr->ifun)
    {
    case A_ADDQ:
      state->registerFile[instr->rB] = state->registerFile[instr->rB] + state->registerFile[instr->rA];
      break;
    case A_SUBQ:
      state->registerFile[instr->rB] = state->registerFile[instr->rB] - state->registerFile[instr->rA];
      break;
    case A_ANDQ:
      state->registerFile[instr->rB] = state->registerFile[instr->rB] & state->registerFile[instr->rA];
      break;
    case A_XORQ:
      state->registerFile[instr->rB] = state->registerFile[instr->rB] ^ state->registerFile[instr->rA];
      break;
    case A_MULQ:
      state->registerFile[instr->rB] = state->registerFile[instr->rB] * state->registerFile[instr->rA];
      break;
    case A_DIVQ:
      state->registerFile[instr->rB] = state->registerFile[instr->rB] / state->registerFile[instr->rA];
      break;
    case A_MODQ:
      state->registerFile[instr->rB] = state->registerFile[instr->rB] % state->registerFile[instr->rA];
      break;
    }

    // check condition code
    if (!((~state->registerFile[instr->rB] + 1) & 0x8000000000000000))
    { //if <=0
      if ((state->registerFile[instr->rB] & 0x8000000000000000) != 0)
      {
        state->conditionCodes = CC_SIGN_MASK; //if < 0
      }
      else
      { //if == 0
        state->conditionCodes = CC_ZERO_MASK;
      }
    }
    else if (state->registerFile[instr->rB] > 0)
    {
      state->conditionCodes = CC_SIGN_MASK & CC_ZERO_MASK;
    }
    else
    { //if !=0
      state->conditionCodes = CC_SIGN_MASK & CC_ZERO_MASK;
    }

    // update PC
    state->programCounter = instr->valP;
    return 1;
    break;
  case I_JXX:
    switch (instr->ifun)
    {
    case C_NC: //no condition
      state->programCounter = instr->valC;
      break;
    case C_LE: //<=0
      if (((state->conditionCodes & CC_ZERO_MASK) != 0) ||
          ((state->conditionCodes & CC_SIGN_MASK) != 0))
      {
        state->programCounter = instr->valC;
      }
      else
      {
        state->programCounter = instr->valP;
      }
      break;
    case C_L: //<0
      if ((state->conditionCodes & CC_SIGN_MASK) != 0)
      {
        state->programCounter = instr->valC;
      }
      else
      {
        state->programCounter = instr->valP;
      }
      break;
    case C_E: //==0
      if ((state->conditionCodes & CC_ZERO_MASK) != 0)
      {
        state->programCounter = instr->valC;
      }
      else
      {
        state->programCounter = instr->valP;
      }
      break;
    case C_NE: //!=0
      if ((state->conditionCodes & CC_ZERO_MASK) == 0)
      {
        state->programCounter = instr->valC;
      }
      else
      {
        state->programCounter = instr->valP;
      }
      break;
    case C_GE: //>=0
      if ((state->conditionCodes & CC_SIGN_MASK) == 0)
      {
        state->programCounter = instr->valC;
      }
      else
      {
        state->programCounter = instr->valP;
      }
      break;
    case C_G: //>0
      if (((state->conditionCodes & CC_ZERO_MASK) == 0) && ((state->conditionCodes & CC_SIGN_MASK) == 0))
      {
        state->programCounter = instr->valC;
      }
      else
      {
        state->programCounter = instr->valP;
      }
      break;
    }
    return 1;
    break;
  case I_CALL:
    state->registerFile[4] = state->registerFile[4] - 8;
    if(memWriteQuadLE(state, state->registerFile[4], instr->valP) == 0){
      instr->icode = I_INVALID;
      return 0;
    }
    state->programCounter = instr->valC;
    return 1;
    break;
  case I_RET:
    if(memReadQuadLE(state, state->registerFile[4], &state->programCounter) == 0){
      instr->icode = I_INVALID;
      return 0;
    }
    state->registerFile[4] = state->registerFile[4] + 8;
    return 1;
    break;
  case I_PUSHQ:
    if(memWriteQuadLE(state, state->registerFile[4] - 8, state->registerFile[instr->rA]) == 0){
      instr->icode = I_INVALID;
      return 0;
    }
    state->registerFile[4] = state->registerFile[4] - 8;
    state->programCounter = instr->valP;
    return 1;
    break;
  case I_POPQ: ;
    uint64_t poppedValue;
    if(memReadQuadLE(state, state->registerFile[4], &poppedValue) == 0)
    {
      instr->icode = I_INVALID;
      return 0;
    }
    state->registerFile[4] = state->registerFile[4] + 8;
    state->registerFile[instr->rA] = poppedValue;
    state->programCounter = instr->valP;
    return 1;
    break;
  case I_INVALID:
    return 0;
    break;
  case I_TOO_SHORT:
    return 0;
    break;
  default:
    return 0;
    break;
  }
}
