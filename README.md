# chook
A new approach to hook lazy binding functions by using dyld info.

# Usage
## Objective-C or C
```c
#include "simple_chook.h"

static char* (*ori_getenv)(const char *);

char* my_getenv(const char *s)
{
    printf("🚀 chook bigo!\n");
    return ori_getenv(s);
}

void c_hook_func(const struct mach_header *header, intptr_t slide)
{
    c_hook(header, slide, "getenv", my_getenv, (void *)&ori_getenv);
}

void reset_c_hook_func(const struct mach_header *header, intptr_t slide)
{
    reset_c_hook(header, slide, "getenv");
}

int main(int argc, char * argv[])
{
    _dyld_register_func_for_add_image(c_hook_func);

    printf("path = %s\n", getenv("PATH"));

    _dyld_register_func_for_add_image(reset_c_hook_func);

    printf("path = %s\n", getenv("PATH"));
}

// output
// 🚀 chook bigo!
// path = path = /usr/bin:/bin:/usr/sbin:/sbin
// path = path = /usr/bin:/bin:/usr/sbin:/sbin
```

## Swift
```swift
import Foundation

//_$sSa6appendyyxnF

typealias AppendType = @convention(c) (__owned Int) -> Void
var myAppend: AppendType = { e in
    print("🚀 bigo!")
}
var replacement = unsafeBitCast(myAppend, to: UnsafeMutableRawPointer.self)
var replaced: UnsafeMutableRawPointer?

var arr = [1]

arr.append(2)

_dyld_register_func_for_add_image { (header, slide) in
    c_hook(header, slide, "$sSa6appendyyxnF", replacement, &replaced)
}

arr.append(3)

// output
// 🚀 bigo!
```

# How it works
## hook lazy binding function
![chook.png](./chook.png)

## reset lazying bound function
![chook.png](./reset_chook.png)

# TODO
- [x] reset hooked C functions.
- [ ] support Swift completely.
- [ ] support hook C functions in non-lazy symbol.
- [ ] support more simple API.
- [ ] support hook multiple C functions in one line call.
