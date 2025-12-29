## Binvariants - Invariant Learner

This directory contains the source code for the **Invariant Learner** in `Binvariants`.

It uses `afl-showmap` and `QEMU` to execute a set of inputs on the target binary, monitor the runtime register values, and learn register-level likely invariants.

The inferred invariants are written to a file in the directory specified by the `BINV_TRACES_DIR` environment variable.

To use `Invariant Learner`, see [../Example/1-learn_invs.sh](../Example/1-learn_invs.sh).


### Invariant Encoding

During the learning process, each invariant is encoded in 16 bits and stored in `invs` field of `BinvariantsTB` structure. The first bit stores the invariant type:
| Type | Meaning Bugs |
| ---- | ---- |
| 0	| Relationship|
| 1 | Value-range|

**Bit 0** → invariant type (0 or 1)

**Relationship Invariants**:
```
Bit offset:   0     1   2–5     6–8     9–12    13–15
Field:       Type   -   REG1    OP      REG2      -
```
* REG1 / REG2: register IDs (QEMU register enums)
* OP: operator code (enum for <, <=, ==, >=, >)
* Bits 1 and 13–15 are unused

**Value-Range Invariants**:
```
Bit offset:   0     1   2–5     6–15
Field:       Type   -   REG     RANGE
```
* REG: register ID
* Bits 6–15: each bit corresponds to a value range


| Bit | Value Range |
| ---- | ---- |
| 6 | `0` to `2^4 − 1`|
| 7 | `2^4` to `2^8 − 1`|
| 8 | `2^8` to `2^16 − 1`|
| 9 | `2^16` to `2^32 − 1`|
| 10 | `2^32` to `2^63 − 1`|
| 11 | `2^63` to `2^64 - 2^32 − 1`|
| 12 | `2^64 - 2^32` to `2^64 - 2^16 − 1`|
| 13 | `2^64 - 2^16` to `2^64 - 2^8 − 1`|
| 14 | `2^64 - 2^8` to `2^64 - 2^4 − 1`|
| 15 | `2^64 - 2^4` to `2^64 − 1`|
