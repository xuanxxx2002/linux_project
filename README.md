# Linux Kernel 自訂系統呼叫：`my_get_physical_addresses`

自訂 Linux kernel syscall 實作，將 user space 的虛擬位址轉換為實體位址，並附上兩個 user space 的示範程式，分別觀察 **Copy-on-Write (CoW)** 與 **Lazy Loading（按需分頁）** 的行為。

---

## 系統環境

| 項目 | 版本 |
|------|------|
| 作業系統 | Ubuntu 22.04.5 |
| Kernel | 5.15.137 |
| 虛擬機 | VMware |

---

## 專案結構

```
.
├── my_get_physical_addresses.c   # Kernel syscall 實作
├── my_get_phys_io.h              # Kernel↔User 共用 I/O 結構
├── my_get_phys_user.h            # User space syscall 包裝函式
├── q1_cow_b.c                    # 示範 1：Copy-on-Write 觀察
└── q2_loader_b.c                 # 示範 2：Lazy Loading 觀察
```

---

## 環境建置：將自訂 Syscall 編譯進 Kernel

### 1. 下載 Kernel Source

```bash
sudo su
wget -P ~/ https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.137.tar.xz
tar -xvf linux-5.15.137.tar.xz -C /usr/src
```

### 2. 安裝編譯所需套件

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install build-essential libncurses-dev libssl-dev libelf-dev bison flex -y
sudo apt clean && sudo apt autoremove -y
```

### 3. 加入 Syscall 原始碼

```bash
cd /usr/src/linux-5.15.137
mkdir get_phsical_address
cd get_phsical_address
```

將 `my_get_physical_addresses.c` 放入此目錄，並在同目錄建立 `Makefile`：

```makefile
obj-y := my_get_physical_addresses.o
```

### 4. 將新目錄加入 Kernel 編譯路徑

編輯最上層的 `Makefile`，在 `core-y` 那行末尾補上 `get_phsical_address/`：

```makefile
core-y += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ get_phsical_address/
```

### 5. 註冊 Syscall 編號

編輯 `arch/x86/entry/syscalls/syscall_64.tbl`，在最後一行加上：

```
449    common    my_get_physical_addresses    sys_my_get_physical_addresses
```

### 6. 宣告 Syscall 原型

編輯 `include/linux/syscalls.h`，在 `#endif` 之前加入：

```c
asmlinkage long sys_my_get_physical_addresses(void);
```

### 7. 編譯並安裝 Kernel

```bash
cd /usr/src/linux-5.15.137
make menuconfig      # 儲存預設設定後離開
make -j$(nproc)
make modules_install
make install
reboot
```

---

## Syscall 運作原理

`my_get_physical_addresses` 接收一個 user space 虛擬位址，依序走訪 kernel 的多層頁表（PGD → PUD → PMD → PTE），回傳對應的實體位址。

**I/O 結構**（透過 `my_get_phys_io.h` 在 kernel 與 user space 共用）：

```c
struct my_pa_req {
    void *va;       // [in]  要查詢的虛擬位址
    uintptr_t pa;   // [out] 回傳的實體位址（0 代表未映射）
};
```

**User space 包裝函式**（`my_get_phys_user.h`）：

```c
void *pa = my_get_physical_addresses(addr);
// 若頁面尚未映射則回傳 NULL
```

此 syscall 在 x86-64 上的編號為 **#449**。

> **為什麼要用 `copy_from_user` / `copy_to_user`？**
>
> Kernel 不能直接存取 user space 指標——若指標未映射、超出範圍或指向其他 process 的記憶體，會造成 kernel crash 或安全漏洞。這兩個 helper 會驗證指標合法性並自動處理 page fault。

---

## 示範 1 — Copy-on-Write（`q1_cow_b.c`）

觀察 `fork()` 之後，父子 process 對同一個全域變數共用**相同的實體頁面**，直到子 process 寫入該變數，kernel 才會配置新的實體頁（CoW 觸發）。

**編譯與執行：**

```bash
gcc -o q1_cow_b q1_cow_b.c
./q1_cow_b
```

**預期輸出：**



**結論：** 當父子 process 共用同一份資源時，系統不會立即複製。只有當其中一方嘗試寫入時，才會對該頁面進行實際的複製——讀取時共享、寫入時複製。

---

## 示範 2 — Lazy Loading（`q2_loader_b.c`）

宣告一個大型全域陣列（`int a[2000000]`，約 8 MB），分別在**存取前後**查詢最後一個元素的實體位址，觀察按需分頁的行為。

**編譯與執行：**

```bash
gcc -o q2_loader_b q2_loader_b.c
./q2_loader_b
```

**預期輸出：**



**結論：** 程式啟動初期，只有實際被存取的頁面才會分配實體記憶體。未被存取的頁面維持未映射狀態，直到第一次存取觸發 page fault，作業系統才動態完成映射與實體記憶體配置。

---

## 授權

MIT
