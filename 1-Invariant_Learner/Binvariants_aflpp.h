#include "Binvariants_common.h"

#ifdef BINV_TRACE
extern FILE* binv_trace_file;
#endif

bool Binvariants_aflpp_init(const char *binary);
void Binvariants_aflpp_deinit(void);
