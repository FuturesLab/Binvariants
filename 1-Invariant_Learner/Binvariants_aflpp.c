#include "Binvariants_aflpp.h"

#ifdef BINV_TRACE
FILE* binv_trace_file;
#endif

void binv_stats() {
    int tb_cnt = 0;
    int total = 0;
    for (int i = 0; i < BinvTBSize; i++) {
        BinvariantsTB *curtb_binv_tb = &binvTB[i];
        if (curtb_binv_tb->invs_cnt && curtb_binv_tb->binv_id) {
            tb_cnt++;
            total += curtb_binv_tb->invs_cnt;
        }
    }

    fprintf(binv_trace_file, "total invs: %d TBs w/ invs: %d\n", \
        total, tb_cnt);
}

bool Binvariants_aflpp_init(const char *binary) {
    #ifdef BINV_TRACE
    char *binv_trace_file_path = getenv("BINV_AFL_TRACE_FILE_ENV");
    if (!binv_trace_file_path) {
        printf("BINV_TRACE is defined but BINV_AFL_TRACE_FILE_ENV not set!\n");
        return false;
    }

    binv_trace_file = fopen(binv_trace_file_path, "w");
    if (binv_trace_file == NULL) {
        perror("fopen failed");
        exit(1);
    }
    fclose(binv_trace_file);

    binv_trace_file = fopen(binv_trace_file_path, "a");
    #endif

    uint64_t binv_start_code = 0; // starting address of .text
    uint64_t binv_end_code = 0; // ending address of .text
    uint64_t binv_start_init = 0; // starting address of .init

    if (get_text_range_gdb(binary, &binv_start_code, &binv_end_code, &binv_start_init) != 0 ||
            (binv_start_code == 0 && binv_end_code == 0 && binv_start_init == 0)) {
        perror("Fail to get code range of the target binary\n");
        exit(1);
    }

    #ifdef BINV_TRACE
    fprintf(binv_trace_file, "binary path: %s\n", binary);
    #endif

    SET_U64_ENV(binv_start_code, BINVARIANTS_START_CODE_ENV);
    SET_U64_ENV(binv_end_code, BINVARIANTS_END_CODE_ENV);
    SET_U64_ENV(binv_start_init, BINVARIANTS_START_INIT_ENV);
    init_binvTB();
    init_binv_id_ptr();

    #ifdef BINV_TRACE
    fputs("Binvariants initialized in AFL++\n", binv_trace_file);
    fflush(binv_trace_file);
    #endif
    return true;
}

void Binvariants_aflpp_deinit() {
    deinit_binvTB();
    deinit_binv_id_ptr();

    #ifdef BINV_TRACE
    binv_stats();
    fputs("Binvariants ended in AFL++\n", binv_trace_file);
    fclose(binv_trace_file);
    #endif
}