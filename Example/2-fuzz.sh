BINVARIANTS_ROOT="$(realpath "$1")"
FUZZ_TIME=$2
TRIAL=$3

#*********************** nconvert ****************************

OUT_ROOT_DIR=$BINVARIANTS_ROOT/Example/fuzzout/
AFLFUZZ=$BINVARIANTS_ROOT/2-Fuzzer/build/AFLplusplus-4.21c/afl-fuzz

PROGRAM=nconvert
PROGRAM_PATH=$BINVARIANTS_ROOT/Example/$PROGRAM
FUZZ_OUT=$OUT_ROOT_DIR/trial$TRIAL/$PROGRAM

export SAVED_INVS_FILE_ENV=$BINVARIANTS_ROOT/Example/traces/$PROGRAM\_trace_qemu_invs

PROGRAM_CMD="$PROGRAM_PATH -overwrite -out tiff -o /tmp/tmp.tif @@"

TESTCASE_DIR=$BINVARIANTS_ROOT/Example/24h_queued/


if [ -d "$FUZZ_OUT" ]; then
    find "$FUZZ_OUT" -mindepth 1 -delete
else
    mkdir -p "$FUZZ_OUT"
fi

(timeout $FUZZ_TIME $AFLFUZZ -i $TESTCASE_DIR -o $FUZZ_OUT -Q -t 10000 -- $PROGRAM_CMD)