//
//  simple_chook.c
//
//  Created by xurunkang on 2020/7/20.
//

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <mach-o/loader.h>
#include <mach-o/dyld.h>

#include "simple_chook.h"

static inline uintptr_t _read_uleb128(uint8_t **p)
{
    uint64_t result = 0;
    int         bit = 0;
    do {
        uint64_t slice = **p & 0x7f;

        if (bit > 63)
            printf("uleb128 too big for uint64, bit=%d, result=0x%0llX", bit, result);
        else {
            result |= (slice << bit);
            bit += 7;
        }
    } while ((**p & 0x80) && (*p)++);
    (*p)++;
    return (uintptr_t)result;
}

void c_hook(const struct mach_header* header, intptr_t slide, const char *name, void *replacement, void **replaced)
{
    // 1. __LINKEDIT Segment        -> VM Address & File Offset
    // 2. LC_DYLD_INFO_ONLY         -> Lazy Binding Info & Lazy Binding Offset
    struct segment_command_64 *linkedit = NULL;
    struct dyld_info_command *dyld_info = NULL;

    struct segment_command_64 *cur_seg  = NULL;
    uint32_t cmd                        = 0;

    uintptr_t cur = (uintptr_t)header + sizeof(struct mach_header_64);
    for (uint32_t i = 0; i < header->ncmds; i++) {
        cur_seg = (struct segment_command_64 *)cur;
        cmd = cur_seg->cmd;
        cur += cur_seg->cmdsize;

        if (strcmp(cur_seg->segname, SEG_LINKEDIT) == 0) linkedit = cur_seg;
        if (cmd == LC_DYLD_INFO_ONLY) dyld_info = (struct dyld_info_command *)cur_seg;

        if (linkedit && dyld_info) break;
    }

    if (!linkedit || !dyld_info) return;

    uintptr_t linkedit_base     = slide + linkedit->vmaddr - linkedit->fileoff;
    uint8_t *lazy_rebind_info   = (uint8_t *)(dyld_info->lazy_bind_off + linkedit_base);
    uint8_t *begin              = lazy_rebind_info;
    uint8_t *end                = begin + dyld_info->lazy_bind_size;

    bool done           = false;
    uintptr_t offset    = -1;
    uint8_t seg_index   = -1;

    while (begin < end) {
        uint8_t opcode = *begin & BIND_OPCODE_MASK;
        uint8_t immediate = *begin & BIND_IMMEDIATE_MASK;
        ++begin;
        switch (opcode) {
            case BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
                seg_index = immediate;
                offset = _read_uleb128(&begin);
                break;
            case BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM:
                done = strcmp(name, (char *)begin + 1) == 0;
                begin += strlen((char *)begin);
                break;
            case BIND_OPCODE_SET_DYLIB_ORDINAL_IMM:
            case BIND_OPCODE_DO_BIND:
            case BIND_OPCODE_DONE:
            default:
                break;
        }

        if (done) break;
    }

    if (done && offset && seg_index) {

        struct segment_command_64 *data = NULL;
        cur = (uintptr_t)header + sizeof(struct mach_header_64);
        for (uint32_t i = 0; i < header->ncmds && i <= seg_index; i++) {
            data = (struct segment_command_64 *)cur;
            cmd = data->cmd;
            cur += data->cmdsize;
        }

        if (!data) return;

        void **location = (void **)(slide + data->vmaddr + offset);
        if (replaced != NULL && *replaced == 0x0) {
            *replaced = *location;
        }
        *location = replacement;
    }
}
