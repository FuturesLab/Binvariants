#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <regex.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define BINV_TRACE

#define BINVARIANTS_START_CODE_ENV "BINVARIANTS_START_CODE"
#define BINVARIANTS_END_CODE_ENV "BINVARIANTS_END_CODE"
#define BINVARIANTS_START_INIT_ENV "BINVARIANTS_START_INIT"

#define SET_INT_ENV(value, env_name) do { \
    int id_len = snprintf(NULL, 0, "%d", value); \
    if (id_len < 0) { \
        perror("snprintf failed"); \
        exit(0); \
    } \
    char* id_str = malloc(id_len+1); \
    if (!id_str) { \
        perror("malloc failed"); \
        exit(0); \
    } \
    snprintf(id_str, id_len+1, "%d", value); \
    setenv(env_name, id_str, 1); \
    free(id_str); \
} while (0)

#define SET_U64_ENV(value, env_name) do { \
    int id_len = snprintf(NULL, 0, "%lu", value); \
    if (id_len < 0) { \
        perror("snprintf failed"); \
        exit(0); \
    } \
    char* id_str = malloc(id_len+1); \
    if (!id_str) { \
        perror("malloc failed"); \
        exit(0); \
    } \
    snprintf(id_str, id_len+1, "%lu", value); \
    setenv(env_name, id_str, 1); \
    free(id_str); \
} while (0)

int get_text_range_gdb(const char *binary, uint64_t *start, uint64_t *end, uint64_t *init);

typedef struct BinvariantsTB {
    uint64_t pc;
    uint16_t *invs;
    int invs_cnt;
    uint16_t binv_id;
} BinvariantsTB;

#define BinvTBSize (1 << 15)
#define BINVTB_SHM_ID_ENV "BINVTB_SHM_ID"
extern int binvTB_struct_shm_id;
extern BinvariantsTB *binvTB;

void init_binvTB_struct(void);
void init_binvTB(void);
void attach_binvTB(void);
void deinit_binvTB(void);

#define BINV_ID_SHM_ID_ENV "BINV_ID_SHM_ID"
extern int binv_id_shm_id;
extern uint16_t *binv_id;

void init_binv_id_shm(void);
void init_binv_id_ptr(void);
void attach_binv_id_ptr(void);
void deinit_binv_id_ptr(void);