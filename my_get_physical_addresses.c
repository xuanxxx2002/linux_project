#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/pgtable.h>
#include <linux/sched.h>

/* 與 user 端相同的 I/O 結構 */
struct my_pa_req {
    void __user *va;      // [in]  使用者要查的 VA
    unsigned long pa;     // [out] 實體實位址（0=未映射）
};

/* 只處理一般頁（不特判巨大頁），避免特定核心版本的 API 差異 */
static inline unsigned long vaddr_to_phys(struct mm_struct *mm, unsigned long vaddr)
{
    pgd_t *pgd;
#if defined(CONFIG_PGTABLE_LEVELS) && CONFIG_PGTABLE_LEVELS >= 5
    p4d_t *p4d;
#endif
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    spinlock_t *ptl;
    phys_addr_t phys = 0;

    pgd = pgd_offset(mm, vaddr);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
        return 0;

#if defined(CONFIG_PGTABLE_LEVELS) && CONFIG_PGTABLE_LEVELS >= 5
    p4d = p4d_offset(pgd, vaddr);
    if (p4d_none(*p4d) || p4d_bad(*p4d))
        return 0;
    pud = pud_offset(p4d, vaddr);
#else
    pud = pud_offset(pgd, vaddr);
#endif

    if (pud_none(*pud) || pud_bad(*pud))
        return 0;

    pmd = pmd_offset(pud, vaddr);
    if (pmd_none(*pmd) || pmd_bad(*pmd))
        return 0;

    pte = pte_offset_map_lock(mm, pmd, vaddr, &ptl);
    if (!pte)
        return 0;

    if (pte_present(*pte)) {
        unsigned long pfn = pte_pfn(*pte);
        phys = ((phys_addr_t)pfn << PAGE_SHIFT) + (vaddr & ~PAGE_MASK);
    }
    pte_unmap_unlock(pte, ptl);

    return phys;
}

SYSCALL_DEFINE1(my_get_physical_addresses, struct my_pa_req __user *, ureq)
{
    struct my_pa_req kreq;
    unsigned long vaddr;
    unsigned long pa = 0;
    struct mm_struct *mm = current->mm;

    if (!ureq)
        return -EINVAL;

    if (copy_from_user(&kreq, ureq, sizeof(kreq)))
        return -EFAULT;

    vaddr = (unsigned long)kreq.va;

    if (mm && access_ok(kreq.va, 1)) {
        down_read(&mm->mmap_lock);
        pa = vaddr_to_phys(mm, vaddr);
        up_read(&mm->mmap_lock);
    } else {
        pa = 0;
    }

    kreq.pa = pa;

    if (copy_to_user(ureq, &kreq, sizeof(kreq)))
        return -EFAULT;

    return 0;  // 成功；結果在 user 的 req.pa
}
