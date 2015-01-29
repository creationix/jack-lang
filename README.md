# jack-lang

This project is a prototype VM for implementing the Jack language in simple,
portable C.  Compiling the project is as simple as `gcc *.c` (or tcc or clang,
etc...).  This is just the language runtime.  It's a fairly typical scripting
language runtime including a bytecode interpreter and a stack based memory
model.  The C API is heavily inspired by lua's C API.  Outside code is never
given direct pointer access to any of the VM objects.  The garbage collector is
a simple reference counting system.  This means it can't detect and free cycles
(the script author has to manually break cycles), but it is extremely simple and
reliable.  There are no unexpected GC pauses since everything is freed exactly
when the last reference is lost.  Functions in the runtime are extremely
flexible and make even the bytecode interpreter a separate module entirely that
could be replaced without touching the core engine.

## Data Types

Currently there are seven data types, they are:

 - Boolean: `true` or `false`
 - Integer: Signed pointer sized integer
 - Buffer: Mutable fixed-length byte-array
 - Symbol: Immutable interned byte-array
 - List: Doubly linked list of arbitrary values
 - Map: Hash map with unique and arbitrary keys associated to arbitrary values
 - Function: C function pointer with internal state and upvalues.
             User functions combine bytecode in state with interpreter pointer.
