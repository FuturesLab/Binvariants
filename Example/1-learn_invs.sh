BINVARIANTS_ROOT="$(realpath "$1")"

#*********************** nconvert ****************************
PROGRAM=nconvert

BINVSHOWMAP=$BINVARIANTS_ROOT/1-Invariant_Learner/build/AFLplusplus-4.21c/afl-showmap

PROGRAM_PATH=$BINVARIANTS_ROOT/Example/$PROGRAM

QUEUE_DIR=$BINVARIANTS_ROOT/Example/24h_queued/

PROGRAM_CMD="$PROGRAM_PATH -overwrite -out tiff -o /tmp/tmp.tif @@"

BINV_TRACES_DIR=$BINVARIANTS_ROOT/Example/traces/
export BINV_AFL_TRACE_FILE_ENV=$BINV_TRACES_DIR/$PROGRAM\_trace_afl
export BINV_QEMU_TRACE_FILE_ENV=$BINV_TRACES_DIR/$PROGRAM\_trace_qemu

(mkdir -p $BINV_TRACES_DIR/tmp/)
($BINVSHOWMAP -i $QUEUE_DIR -o $BINV_TRACES_DIR/tmp/$PROGRAM -C -Q -t 10000 -- $PROGRAM_CMD)