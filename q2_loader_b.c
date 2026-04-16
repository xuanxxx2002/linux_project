#include <stdio.h>
#include <stdint.h>
#include "my_get_phys_user.h"

int a[2000000]; // 約 8MB（假設 int=4）

int main(void)
{
    void *pa;

    puts("==== Check first and last element physical mapping ====");
    pa = my_get_physical_addresses(&a[0]);
    printf("a[0]:        Offset:[%p]   Physical:[%p]\n",  (void*)&a[0], pa);

    pa = my_get_physical_addresses(&a[1999999]);
    printf("a[1999999]:  Offset:[%p]   Physical:[%p]\n",  (void*)&a[1999999], pa);

    puts("---- Touch the last element and re-check ----");
    a[1999999] = 42; // 觸發Page fault，按需求配置最後一段

    pa = my_get_physical_addresses(&a[1999999]);
    printf("a[1999999]:  Offset:[%p]   Physical:[%p]\n",  (void*)&a[1999999], pa);

    return 0;
}
