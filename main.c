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

typedef enum {

  NOP, // No operation, does nothing at all!

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
  // Buffer + Buffer   | Concatenate
  // Buffer * Integer  | Repeat
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

} jack_opcode_t;

// A single bytecode instruction is 32 bit wide and has an 8 bit opcode field
// and several operand fields of 8 or 16 bit. Instructions come in one of two
// formats:
// ┏━━━┳━━━┳━━━┳━━━━┓
// ┃ B ┃ C ┃ A ┃ OP ┃
// ┣━━━┻━━━╋━━━╋━━━━┫
// ┃   D   ┃ A ┃ OP ┃
// ┗━━━━━━━┻━━━┻━━━━┛


int main() {
  return 0;
}
