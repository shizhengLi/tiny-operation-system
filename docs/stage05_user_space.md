# 阶段5: 用户空间与进程隔离技术文档

## 概述

阶段5成功实现了用户空间与进程隔离功能，为微型操作系统提供了完整的内存保护、进程隔离和用户空间支持。本阶段标志着操作系统从单一内核空间发展到支持多进程、内存保护的现代操作系统架构。

## 核心功能实现

### 1. 内存分页机制

**文件**: `src/kernel_usermode.c`

实现了完整的x86内存分页机制：

**分页结构**:
```c
struct page_table_entry {
    uint32_t present : 1;
    uint32_t writable : 1;
    uint32_t user : 1;
    uint32_t writethrough : 1;
    uint32_t cachedisable : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t zero : 1;
    uint32_t global : 1;
    uint32_t available : 3;
    uint32_t frame : 20;
} __attribute__((packed));

struct page_directory_entry {
    uint32_t present : 1;
    uint32_t writable : 1;
    uint32_t user : 1;
    uint32_t writethrough : 1;
    uint32_t cachedisable : 1;
    uint32_t accessed : 1;
    uint32_t zero : 1;
    uint32_t size : 1;
    uint32_t ignored : 1;
    uint32_t available : 3;
    uint32_t frame : 20;
} __attribute__((packed));
```

**分页功能**:
- 4KB页面大小
- 1024个页目录项和页表项
- 内核空间映射到高地址 (0xC0000000)
- 用户空间映射到低地址 (0x08048000)
- 页面级权限控制
- 按需分页支持

**核心函数**:
- `paging_init()`: 初始化分页系统
- `paging_enable()`: 启用分页
- `paging_alloc_frame()`: 分配物理帧
- `paging_free_frame()`: 释放物理帧
- `paging_map_page()`: 映射虚拟页面到物理帧
- `paging_switch_directory()`: 切换页目录

### 2. 用户空间与内核空间分离

**文件**: `src/kernel_usermode.c`, `src/usermode_syscall_handlers.c`

实现了完整的用户空间与内核空间分离：

**地址空间布局**:
- 用户空间: 0x00000000 - 0xBFFFFFFF
- 内核空间: 0xC0000000 - 0xFFFFFFFF
- 用户程序基地址: 0x08048000
- 内核基地址: 0xC0000000

**内存保护**:
- 用户空间代码不能直接访问内核空间
- 内核空间可以访问所有内存
- 页面级权限控制
- 段级保护 (用户段 vs 内核段)

**用户空间验证**:
```c
static int validate_user_pointer(const void* ptr, uint32_t size) {
    uint32_t addr = (uint32_t)ptr;
    
    /* 检查是否在用户空间 */
    if (addr >= 0xC0000000) {
        return 0;  /* 内核空间 */
    }
    
    /* 检查溢出 */
    if (addr + size < addr) {
        return 0;  /* 溢出 */
    }
    
    return 1;
}
```

### 3. 进程隔离与保护

**文件**: `src/kernel_usermode.c`

实现了完整的进程隔离机制：

**进程控制块增强**:
```c
struct process {
    uint32_t pid;
    uint32_t parent_pid;
    enum process_state state;
    uint32_t esp;
    uint32_t eip;
    uint32_t cr3;                    /* 页目录物理地址 */
    uint32_t kernel_stack;
    uint32_t user_stack;
    uint32_t exit_code;
    char name[32];
    uint32_t page_directory;         /* 页目录物理地址 */
    uint32_t brk;                    /* 程序断点 */
};
```

**进程隔离功能**:
- 每个进程独立的页目录
- 进程间内存隔离
- 用户空间代码执行限制
- 内核栈与用户栈分离
- 进程状态管理

**进程管理功能**:
- 进程创建和销毁
- 进程切换
- 进程调度
- 资源清理

### 4. 系统调用入口优化

**文件**: `src/usermode_syscall.asm`, `src/usermode_syscall_handlers.c`

实现了优化的系统调用处理机制：

**系统调用处理流程**:
1. 用户态触发 INT 0x80
2. 保存用户态寄存器状态
3. 切换到内核栈
4. 验证用户空间参数
5. 调用内核系统调用处理函数
6. 恢复用户态寄存器状态
7. 返回用户态

**参数验证**:
```c
static int copy_from_user(void* kernel_dest, const void* user_src, uint32_t size) {
    if (!validate_user_pointer(user_src, size)) {
        return -1;  /* 无效用户指针 */
    }
    
    /* 逐字节复制数据 */
    uint8_t* dest = (uint8_t*)kernel_dest;
    const uint8_t* src = (const uint8_t*)user_src;
    
    for (uint32_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
    
    return 0;
}
```

**增强的系统调用**:
- SYSCALL_EXIT (0): 进程退出
- SYSCALL_FORK (8): 创建进程
- SYSCALL_EXEC (9): 执行程序
- SYSCALL_BRK (15): 改变程序断点
- 用户空间参数验证
- 错误处理机制

### 5. 任务状态段 (TSS)

**文件**: `src/kernel_usermode.c`

实现了TSS支持：

**TSS结构**:
```c
struct tss {
    uint32_t prev_tss;
    uint32_t esp0;        /* 内核栈指针 */
    uint32_t ss0;         /* 内核栈段 */
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));
```

**TSS功能**:
- 用户态到内核态的栈切换
- 特权级切换支持
- I/O权限位图

### 6. 页面错误处理

**文件**: `src/page_fault_handler.c`, `src/usermode_syscall.asm`

实现了完整的页面错误处理机制：

**页面错误处理流程**:
1. 捕获页面错误异常 (INT 14)
2. 获取错误代码和故障地址
3. 分析错误类型
4. 处理用户空间页面错误
5. 处理内核空间页面错误
6. 终止违规进程或停止系统

**错误代码分析**:
```c
#define PF_PRESENT    0x01  /* 页面存在 */
#define PF_WRITE      0x02  /* 写操作 */
#define PF_USER       0x04  /* 用户模式 */
#define PF_RESERVED   0x08  /* 保留位违规 */
#define PF_INSTRUCTION 0x10 /* 指令获取 */
```

### 7. 用户程序加载器

**文件**: `src/user_program.asm`

实现了简单的用户程序支持：

**用户程序示例**:
```assembly
section .text
global _start

_start:
    ; 写入系统调用
    mov eax, 2           ; SYSCALL_WRITE
    mov ebx, 1           ; stdout
    mov ecx, message     ; 消息地址
    mov edx, 18          ; 消息长度
    int 0x80
    
    ; 获取PID系统调用
    mov eax, 12          ; SYSCALL_GETPID
    int 0x80
    
    ; 退出系统调用
    mov eax, 0           ; SYSCALL_EXIT
    mov ebx, 0           ; 退出代码
    int 0x80

section .data
message db "Hello, User Space!", 10
```

## 技术特点

### 1. 完整的内存保护
- 硬件级内存分页
- 用户空间与内核空间分离
- 页面级权限控制
- 进程间内存隔离

### 2. 现代操作系统架构
- 特权级分离 (用户态/内核态)
- 系统调用接口
- 进程管理
- 内存管理

### 3. 安全性增强
- 用户空间参数验证
- 页面错误处理
- 进程隔离
- 资源保护

### 4. 可扩展性
- 模块化设计
- 清晰的接口定义
- 支持多种系统调用
- 可扩展的进程管理

## 构建系统

### 1. 构建目标
- `all`: 构建用户空间内核 (默认)
- `kernel-user`: 构建用户空间内核
- `floppy`: 创建启动盘镜像
- `run-user`: 在QEMU中运行用户空间内核

### 2. 编译选项
- 32位保护模式编译
- 支持分页和内存保护
- 严格的编译警告
- 优化级别 O2

## 测试和验证

### 1. 启动测试
成功在QEMU模拟器中启动：
- 引导程序正常工作
- 内核正确加载
- 保护模式成功进入
- 分页机制启用

### 2. 功能验证
验证了以下功能：
- 内存分页机制
- 用户空间与内核空间分离
- 进程隔离
- 系统调用处理
- 页面错误处理

### 3. 用户程序测试
创建了简单的用户程序：
- 用户空间系统调用测试
- 字符串输出测试
- 进程ID获取测试
- 进程退出测试

## 已知限制

### 1. 功能限制
- 无真正的文件系统
- 无网络支持
- 无设备驱动
- 有限的进程数量

### 2. 性能限制
- 简单的调度算法
- 无内存预取
- 无缓存优化
- 同步系统调用

### 3. 安全性限制
- 无地址空间布局随机化
- 无堆栈保护
- 无执行保护
- 有限的审计功能

## 文件结构

```
src/
├── kernel_usermode.c            # 主内核文件（用户空间版本）
├── usermode_syscall.asm         # 用户空间系统调用汇编处理程序
├── usermode_syscall_handlers.c  # 用户空间系统调用C处理程序
├── page_fault_handler.c         # 页面错误处理程序
├── interrupt_handlers.c         # 中断处理程序
├── isr.asm                      # 中断服务例程
└── user_program.asm             # 示例用户程序
```

## 性能指标

### 1. 内存使用
- 页面大小: 4KB
- 总内存: 64MB
- 页表数量: 1024个页目录项
- 物理帧管理: 位图方式

### 2. 进程管理
- 最大进程数: 16个
- 进程切换时间: 微秒级
- 调度算法: 轮转调度
- 栈大小: 内核栈16KB，用户栈8KB

## 未来改进方向

### 1. 功能扩展
- 实现真正的文件系统
- 添加网络支持
- 实现更多设备驱动
- 支持更多系统调用

### 2. 性能优化
- 实现更智能的调度算法
- 添加内存预取
- 实现缓存机制
- 优化系统调用性能

### 3. 安全性增强
- 实现地址空间布局随机化
- 添加堆栈保护
- 实现执行保护
- 增强审计功能

### 4. 调试支持
- 添加调试接口
- 实现进程跟踪
- 添加性能监控
- 改进错误报告

## 总结

阶段5成功实现了完整的用户空间与进程隔离功能，为微型操作系统提供了：

1. **完整的内存保护机制** - 通过x86分页机制实现硬件级内存保护
2. **用户空间与内核空间分离** - 现代操作系统的核心架构
3. **进程隔离** - 每个进程拥有独立的地址空间
4. **安全的系统调用接口** - 带有参数验证和错误处理
5. **页面错误处理** - 完整的内存异常处理机制
6. **用户程序支持** - 可以加载和执行用户空间程序

这些功能构成了一个现代操作系统的基础架构，标志着操作系统从简单的单一内核发展到支持多进程、内存保护的完整系统。系统具备了处理用户程序、管理资源、提供安全保障的基本能力，为进一步的功能扩展和优化提供了坚实的基础。

通过QEMU模拟器的测试验证，用户空间功能运行正常，为后续阶段的开发提供了可靠的验证平台。阶段5的实现标志着微型操作系统已经具备了现代操作系统的核心特征。