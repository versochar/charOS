#ifndef CHAROS_PROCESS_EXEC_H
#define CHAROS_PROCESS_EXEC_H

#include <stdint.h>

/* 19A: args = boşlukla ayrılmış argüman dizesi (NULL = argümansız).
 * Taze stack'in tepesine [argc][argv][NULL][stringler] kurulur; esp argc'yi gösterir. */
int exec_flat(const char* path, const char* args);

#endif
