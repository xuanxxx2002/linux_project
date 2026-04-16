#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 使用的 user↔kernel I/O 結構。
 * 要與 kernel/my_get_physical_addresses.c 內的 struct my_pa_req 完全一致。
 */
struct my_pa_req {
    void *va;          // [in]  要查詢的 user 虛擬位址
    uintptr_t pa;      // [out] 回傳的實體位址（0 代表沒映射）
};

#ifdef __cplusplus
}
#endif
