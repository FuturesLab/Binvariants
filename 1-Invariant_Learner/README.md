### Binvariants - Invariant Learner

This directory contains the source code for the **Invariant Learner** in `Binvariants`.

It uses `afl-showmap` to execute a set of inputs on the target binary, monitor the runtime register values, and learn register-level likely invariants.

The inferred invariants are written to a file in the directory specified by the `BIV_TRACES_DIR` environment variable.

To use `Invariant Learner`, see [../Example/1-learn_invs.sh](../Example/1-learn_invs.sh).