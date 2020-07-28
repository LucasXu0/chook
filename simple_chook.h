//
//  simple_chook.h
//
//  Created by xurunkang on 2020/7/20.
//

#include <mach-o/dyld.h>

void c_hook(const struct mach_header* header,
              intptr_t slide,
              const char *name,
              void *replacement,
              void **replaced);
