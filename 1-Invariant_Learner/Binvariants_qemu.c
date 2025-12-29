#include "Binvariants_qemu.h"

#ifdef BINV_TRACE
FILE* binv_trace_file = NULL;
#endif

uint64_t start_code = 0;
uint64_t end_code = 0;
uint64_t start_init_qemu = 0;
uint64_t base_offset = 0;

int invs_to_gen_max = invs_to_gen_cap;

uint8_t* regs_array = NULL;
uint8_t regs_array_count = 0;
bool need_record_accd_regs = false;
bool Binv_qemu_initialized = false;

uint64_t cur_TB_start;
uint64_t cur_insn_start;
uint64_t cur_insn_end;

static const char *REG_NAMES[] = {
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "r8", "r9", "r10", "r11",
    "r12", "r13", "r14", "r15"
};

int NUM_REGS = sizeof(REG_NAMES) / sizeof(REG_NAMES[0]);

/* for identifying registers in an instruction */
static const char *REG_NAMES_EXT[] = {
    "%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%rbp", "%rsp", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15",
    "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi", "%ebp", "%esp",
    "%ax",  "%bx",  "%cx",  "%dx",  "%si",  "%di",  "%bp",  "%sp",
    "%al",  "%bl",  "%cl",  "%dl",
    "%ah",  "%bh",  "%ch",  "%dh"
};

/* mapping each register to its ID (x86_64)*/
static const int REG_IDS[] = {
    R_EAX, R_EBX, R_ECX, R_EDX, R_ESI, R_EDI, R_EBP, R_ESP, R_R8, R_R9, R_R10, R_R11, R_R12, R_R13, R_R14, R_R15,
    R_EAX, R_EBX, R_ECX, R_EDX, R_ESI, R_EDI, R_EBP, R_ESP,
    R_EAX, R_EBX, R_ECX, R_EDX, R_ESI, R_EDI, R_EBP, R_ESP,
    R_EAX, R_EBX, R_ECX, R_EDX, 
    R_EAX, R_EBX, R_ECX, R_EDX
};

uint8_t NUM_REGS_EXT = sizeof(REG_NAMES_EXT) / sizeof(REG_NAMES_EXT[0]);

static const char *REG_ID2NAME[] = {
    "RAX",
    "RCX",
    "RDX",
    "RBX",
    "RSP",
    "RBP",
    "RSI",
    "RDI",
    "R8",
    "R9",
    "R10",
    "R11",
    "R12",
    "R13",
    "R14",
    "R15"
};

static const char *OP2NAME[] = {
    "==",
    "< ",
    "<=",
    "> ",
    ">=",
    "!="
};

inline bool inCodeRange(uint64_t TBaddr) {
    return (TBaddr >= start_code && TBaddr <= end_code);
}

char *get_src_and_dest(char *p, char **out_src, char **out_dest) {
    int depth = 0;
    char *comma = NULL;
    for (char *q = p; *q; q++) {
        if (*q == '(') {
            depth++;
        } else if (*q == ')') {
            if (depth > 0) depth--;
        } else if (*q == ',' && depth == 0) {
            comma = q;
            break;
        }
    }
    if (!comma) return NULL;

    *comma = '\0';
    *out_src  = p;
    *out_dest = comma + 1;
    while (**out_dest == ' ') (*out_dest)++;

    return p;
}

void recordRegs_init(uint64_t cur_translate_TB, TranslationBlock *cur_tb) {
    if (!inCodeRange(cur_translate_TB)) {
        need_record_accd_regs = false;
        cur_tb->binv_id = 0;
        return;
    }
    if (cur_tb->pc == binvTB[cur_tb->binv_id].pc) {
        need_record_accd_regs = false;
        return;
    }

    need_record_accd_regs = true;

    if (regs_array) {
        free(regs_array);
        regs_array = NULL;
    }

    regs_array = calloc(NUM_REGS, sizeof(uint8_t));
    if (regs_array == NULL) {
        perror("calloc regs_array failed");
        exit(0);
    }
    regs_array_count = 0;

    cur_TB_start = 0;
    cur_insn_start = 0;
    cur_insn_end = 0;
}

static inline void addToAccessed(uint8_t reg_id) {
    for (uint8_t i = 0; i < regs_array_count; i++) {
        if (regs_array[i] == reg_id) {
            return;
        }
    }
    regs_array[regs_array_count++] = reg_id;
}

void recordRegs(CPUState *cpu, uint64_t cur_translate_TB, uint64_t next_insn) {
    if (!inCodeRange(cur_translate_TB)) {
        return;
    }

    size_t insn_size = 0;

    cur_insn_end = next_insn;
    if (cur_TB_start != cur_translate_TB) {
        cur_TB_start = cur_translate_TB;
        cur_insn_start = cur_translate_TB;
    }
    insn_size = cur_insn_end - cur_insn_start;
    if (insn_size > 0) {
        cs_insn *binv_cap_insn = Binvariants_disas(cur_insn_start, insn_size);

        if (binv_cap_insn) {
            char *mn = binv_cap_insn->mnemonic;
            char *op_str = binv_cap_insn->op_str;

            if (strstr(mn, "rep")) addToAccessed(R_ECX);

            if (strstr(mn, "push") || strstr(mn, "pop")) { 
                // skip
            } else if (strstr(mn, "mov") && !strstr(mn, "cmov")) {
                if (!strchr(op_str, '$')) {
                    char *src, *dest;
                    get_src_and_dest(op_str, &src, &dest);

                    for (uint8_t r = 0; r < NUM_REGS_EXT; r++){
                        const char *reg = REG_NAMES_EXT[r];
                        if (src && strstr(src, reg)) {
                            addToAccessed(REG_IDS[r]);
                        }
                    }
                }
            } else if (strstr(mn, "lea") && !strstr(mn, "leave")) {
                if (!strstr(op_str, "rip")) {
                    char *src, *dest;
                    get_src_and_dest(op_str, &src, &dest);

                    for (uint8_t r = 0; r < NUM_REGS_EXT; r++){
                        const char *reg = REG_NAMES_EXT[r];
                        if (src && strstr(src, reg)) {
                            addToAccessed(REG_IDS[r]);
                        }
                    }
                }
            } else {
                for (uint8_t r = 0; r < NUM_REGS_EXT; r++){
                    const char *reg = REG_NAMES_EXT[r];
                    if (op_str && strstr(op_str, reg)) {
                        addToAccessed(REG_IDS[r]);
                    }
                }

                if (strstr(mn, "mul") || strstr(mn, "div") || strstr(mn, "stos")) {
                    addToAccessed(R_EAX);
                } else if (strstr(mn, "lods")) {
                    addToAccessed(R_ESI);
                }
            }
        }
    }
    cur_insn_start = next_insn;
}

void recordRegs_post(uint64_t cur_translate_TB, TranslationBlock *cur_tb) {
    if (!inCodeRange(cur_translate_TB)) {
        cur_tb->binv_id = 0;
        need_record_accd_regs = false;
        return;
    }

    if (!regs_array_count || (regs_array[0] == R_ESP && regs_array_count == 1)) {
        cur_tb->binv_id = 0;
    } else {
        cur_tb->acc_regs_array = regs_array;
        cur_tb->acc_regs_cnt = regs_array_count;

        cur_tb->binv_id = *binv_id;
        BinvariantsTB *curtb_binvTB = &binvTB[*binv_id];

        curtb_binvTB->pc = cur_translate_TB;
        curtb_binvTB->binv_id = *binv_id;
        (*binv_id)++;
    }
    
    regs_array = NULL;
    regs_array_count = 0;

    need_record_accd_regs = false;
}

static inline enum EvalReturn cmp_equal(uint64_t a, uint64_t b) { return (a == b) ? EQUAL : (a < b) ? LESSEQUAL : GREATEREQUAL; }
static inline enum EvalReturn cmp_less(uint64_t a, uint64_t b) { return (a < b) ? LESS : (a == b) ? LESSEQUAL : NOTEQUAL; }
static inline enum EvalReturn cmp_lessequal(uint64_t a, uint64_t b) { return (a <= b) ? LESSEQUAL : INVALIDOP; }
static inline enum EvalReturn cmp_greater(uint64_t a, uint64_t b) { return (a > b) ? GREATER : (a == b) ? GREATEREQUAL : NOTEQUAL; }
static inline enum EvalReturn cmp_greaterequal(uint64_t a, uint64_t b) { return (a >= b) ? GREATEREQUAL : INVALIDOP; }
static inline enum EvalReturn cmp_notequal(uint64_t a, uint64_t b) { return (a != b) ? NOTEQUAL : INVALIDOP; }

typedef enum EvalReturn (*cmp_func_t)(uint64_t, uint64_t);
static const cmp_func_t cmp_table[] = {
    cmp_equal,
    cmp_less,
    cmp_lessequal,
    cmp_greater,
    cmp_greaterequal,
    cmp_notequal
};

const uint64_t minRanges[16] = {
    0, 0, 0, 0, 0, 0,
    0,                // i = 6
    (1ULL << 4),      // i = 7
    (1ULL << 8),      // i = 8
    (1ULL << 16),     // i = 9
    (1ULL << 32),     // i = 10
    (1ULL << 63),     // i = 11
    UINT64_MAX - (1ULL << 32) + 1, // i = 12
    UINT64_MAX - (1ULL << 16) + 1, // i = 13
    UINT64_MAX - (1ULL << 8) + 1,  // i = 14
    UINT64_MAX - (1ULL << 4) + 1   // i = 15
};

const uint64_t maxRanges[16] = {
    0, 0, 0, 0, 0, 0,
    (1ULL << 4) - 1,       // i = 6
    (1ULL << 8) - 1,       // i = 7
    (1ULL << 16) - 1,      // i = 8
    (1ULL << 32) - 1,      // i = 9
    (1ULL << 63) - 1,      // i = 10
    UINT64_MAX - (1ULL << 32), // i = 11
    UINT64_MAX - (1ULL << 16), // i = 12
    UINT64_MAX - (1ULL << 8),  // i = 13
    UINT64_MAX - (1ULL << 4),  // i = 14
    UINT64_MAX               // i = 15
};

inline void set_nth_bit(uint16_t *inv, uint8_t n) {
    *inv |= (1 << n);
}

inline void clear_nth_bit(uint16_t *inv, uint8_t n) {
    __asm__("btr %1, %w0" : "+m" (*inv) : "Ir" ((uint16_t)n) : "cc");
}

inline void clear_inv(uint16_t *inv) {
    *inv = 0;
}

inline void set_reg1_bits(uint16_t *inv, int reg_id) {
    __asm__(
        "andw %w2, %w0\n\t"
        "orw  %w1, %w0"
        : "+m" (*inv)
        : "r" ((reg_id & 0xF) << 2), "r" (~REG1_MASK)
        : "cc");
}

inline void set_op_bits(uint16_t *inv, enum EvalReturn REL) {
    __asm__(
        "andw %w2, %w0\n\t"
        "orw  %w1, %w0"
        : "+m" (*inv)
        : "r" ((REL & 0x7) << 6), "r" (~OP_MASK)
        : "cc");
}

inline void set_reg2_bits(uint16_t *inv, int reg_id) {
    __asm__(
        "andw %w2, %w0\n\t"
        "orw  %w1, %w0"
        : "+m" (*inv)
        : "r" ((reg_id & 0xF) << 9), "r" (~REG2_MASK)
        : "cc");
}

inline uint8_t regValue_setBit(uint64_t regValue) {
    if (regValue <= UINT64_MAX / 2) {
        if (regValue < (1ULL << 4)) return 6; // [0, 2^4 - 1]
        if (regValue < (1ULL << 8)) return 7; // [2^4, 2^8 - 1]
        if (regValue < (1ULL << 16)) return 8; // [2^8, 2^16 - 1]
        if (regValue < (1ULL << 32)) return 9; // [2^16, 2^32 - 1]
        return 10; // [2^32, 2^63 - 1]
    } else {
        if (regValue > UINT64_MAX - (1ULL << 4)) return 15; // [2^64 - 2^4, 2^64 - 1]
        if (regValue > UINT64_MAX - (1ULL << 8)) return 14; // [2^64 - 2^8, 2^64 - 2^4 - 1]
        if (regValue > UINT64_MAX - (1ULL << 16)) return 13; // [2^64 - 2^16, 2^64 - 2^8 - 1]
        if (regValue > UINT64_MAX - (1ULL << 32)) return 12; // [2^64 - 2^32, 2^64 - 2^16 - 1]
        return 11; // [2^63, 2^64 - 2^32 - 1]
    }
}

inline void regValRange(uint16_t *inv, uint64_t valRange[2]) {
    uint16_t bits = ((*inv) >> 6) & 0x03FF;  // Extract relevant bits (6-15)

    if (!bits) {
        valRange[0] = 0;
        valRange[1] = 0;
        return;
    }

    int first_bit = __builtin_ctz(bits) + 6;  // First set bit index
    int last_bit = 31 - __builtin_clz(bits) + 6;  // Last set bit index

    valRange[0] = minRanges[first_bit];
    valRange[1] = maxRanges[last_bit];
}

static inline void Binvariants_interpretInvs(uint16_t *cur_invs, int cur_invs_cnt, FILE *out_file) {
    if (!cur_invs || cur_invs_cnt == 0) {
        return;
    }

    for (int j = 0; j < cur_invs_cnt; j++) {
        fprintf(out_file, "0x%x ", cur_invs[j]);
    }
    fputs("\n", out_file);

    for (int i = 0; i < cur_invs_cnt; i++) {
        if (cur_invs[i] == 0xFFFF) continue;

        uint16_t reg1_Id;

        __asm__(                    // reg_id = (cur_invs[i] >> 2) & 0x0F;
            "mov %1, %w0\n\t"
            "shr $2, %w0\n\t"
            "and $0x0F, %w0\n\t"
            : "=r"(reg1_Id)
            : "r"(cur_invs[i])
            : "cc"
        );

        if (!(cur_invs[i] & 1)) {
            uint16_t cur_op, reg2_Id;

            __asm__(                // op = (cur_invs[i] >> 6) & 0b111;
                "mov %1, %w0\n\t"
                "shr $6, %w0\n\t"
                "and $0x07, %w0\n\t"
                : "=r"(cur_op)
                : "r"(cur_invs[i])
                : "cc"
            );
            __asm__(                // reg2_id = (cur_invs[i] >> 9) & 0b1111;
                "mov %1, %w0\n\t"
                "shr $9, %w0\n\t"
                "and $0x0F, %w0\n\t"
                : "=r"(reg2_Id)
                : "r"(cur_invs[i])
                : "cc"
            );
            if (cur_invs[i] & (1u << 1)) {
                fprintf(out_file, "INV%d: %s %s %s **VIOL**\n", i, REG_ID2NAME[reg1_Id], OP2NAME[cur_op], REG_ID2NAME[reg2_Id]);
            } else {
                fprintf(out_file, "INV%d: %s %s %s [[NO VIOL]]\n", i, REG_ID2NAME[reg1_Id], OP2NAME[cur_op], REG_ID2NAME[reg2_Id]);
            }
        } else {
            uint64_t valRange[2] = {0};
            regValRange(&cur_invs[i], valRange);

            if (cur_invs[i] & (1u << 1)) {
                fprintf(out_file, "INV%d: %lx <= %s <= %lx **VIOL**\n", i, valRange[0], REG_ID2NAME[reg1_Id], valRange[1]);
            } else {
                fprintf(out_file, "INV%d: %lx <= %s <= %lx [[NO VIOL]]\n", i, valRange[0], REG_ID2NAME[reg1_Id], valRange[1]);
            }
        }
    }
    fputs("\n", out_file);
    fflush(out_file);
}

static inline void append_str(char **pdest, const char *src) {
    size_t old_len = (*pdest != NULL) ? strlen(*pdest) : 0;
    size_t add_len = strlen(src);
    size_t new_size = old_len + add_len + 1;

    char *new_buf = realloc(*pdest, new_size);
    if (!new_buf) {
        return;
    }
    memcpy(new_buf + old_len, src, add_len + 1); 
    *pdest = new_buf;
}

void binv_log_all_invs(void) {
    char *env_path = getenv("BINV_QEMU_TRACE_FILE_ENV");

    size_t base_len = strlen(env_path);
    char *saved_invs_file_path = malloc(base_len + 1);
    if (!saved_invs_file_path) {
        perror("malloc");
        return;
    }
    memcpy(saved_invs_file_path, env_path, base_len + 1);

    append_str(&saved_invs_file_path, "_invs");

    FILE *saved_invs_file = fopen(saved_invs_file_path, "w");
    if (saved_invs_file == NULL) {
        perror("fopen failed for saved_invs_file");
        exit(1);
    }
    fclose(saved_invs_file);

    saved_invs_file = fopen(saved_invs_file_path, "a");

    for (uint16_t i = 0; i < *binv_id; i++) {
        BinvariantsTB *curtb_binvTB = &binvTB[i];
        if (curtb_binvTB->invs && curtb_binvTB->invs_cnt && curtb_binvTB->binv_id) {
            fprintf(saved_invs_file, "TBAddr: 0x%lx\n", curtb_binvTB->pc);
            Binvariants_interpretInvs(curtb_binvTB->invs, curtb_binvTB->invs_cnt, saved_invs_file);
        }
    }
    fclose(saved_invs_file);

    fflush(binv_trace_file);
}

void Binvariants_qemu_init(CPUState *cpu) {
    #ifdef BINV_TRACE
    char *binv_trace_file_path = getenv("BINV_QEMU_TRACE_FILE_ENV");
    if (!binv_trace_file_path) {
        printf("BINV_TRACE is defined but BINV_QEMU_TRACE_FILE_ENV not set");
        exit(0);
    }

    binv_trace_file = fopen(binv_trace_file_path, "w");
    if (binv_trace_file == NULL) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }
    fclose(binv_trace_file);

    binv_trace_file = fopen(binv_trace_file_path, "a");
    fputs("Starting to init Binvariants in QEMU\n", binv_trace_file);
    #endif

    char *start_code_str = getenv(BINVARIANTS_START_CODE_ENV);
    char *end_code_str = getenv(BINVARIANTS_END_CODE_ENV);
    char *start_init_str = getenv(BINVARIANTS_START_INIT_ENV);
    if (start_code_str == NULL || end_code_str == NULL || start_init_str == NULL) {
        perror("BINVARIANTS_START_CODE_ENV / BINVARIANTS_END_CODE_ENV / BINVARIANTS_START_INIT_ENV not found\n");
        exit(1);
    }

    start_code = strtoull(start_code_str, NULL, 10);
    end_code = strtoull(end_code_str, NULL, 10);
    uint64_t start_init = strtoull(start_init_str, NULL, 10);

    if (start_code == 0 || end_code == 0 || start_init == 0 || start_init_qemu == 0) {
        perror("Failed to find code segment range");
        exit(1);
    }

    base_offset = start_init_qemu - start_init;
    start_code += base_offset;
    end_code += base_offset;

    attach_binvTB();
    attach_binv_id_ptr();
    Binvariants_initDisas(cpu);

    invs_to_gen_max = invs_to_gen_cap;

    #ifdef BINV_TRACE
    fputs("Binvariants initialized in QEMU\n", binv_trace_file);
    fflush(binv_trace_file);
    #endif
}

inline void Binvariants_genInvsChecks_newTB(BinvariantsTB *curtb_binvTB, struct binv_args *ba, struct binv_offsets *bo) {
    uint64_t *cur_reg_values = ba->cur_reg_values;
    uint8_t *curTB_regs_array = ba->cur_acc_regs_array;
    uint8_t curTB_regs_count = ba->cur_acc_regs_cnt;

    size_t regs_offset = bo->regs_offset;
    size_t stride = bo->stride;
    size_t viol_offset = bo->viol_offset;
    
    uint16_t *invs_to_gen = calloc(invs_to_gen_cap, sizeof(uint16_t));
    if (invs_to_gen == NULL) {
        perror("malloc");
        return;
    }
    int invs_to_gen_count = 0;

    // single reg
    for (uint8_t i = 0; i < curTB_regs_count; i++) {
        REALLOC_IF_FULL(invs_to_gen, invs_to_gen_count);
        uint8_t cur_regId = curTB_regs_array[i];

        if (cur_regId == R_ESP) continue; // ignore RSP's value range constraint

        uint64_t curRegVal = cur_reg_values[cur_regId];

        set_nth_bit(&invs_to_gen[invs_to_gen_count], 0); // set inv type
        set_reg1_bits(&invs_to_gen[invs_to_gen_count], cur_regId); // set reg id
        uint8_t bit_valRange = regValue_setBit(curRegVal); // set reg value range
        set_nth_bit(&invs_to_gen[invs_to_gen_count], bit_valRange);

        uint64_t valRange[2] = {0};
        regValRange(&invs_to_gen[invs_to_gen_count], valRange);

        TCGv_i64 min_v = tcg_constant_i64(valRange[0]);
        TCGv_i64 max_v = tcg_constant_i64(valRange[1]);
        
        TCGv_i64 reg_val = tcg_temp_new_i64();
        tcg_gen_ld_i64(reg_val, cpu_env, regs_offset + cur_regId * stride);

        TCGLabel *L_not_voil = gen_new_label();

        /* check min <= reg_val <= max */
        TCGv_i64 min_cond = tcg_temp_new_i64();
        tcg_gen_setcond_i64(TCG_COND_GEU, min_cond, reg_val, min_v);
        TCGv_i64 max_cond = tcg_temp_new_i64();
        tcg_gen_setcond_i64(TCG_COND_LEU, max_cond, reg_val, max_v);

        TCGv_i64 both_cond = tcg_temp_new_i64();
        tcg_gen_and_i64(both_cond, min_cond, max_cond);
        tcg_gen_brcond_i64(TCG_COND_EQ, both_cond, tcg_constant_i64(1), L_not_voil);

        tcg_gen_st_i32(tcg_constant_i32(1), cpu_env, viol_offset); // mark violation (CPUX86State->viol = 1;)
        
        gen_set_label(L_not_voil);
        tcg_temp_free_i64(reg_val);
        
        invs_to_gen_count++;
    }

    // dual regs
    for (uint8_t i = 0; i < curTB_regs_count; i++) {
        uint8_t reg1_Id = curTB_regs_array[i];
        uint64_t reg1_val = cur_reg_values[reg1_Id];
        for (int j = i + 1; j < curTB_regs_count; j++) {
            REALLOC_IF_FULL(invs_to_gen, invs_to_gen_count);

            uint8_t reg2_Id = curTB_regs_array[j];
            uint64_t reg2_val = cur_reg_values[reg2_Id];
            uint16_t op;

            set_reg1_bits(&invs_to_gen[invs_to_gen_count], reg1_Id); // set reg1 id
            set_reg2_bits(&invs_to_gen[invs_to_gen_count], reg2_Id); // set reg2 id

            if (reg1_val == reg2_val) {
                set_op_bits(&invs_to_gen[invs_to_gen_count], EQUAL);
                op = EQUAL;
            } else if (reg1_val < reg2_val) {
                set_op_bits(&invs_to_gen[invs_to_gen_count], LESS);
                op = LESS;
            } else {
                set_op_bits(&invs_to_gen[invs_to_gen_count], GREATER);
                op = GREATER;
            }

            TCGv_i64 r1_val = tcg_temp_new_i64();
            tcg_gen_ld_i64(r1_val, cpu_env, regs_offset + reg1_Id * stride);

            TCGv_i64 r2_val = tcg_temp_new_i64();
            tcg_gen_ld_i64(r2_val, cpu_env, regs_offset + reg2_Id * stride);

            TCGLabel *L_not_voil = gen_new_label();

            /* check reg1 [OP] reg2 */
            if (op == EQUAL) {
                tcg_gen_brcond_i64(TCG_COND_EQ, r1_val, r2_val, L_not_voil);
            } else if (op == LESS) {
                tcg_gen_brcond_i64(TCG_COND_LTU, r1_val, r2_val, L_not_voil);
            } else {
                tcg_gen_brcond_i64(TCG_COND_GTU, r1_val, r2_val, L_not_voil);
            }

            tcg_gen_st_i32(tcg_constant_i32(1), cpu_env, viol_offset); // mark violation (CPUX86State->viol = 1;)

            gen_set_label(L_not_voil);
            tcg_temp_free_i64(r1_val);
            tcg_temp_free_i64(r2_val);

            invs_to_gen_count++;
        }
    }

    if (invs_to_gen_count > 0) {
        size_t invs_gen_size = invs_to_gen_count * sizeof(uint16_t);
        uint16_t *realloc_tmp = realloc(invs_to_gen, invs_gen_size);
        if (realloc_tmp) {
            curtb_binvTB->invs_cnt = invs_to_gen_count;
            curtb_binvTB->invs = realloc_tmp;

            invs_to_gen_max = invs_to_gen_cap;
        }
    } else {
        free(invs_to_gen);
    }
}

inline bool Binvariants_updateInvs_violTB(uint64_t *cur_reg_values, BinvariantsTB *curtb_binvTB) {
    bool invs_updated = false;

    uint16_t *cur_invs = curtb_binvTB->invs;
    int cur_invs_cnt = curtb_binvTB->invs_cnt;

    for (int i = 0; i < cur_invs_cnt; i++) {
        if (cur_invs[i] == 0xFFFF) continue;
        uint16_t reg1_Id;

        __asm__(                    // reg_id = (cur_invs[i] >> 2) & 0x0F;
            "mov %1, %w0\n\t"
            "shr $2, %w0\n\t"
            "and $0x0F, %w0\n\t"
            : "=r"(reg1_Id)
            : "r"(cur_invs[i])
            : "cc"
        );
        uint64_t reg1_value = cur_reg_values[reg1_Id];

        if (!(cur_invs[i] & 1)) {
            uint16_t cur_op, reg2_Id, op;

            __asm__(                // op = (cur_invs[i] >> 6) & 0b111;
                "mov %1, %w0\n\t"
                "shr $6, %w0\n\t"
                "and $0x07, %w0\n\t"
                : "=r"(cur_op)
                : "r"(cur_invs[i])
                : "cc"
            );
            __asm__(                // reg2_id = (cur_invs[i] >> 9) & 0b1111;
                "mov %1, %w0\n\t"
                "shr $9, %w0\n\t"
                "and $0x0F, %w0\n\t"
                : "=r"(reg2_Id)
                : "r"(cur_invs[i])
                : "cc"
            );

            uint64_t reg2_value = cur_reg_values[reg2_Id];
            op = cmp_table[cur_op](reg1_value, reg2_value);

            if (op == INVALIDOP) {
                cur_invs[i] = 0xFFFF; // mark as invalid
                invs_updated = true;
                continue;
            }
            if (op != cur_op) {
                set_op_bits(&cur_invs[i], op);
                set_nth_bit(&cur_invs[i], 1); // mark as violated/updated
                invs_updated = true;
            }
        } else {
            uint64_t valRange[2] = {0};
            regValRange(&cur_invs[i], valRange);

            if (reg1_value < valRange[0] || reg1_value > valRange[1]) {
                uint8_t bit_valRange = regValue_setBit(reg1_value);
                set_nth_bit(&cur_invs[i], bit_valRange);
                set_nth_bit(&cur_invs[i], 1); // mark as violated/updated
                invs_updated = true;
            }
        }
    }
    return invs_updated;
}

inline void Binvariants_regenInvChecks_violTB(BinvariantsTB *curtb_binvTB, struct binv_offsets *bo) {
    uint16_t *cur_invs = curtb_binvTB->invs;
    int cur_invs_cnt = curtb_binvTB->invs_cnt;

    size_t regs_offset = bo->regs_offset;
    size_t stride = bo->stride;
    size_t viol_offset = bo->viol_offset;

    for (int i = 0; i < cur_invs_cnt; i++) {
        if (cur_invs[i] == 0xFFFF) continue;
        uint16_t reg1_Id;

        __asm__(                    // reg_id = (cur_invs[i] >> 2) & 0x0F;
            "mov %1, %w0\n\t"
            "shr $2, %w0\n\t"
            "and $0x0F, %w0\n\t"
            : "=r"(reg1_Id)
            : "r"(cur_invs[i])
            : "cc"
        );

        if (!(cur_invs[i] & 1)) { // dual regs
            uint16_t cur_op, reg2_Id;

            __asm__(                // op = (cur_invs[i] >> 6) & 0b111;
                "mov %1, %w0\n\t"
                "shr $6, %w0\n\t"
                "and $0x07, %w0\n\t"
                : "=r"(cur_op)
                : "r"(cur_invs[i])
                : "cc"
            );
            __asm__(                // reg2_id = (cur_invs[i] >> 9) & 0b1111;
                "mov %1, %w0\n\t"
                "shr $9, %w0\n\t"
                "and $0x0F, %w0\n\t"
                : "=r"(reg2_Id)
                : "r"(cur_invs[i])
                : "cc"
            );

            TCGv_i64 r1_val = tcg_temp_new_i64();
            tcg_gen_ld_i64(r1_val, cpu_env, regs_offset + reg1_Id * stride);

            TCGv_i64 r2_val = tcg_temp_new_i64();
            tcg_gen_ld_i64(r2_val, cpu_env, regs_offset + reg2_Id * stride);

            TCGLabel *L_not_voil = gen_new_label();
            if (cur_op == EQUAL) {
                tcg_gen_brcond_i64(TCG_COND_EQ, r1_val, r2_val, L_not_voil);
            } else if (cur_op == LESS) {
                tcg_gen_brcond_i64(TCG_COND_LTU, r1_val, r2_val, L_not_voil);
            } else if (cur_op == LESSEQUAL) {
                tcg_gen_brcond_i64(TCG_COND_LEU, r1_val, r2_val, L_not_voil);
            } else if (cur_op == GREATER) {
                tcg_gen_brcond_i64(TCG_COND_GTU, r1_val, r2_val, L_not_voil);
            } else if (cur_op == GREATEREQUAL) {
                tcg_gen_brcond_i64(TCG_COND_GEU, r1_val, r2_val, L_not_voil);
            } else {
                tcg_gen_brcond_i64(TCG_COND_NE, r1_val, r2_val, L_not_voil);
            }
            
            tcg_gen_st_i32(tcg_constant_i32(1), cpu_env, viol_offset); // mark violation (CPUX86State->viol = 1;)

            gen_set_label(L_not_voil);
            tcg_temp_free_i64(r1_val);
            tcg_temp_free_i64(r2_val);
        } else { // single reg
            uint64_t valRange[2] = {0};
            regValRange(&cur_invs[i], valRange);

            TCGv_i64 min_v = tcg_constant_i64(valRange[0]);
            TCGv_i64 max_v = tcg_constant_i64(valRange[1]);

            TCGv_i64 reg_val = tcg_temp_new_i64();
            tcg_gen_ld_i64(reg_val, cpu_env, regs_offset + reg1_Id * stride);

            TCGLabel *L_not_voil = gen_new_label();

            TCGv_i64 min_cond = tcg_temp_new_i64();
            tcg_gen_setcond_i64(TCG_COND_GEU, min_cond, reg_val, min_v);
            TCGv_i64 max_cond = tcg_temp_new_i64();
            tcg_gen_setcond_i64(TCG_COND_LEU, max_cond, reg_val, max_v);
            
            TCGv_i64 both_cond = tcg_temp_new_i64();
            tcg_gen_and_i64(both_cond, min_cond, max_cond);
            tcg_gen_brcond_i64(TCG_COND_EQ, both_cond, tcg_constant_i64(1), L_not_voil);

            tcg_gen_st_i32(tcg_constant_i32(1), cpu_env, viol_offset); // mark violation (CPUX86State->viol = 1;)
            
            gen_set_label(L_not_voil);
            tcg_temp_free_i64(reg_val);
        }
    }
}
