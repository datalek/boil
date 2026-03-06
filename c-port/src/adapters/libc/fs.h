/*
 * fs.h - libc filesystem adapter
 */

#ifndef BOIL_ADAPTERS_LIBC_FS_H
#define BOIL_ADAPTERS_LIBC_FS_H

#include "../../domain/template.h"

/*
 * Create filesystem adapter using libc.
 */
Fs libc_fs_make(void);

#endif /* BOIL_ADAPTERS_LIBC_FS_H */
