### Binvariants - Fuzzer

This directory contains the source code for the **Fuzzer** in `Binvariants`.

It reads invariants from the saved file, instruments the target binary with invariant-violation checks, and sends feedback to `AFL++` when a violation occurs.

To use `Fuzzer`, see [../Example/2-fuzz.sh](../Example/2-fuzz.sh).