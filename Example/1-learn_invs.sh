BINVARIANTS_ROOT=$1

#*********************** nconvert ****************************
PROGRAM=nconvert

BIVSHOWMAP=$BINVARIANTS_ROOT/1-Invariant_Learner/build/AFLplusplus-4.21c/afl-showmap

PROGRAM_PATH=$BINVARIANTS_ROOT/Example/$PROGRAM

QUEUE_DIR=$BINVARIANTS_ROOT/Example/24h_queued/

PROGRAM_CMD="$PROGRAM_PATH -out tiff -o /tmp/tmp.tif @@"

BIV_TRACES_DIR=$BINVARIANTS_ROOT/Example/traces/
export INVS_AFL_TRACE_FILE_ENV=$BIV_TRACES_DIR/$PROGRAM\_trace_afl
export INVS_QEMU_TRACE_FILE_ENV=$BIV_TRACES_DIR/$PROGRAM\_trace_qemu

(mkdir -p $BIV_TRACES_DIR/tmp/)
($BIVSHOWMAP -i $QUEUE_DIR -o $BIV_TRACES_DIR/tmp/$PROGRAM -C -Q -t 10000 -- $PROGRAM_CMD)