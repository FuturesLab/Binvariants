#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef struct {
    uint64_t  tbaddr;        // the TBAddr value
    size_t         n_bytes;       // current count of bytes
    size_t         byte_cap;      // allocated capacity of bytes[]
    uint16_t *bytes;         // dynamically‐allocated byte array
} TBEntry;

#define BINV_INITIAL_TB_CAP   16    // initial number of TBEntry slots
#define BINV_INITIAL_BYTE_CAP 16    // initial bytes[] capacity per TB
#define BINV_MAX_LINE        1024

struct binv_offsets {

  size_t regs_offset;
  size_t stride;

};

enum EvalReturn {
  EQUAL = 0,
  LESS = 1,
  LESSEQUAL = 2,
  GREATER = 3,
  GREATEREQUAL = 4,
  NOTEQUAL = 5,
  // 6 - 15
  PASS = 16,
  INVALIDOP = 17
};


extern uint64_t min_tb_addr;
extern uint64_t max_tb_addr;

extern TBEntry *entries;
extern size_t tb_count;

void regValRange(uint16_t *inv, uint64_t valRange[2]);
void Binv_init(void);
void Binv_deinit(void);
void Binv_add_invChecks(size_t cur_invs_cnt, uint16_t *cur_invs, struct binv_offsets *bo, uint32_t pc_hash);