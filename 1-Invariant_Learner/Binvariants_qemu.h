#include "Binvariants_common.h"
#include "qemu/typedefs.h"
#include "qemu/osdep.h"
#include "qemu.h"
#include "disas/disas.h"
#include "tcg/tcg.h"
#include "tcg/tcg-op.h"
#include <capstone.h>

#ifdef BINV_TRACE
extern FILE* binv_trace_file;
#endif

extern int NUM_REGS;
extern bool Binv_qemu_initialized;
extern bool need_record_accd_regs;
extern uint64_t start_init_qemu;

extern int invs_to_gen_max;
#define invs_to_gen_cap 16

#define BINVARIANTS_QEMU_INIT_SNIPPET do { \
  if (!Binv_qemu_initialized) { \
    Binvariants_qemu_init(cpu); \
    Binv_qemu_initialized = true; \
  } \
} while (0)

#define BINVARIANTS_RECORD_REGS_SNIPPET1 do { \
  if (Binv_qemu_initialized) { \
    recordRegs_init(db->pc_first, tb); \
  } \
} while (0)

#define BINVARIANTS_RECORD_REGS_SNIPPET2 do { \
  if (Binv_qemu_initialized && need_record_accd_regs) { \
    recordRegs(cpu, db->pc_first, db->pc_next); \
  } \
} while (0)

#define BINVARIANTS_RECORD_REGS_SNIPPET3 do { \
  if (Binv_qemu_initialized && need_record_accd_regs) { \
    recordRegs_post(db->pc_first, tb); \
  } \
} while (0)

#define REALLOC_IF_FULL(invs_arr, invs_cnt) do { \
  if (invs_cnt == invs_to_gen_max) { \
      invs_to_gen_max *= 2; \
      uint16_t *invs_temp = realloc(invs_arr, invs_to_gen_max * sizeof(uint16_t)); \
      if (invs_temp == NULL) { \
          perror("realloc\n"); \
          free(invs_arr); \
          invs_to_gen_max /= 2; \
          continue; \
      } \
      invs_arr = invs_temp; \
      memset(invs_arr + invs_cnt, 0 , (invs_to_gen_max - invs_cnt) * sizeof(uint16_t)); \
  } \
} while (0)

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

struct binv_args {
  uint64_t *cur_reg_values;
  uint8_t *cur_acc_regs_array;
  uint8_t cur_acc_regs_cnt;
  uint16_t cur_binv_id;
};

struct binv_offsets {
  size_t regs_offset;
  size_t stride;
  size_t viol_offset;
};

enum binv_msg_type { MSG_TSL = 1, MSG_BINV = 2 };

struct __attribute__((packed)) binv_msg_hdr {
    uint8_t  type;
    uint16_t len;
};

#define REG1_MASK (0xF << 2)
#define OP_MASK   (0x7 << 6)
#define REG2_MASK (0xF << 9)

#define MAX_INVS_PER_TB 152

char *get_src_and_dest(char *p, char **out_src, char **out_dest);
void recordRegs_init(uint64_t cur_translate_TB, TranslationBlock *cur_tb);
void recordRegs(CPUState *cpu, uint64_t cur_translate_TB, uint64_t next_insn);
void recordRegs_post(uint64_t cur_translate_TB, TranslationBlock *cur_tb);

bool inCodeRange(uint64_t TBaddr);
void Binvariants_qemu_init(CPUState *cpu);
void binv_log_all_invs(void);

void binv_request_update_invs(BinvariantsTB *curtb_binvTB);

void Binvariants_genInvsChecks_newTB(BinvariantsTB *curtb_binvTB, struct binv_args *ba, struct binv_offsets *bo);
bool Binvariants_updateInvs_violTB(uint64_t *cur_reg_values, BinvariantsTB *curtb_binvTB);
void Binvariants_regenInvChecks_violTB(BinvariantsTB *curtb_binvTB, struct binv_offsets *bo);