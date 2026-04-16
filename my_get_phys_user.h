#pragma once
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>
#include "my_get_phys_io.h"

// 與 arch/x86/entry/syscalls/syscall_64.tbl 的號碼一致
#ifndef SYS_my_get_physical_addresses
  #define SYS_my_get_physical_addresses 449
#endif

/* 使用者側的便捷包裝：把 va 填進 req，呼叫 syscall，回來後看 req.pa */
static inline void* my_get_physical_addresses(void *addr) {
    struct my_pa_req req;
    req.va = addr;
    req.pa = 0;

    long rc = syscall(SYS_my_get_physical_addresses, &req);
    if (rc < 0) {
        return (void*)0;  // 失敗就當作沒映射
    }
    return (req.pa == 0) ? (void*)0 : (void*)(uintptr_t)req.pa;
}
