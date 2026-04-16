#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "my_get_phys_user.h"

int global_a = 123;

static void line(void){ puts("================================================================================"); }

int main(void)
{
    void *parent_pa, *child_pa;

    puts("=========================== Before fork ===========================");
    parent_pa = my_get_physical_addresses(&global_a);
    printf("pid=%d: global_a:\n", getpid());
    printf("Offset:[%p]   Physical:[%p]\n", (void*)&global_a, parent_pa);
    line();

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid > 0) {
        puts("vvvvvvvvvvvvvvvvvvvvvv After fork (parent) vvvvvvvvvvvvvvvvvvvvvv");
        parent_pa = my_get_physical_addresses(&global_a);
        printf("pid=%d: global_a:\n", getpid());
        printf("******* Offset:[%p]   Physical:[%p]\n", (void*)&global_a, parent_pa);
        line();
        wait(NULL);
    } else {
        puts("llllllllllllllllllll After fork (child) llllllllllllllllllllll");
        child_pa = my_get_physical_addresses(&global_a);
        printf("******* pid=%d: global_a:\n", getpid());
        printf("******* Offset:[%p]   Physical:[%p]\n", (void*)&global_a, child_pa);
        line();

        // 觸發 Copy-on-Write
        global_a = 789;

        puts("iiiiiiiiiiiiii Test copy-on-write in child iiiiiiiiiiiii");
        child_pa = my_get_physical_addresses(&global_a);
        printf("******* pid=%d: global_a:\n", getpid());
        printf("******* Offset:[%p]   Physical:[%p]\n", (void*)&global_a, child_pa);
        line();
        sleep(2);
    }
    return 0;
}
