#include <include/Binv_instrument.h>

TBEntry *entries = NULL;
size_t tb_count = 0;

uint64_t min_tb_addr = UINT64_MAX;
uint64_t max_tb_addr = 0;

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

void regValRange(uint16_t *inv, uint64_t valRange[2]) {
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

void Binv_init(void) {
    char *saved_invs_path = getenv("SAVED_INVS_FILE_ENV");
    if (!saved_invs_path) {
        printf("SAVED_INVS_FILE_ENV not set");
        exit(1);
    }

    FILE *f = fopen(saved_invs_path, "r");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    entries = malloc(BINV_INITIAL_TB_CAP * sizeof *entries);
    if (!entries) {
        perror("malloc");
        exit(1);
    }
    size_t tb_cap = BINV_INITIAL_TB_CAP;
    tb_count = 0;
    TBEntry *cur = NULL;

    char line[BINV_MAX_LINE];
    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (isspace((unsigned char)*p)) p++;

        if (strncmp(p, "TBAddr:", 7) == 0) {
            if (tb_count == tb_cap) {
                size_t new_cap = tb_cap * 2;
                TBEntry *tmp = realloc(entries, new_cap * sizeof *entries);
                if (!tmp) { perror("realloc entries"); break; }
                entries = tmp;
                tb_cap = new_cap;
            }

            p += 7;
            while (isspace((unsigned char)*p)) p++;
            uint64_t addr = strtoul(p, NULL, 0);

            cur = &entries[tb_count++];
            cur->tbaddr   = addr;
            if (addr < min_tb_addr) min_tb_addr = addr;
            if (addr > max_tb_addr) max_tb_addr = addr;

            cur->n_bytes  = 0;
            cur->byte_cap = BINV_INITIAL_BYTE_CAP;
            cur->bytes    = malloc(cur->byte_cap * sizeof *cur->bytes);
            if (!cur->bytes) {
                perror("malloc bytes");
                exit(1);
            }
        } else if (cur && strncmp(p, "0x", 2) == 0) {
            char *tok = strtok(p, " \t\r\n");
            while (tok) {
                if (strncmp(tok, "0x", 2) == 0) {
                    if (cur->n_bytes == cur->byte_cap) {
                        size_t new_cap = cur->byte_cap * 2;
                        uint16_t *tmp = realloc(cur->bytes,
                                                     new_cap * sizeof *cur->bytes);
                        if (!tmp) { perror("realloc bytes"); exit(1); }
                        cur->bytes    = tmp;
                        cur->byte_cap = new_cap;
                    }
                    cur->bytes[cur->n_bytes++] = strtoul(tok, NULL, 0);
                }
                tok = strtok(NULL, " \t\r\n");
            }
        }
    }
    fclose(f);

    if (min_tb_addr == UINT64_MAX || max_tb_addr == 0) {
        printf("check invs file!!!");
        exit(1);
    }
}

void Binv_deinit(void) {
    free(entries);
}