### Binvariants - Example

This directory contains the example scripts and test cases for fuzzing [nconvert](https://www.xnview.com/en/nconvert/) with `Binvariants`. You can modify the scripts to fuzz other binaries.

`testcases.tar.gz` includes the initial seed used for a 24-hour AFL++ run of nconvert under QEMU mode, as well as the test cases saved during the run. The saved test cases are used both for invariant learning and for fuzzing.

#### Descriptions:
```
Example/
   │
   ├───── ./1-learn_invs.sh
   │       ├── PROGRAM: the binary name
   │       │
   │       ├── BINVSHOWMAP: the path to `afl-showmap` of Binvariants' Invariant Learner
   │       │
   │       ├── PROGRAM_PATH: the path to the target binary
   │       │
   │       ├── QUEUE_DIR: the path to the folder of invariant learning corpus
   │       │
   │       ├── PROGRAM_CMD: the execution command of the binary
   │       │
   │       ├── BINV_AFL_TRACE_FILE_ENV: the path to Binvariants' AFL++ side logging file
   │       │
   │       ├── BINV_QEMU_TRACE_FILE_ENV: the path Binvariants' QEMU side logging file; used to construct the saved invariants file
   │       │
   │       └── BINV_TRACES_DIR: the folder for saving inferred invariants    
   │       
   └───── ./2-fuzz.sh      
           ├── OUT_ROOT_DIR: the path to fuzzing output root directory
           │
           ├── AFLFUZZ: the path to `afl-fuzz` of Binvariants' Fuzzer
           │
           ├── PROGRAM: the binary name
           │
           ├── PROGRAM_PATH: the path to the target binary
           │
           ├── FUZZ_OUT: the path to current fuzzing campaign directory
           │
           ├── SAVED_INVS_FILE_ENV: the path to the saved invariants file
           │
           ├── PROGRAM_CMD: the execution command of the binary
           │
           └── TESTCASE_DIR: the path to the folder of fuzzing corpus (same as invariant learning corpus)
```