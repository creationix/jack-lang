#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum {
  Error,    // Contagious type that causes all operations to return Error
  Boolean,  // True or False
  Integer,  // Signed integer
  Function, // C API Function
  Symbol,   // Immutable interned data
  List,     // Linked-list of Values
  Map,      // Hash-map of values (weak key for boxed types)
  Code,     // Bytecode
} jack_type_t;


struct jack_value {
  union {
    bool boolean;
    int integer;
    const char* error;
    // TODO, add more types
  };
  jack_type_t type : 3;
};

typedef struct jack_value jack_value_t;

typedef enum {

  END, // Stop execution

  // Comparison ops
  // --------------
  // These skip the next instruction if false
  // The next instruction is generally a jump
  //
  // OP   | A   | D   | Description
  //------+-----+-----+--------------
  ISLT,  // var | var | Jump if A < D
  ISGE,  // var | var | Jump if A ≥ D
  ISEQV, // var | var | Jump if A = D
  ISNEV, // var | var | Jump if A ≠ D
  ISEQS, // var | str | Jump if A = D
  ISNES, // var | str | Jump if A ≠ D
  ISEQN, // var | num | Jump if A = D
  ISNEN, // var | num | Jump if A ≠ D
  ISEQP, // var | pri | Jump if A = D
  ISNEP, // var | pri | Jump if A ≠ D

  // Unary Test and Copy ops
  // -----------------------
  // These skip the next instruction if false
  // The next instruction is generally a jump
  //
  // OP  | A   | D   | Description
  //-----+-----+-----+------------------------------------
  ISTC, // dst | var | Copy D to A and jump, if D is true
  ISFC, // dst | var | Copy D to A and jump, if D is false
  IST,  //     | var | Jump if D is true
  ISF,  //     | var | Jump if D is false

  // Unary ops
  // ---------
  // OP  | A   | D   | Description
  //-----+-----+-----+----------------------------
  MOV,  // dst | var | Copy D to A
  NOT,  // dst | var | Set A to boolean not of D
  UNM,  // dst | var | Set A to -D (unary minus)
  LEN,  // dst | var | Set A to length of D
  ITER, // dst | var | Set A to iterator or D

  // Binary ops
  // ------------------+-------------
  // Symbol + Symbol   | Concatenate
  // Symbol * Integer  | Repeat
  // Integer + Integer | Add
  // Integer - Integer | Subtract
  // Integer / Integer | Divide
  // Integer % Integer | Modulus
  //
  // OP   | A   | B     | C     | Description
  //------+-----+-------+-------+--------------
  ADDVN, // dst | var   | num   | A = B + C
  SUBVN, // dst | var   | num   | A = B - C
  MULVN, // dst | var   | num   | A = B * C
  DIVVN, // dst | var   | num   | A = B / C
  MODVN, // dst | var   | num   | A = B % C
  ADDNV, // dst | var   | num   | A = C + B
  SUBNV, // dst | var   | num   | A = C - B
  MULNV, // dst | var   | num   | A = C * B
  DIVNV, // dst | var   | num   | A = C / B
  MODNV, // dst | var   | num   | A = C % B
  ADDVV, // dst | var   | var   | A = B + C
  SUBVV, // dst | var   | var   | A = B - C
  MULVV, // dst | var   | var   | A = B * C
  DIVVV, // dst | var   | var   | A = B / C
  MODVV, // dst | var   | var   | A = B % C

  // Constant ops
  // ------------
  // OP     | A     | D     | Description
  //--------+-------+-------+----------------------------------
  KERR,    // dst   | sym   | Set A to error constant D
  KSYM,    // dst   | sym   | Set A to symbol constant D
  KSHORT,  // dst   | lits  | Set A to 16 bit signed integer D
  KNUM,    // dst   | num   | Set A to number constant D
  KPRI,    // dst   | pri   | Set A to primitive D

  JMP,     //       | DELTA | Jump DELTA instructions

} jack_opcode_t;

// A single bytecode instruction is 32 bit wide and has an 8 bit opcode field
// and several operand fields of 8 or 16 bit. Instructions come in one of two
// formats:
// ┏━━━┳━━━┳━━━┳━━━━┓
// ┃ B ┃ C ┃ A ┃ OP ┃
// ┣━━━┻━━━╋━━━╋━━━━┫
// ┃   D   ┃ A ┃ OP ┃
// ┗━━━━━━━┻━━━┻━━━━┛

typedef struct {
  char b : 8;
  char c : 8;
  char a : 8;
  jack_opcode_t op : 8;
} jack_opabc_t;

typedef struct {
  short d : 16;
  char a : 8;
  jack_opcode_t op : 8;
} jack_opd_t;

// #define OPABC(OP, A, B, C) (uint32_t)(jack_opabc_t){ \
//   .op = OP, \
//   .a = A, \
//   .b = B, \
//   .c = C \
// }
#define OPABC(OP, A, B, C) (OP | (int8_t)(A) << 8 | (int8_t)(B) << 24 | (int8_t)(C) << 16)
//(((A) & 0xff) << 8) | (((B) & 0xff) << 24) | (((C) & 0xff) << 16))
#define OPAD(OP, A, D)     (OP | (int8_t)(A) << 8 | (int16_t)(D) << 16)
//(((A) & 0xff) << 8) | (((D) & 0xffff) << 16))

#define OPGETOP(BC) (BC) & 0xff
#define OPGETA(BC) (int8_t)(((BC) >> 8) & 0xff)
#define OPGETB(BC) (int8_t)(((BC) >> 24) & 0xff)
#define OPGETC(BC) (int8_t)(((BC) >> 16) & 0xff)
#define OPGETD(BC) (int16_t)(((BC) >> 16) & 0xffff)

static uint32_t program[] = {
  OPAD(KSHORT, 0, 42),
  OPAD(KSHORT, 1, -100),
  OPABC(ADDVV, 2, 0, 1),
  0,
};
static int pc = 0;
static jack_value_t slots[10];

#define break_if_error(DEST, SOURCE) \
  if (SOURCE->type == Error) { \
    DEST->type = Error; \
    DEST->error = SOURCE->error; \
    break; \
  }

#define break_if_not(DEST, COND, MESSAGE) \
  if (!(COND)) { \
    DEST->type = Error; \
    DEST->error = MESSAGE; \
    break; \
  }

int main() {
  uint32_t bc;
  while ((bc = program[pc++])) {
    switch(OPGETOP(bc)) {
     case KSHORT:
      printf("KSHORT slot=%d value=%d\n", OPGETA(bc), OPGETD(bc));
      jack_value_t* value = &slots[OPGETA(bc)];
      value->type = Integer;
      value->integer = OPGETD(bc);
      break;
     case ADDVV:
      printf("ADDVV %x %x %x\n", OPGETA(bc), OPGETB(bc), OPGETC(bc));
      // A = B + C
      jack_value_t* A = &slots[OPGETA(bc)];
      jack_value_t* B = &slots[OPGETB(bc)];
      jack_value_t* C = &slots[OPGETC(bc)];
      break_if_error(A, B)
      break_if_error(A, C)
      break_if_not(A, B->type == Integer && C->type == Integer, "Not a Number")

      A->type = Integer;
      A->integer = B->integer + C->integer;
      break;
     default:
      printf("%d %08x\n", pc, bc);
      printf("%d OP=%d A=%d B=%d C=%d D=%d\n", pc, OPGETOP(bc), OPGETA(bc), OPGETB(bc), OPGETC(bc), OPGETD(bc));
      break;
    }
  }

  for (int i = 0; i < 3; i++) {
    jack_value_t* slot = &slots[i];
    switch (slot->type) {
     case Integer:
      printf("%d = %d\n", i, slot->integer);
      break;
     case Error:
      printf("%d = Error: %s\n", i, slot->error);
      break;
     default:
      printf("%d = Unknown\n", i);
    }
  }
  return 0;
}
