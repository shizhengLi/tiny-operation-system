# Tiny Operating System - Phase 6 技术文档

## 概述

Phase 6 实现了高级操作系统功能，包括ELF文件加载、真实文件系统、进程间通信、系统监控和调试功能。本阶段将操作系统从基础功能扩展到完整的类Unix系统。

## 系统架构

### 核心组件

1. **ELF文件加载器** - 支持加载和执行标准ELF格式的用户程序
2. **内存文件系统** - 基于inode的文件系统实现
3. **进程间通信** - Unix风格的管道机制
4. **系统监控** - 实时系统统计和性能监控
5. **扩展系统调用** - 82个系统调用的完整接口
6. **高级内存管理** - 位图和分页机制

## 详细技术实现

### 1. ELF文件加载器

#### 1.1 ELF格式支持
```c
struct elf_header {
    uint32_t magic;         /* ELF魔数 */
    uint8_t elf_class;      /* ELF类别 */
    uint8_t data_encoding;  /* 数据编码 */
    uint8_t version;        /* 版本 */
    uint8_t abi;            /* ABI标识 */
    uint8_t abi_version;    /* ABI版本 */
    uint8_t padding[7];     /* 填充 */
    uint16_t type;          /* 文件类型 */
    uint16_t machine;       /* 机器架构 */
    uint32_t version2;       /* 文件版本 */
    uint32_t entry;         /* 入口点 */
    /* ... 其他字段 */
};
```

#### 1.2 加载流程
1. **ELF验证** - 检查文件格式有效性
2. **程序头解析** - 读取段信息
3. **内存映射** - 将段加载到正确的内存位置
4. **权限设置** - 配置内存访问权限
5. **入口点设置** - 准备程序执行环境

#### 1.3 关键函数
- `elf_validate()` - 验证ELF文件格式
- `elf_load()` - 加载ELF文件到内存
- `create_user_process()` - 创建用户进程

### 2. 内存文件系统

#### 2.1 文件系统结构
```c
struct fs_entry {
    char name[64];          /* 文件名 */
    uint32_t size;         /* 文件大小 */
    uint32_t type;         /* 文件类型 */
    uint32_t inode;        /* inode号 */
    uint8_t* data;         /* 文件数据 */
    uint32_t permissions;   /* 访问权限 */
    uint32_t owner;        /* 所有者 */
    uint32_t group;         /* 组 */
    uint32_t created;       /* 创建时间 */
    uint32_t modified;      /* 修改时间 */
};

struct inode {
    uint32_t id;           /* inode ID */
    uint32_t size;         /* 文件大小 */
    uint32_t blocks;       /* 块数量 */
    uint32_t block_ptrs[12]; /* 直接块指针 */
    uint32_t indirect_ptr;  /* 一级间接指针 */
    uint32_t double_indirect_ptr; /* 二级间接指针 */
};
```

#### 2.2 文件系统特性
- **支持目录和文件** - 层次化文件系统结构
- **权限管理** - 用户/组/其他权限控制
- **inode机制** - 高效的文件管理
- **动态内存分配** - 按需分配存储空间

#### 2.3 关键函数
- `fs_init()` - 初始化文件系统
- `fs_create_file()` - 创建新文件
- `fs_read_file()` - 读取文件内容
- `fs_write_file()` - 写入文件内容
- `fs_delete_file()` - 删除文件

### 3. 进程间通信

#### 3.1 管道实现
```c
struct pipe {
    uint8_t* buffer;       /* 管道缓冲区 */
    uint32_t size;         /* 缓冲区大小 */
    uint32_t read_pos;     /* 读位置 */
    uint32_t write_pos;    /* 写位置 */
    uint32_t readers;      /* 读者数量 */
    uint32_t writers;      /* 写者数量 */
    uint32_t used;         /* 使用标志 */
};
```

#### 3.2 管道特性
- **无名管道** - 用于父子进程间通信
- **阻塞I/O** - 读写阻塞机制
- **缓冲区管理** - 环形缓冲区实现
- **同步机制** - 原子操作保证数据一致性

#### 3.3 关键函数
- `pipe_create()` - 创建管道
- `pipe_read()` - 从管道读取数据
- `pipe_write()` - 向管道写入数据
- `pipe_close()` - 关闭管道

### 4. 系统监控和调试

#### 4.1 系统统计结构
```c
struct system_stats {
    uint32_t uptime;           /* 系统运行时间 */
    uint32_t process_count;    /* 进程数量 */
    uint32_t memory_used;      /* 已用内存 */
    uint32_t memory_total;     /* 总内存 */
    uint32_t cpu_usage;        /* CPU使用率 */
    uint32_t context_switches;  /* 上下文切换次数 */
    uint32_t system_calls;     /* 系统调用次数 */
    uint32_t page_faults;      /* 页错误次数 */
    uint32_t interrupts;       /* 中断次数 */
};
```

#### 4.2 监控功能
- **实时统计** - 动态更新系统状态
- **性能分析** - CPU、内存、I/O使用情况
- **进程监控** - 进程状态和资源使用
- **调试信息** - 详细的系统诊断信息

#### 4.3 关键函数
- `system_display_stats()` - 显示系统统计
- `system_update_stats()` - 更新统计信息
- `process_get_info()` - 获取进程信息

### 5. 扩展系统调用接口

#### 5.1 系统调用分类
```c
/* 进程管理 (0-15) */
#define SYS_EXIT        0
#define SYS_FORK        1
#define SYS_EXEC        2
#define SYS_WAIT        3
#define SYS_GETPID      4
#define SYS_KILL        5

/* 文件系统 (16-31) */
#define SYS_OPEN        16
#define SYS_READ        17
#define SYS_WRITE       18
#define SYS_CLOSE       19
#define SYS_SEEK        20
#define SYS_MKDIR       21

/* 内存管理 (32-47) */
#define SYS_MMAP        32
#define SYS_MUNMAP      33
#define SYS_BRK         34
#define SYS_SBRK        35

/* 进程间通信 (48-63) */
#define SYS_PIPE        48
#define SYS_READ_PIPE   49
#define SYS_WRITE_PIPE  50
#define SYS_CLOSE_PIPE  51

/* 系统信息 (64-81) */
#define SYS_STAT        64
#define SYS_FSTAT       65
#define SYS_GETTIME     66
#define SYS_SYSINFO     67
```

#### 5.2 系统调用实现
- **寄存器传递** - 使用eax传递系统调用号
- **参数传递** - ebx, ecx, edx, esi, edi传递参数
- **返回值** - eax寄存器返回结果
- **错误处理** - 负值表示错误

## 内存管理

### 1. 分页机制
- **4KB页大小** - 标准x86分页
- **两级页表** - 页目录和页表
- **按需分页** - 动态分配物理页
- **写时复制** - 优化fork操作

### 2. 内存分配
```c
struct memory_bitmap {
    uint32_t* bits;        /* 位图数组 */
    uint32_t size;         /* 位图大小 */
    uint32_t used_blocks;  /* 已用块数 */
    uint32_t total_blocks; /* 总块数 */
};
```

### 3. 内存保护
- **用户/内核空间分离** - 保护内核数据
- **只读内存** - 保护代码段
- **执行禁止** - 防止数据执行

## 进程管理

### 1. 进程控制块
```c
struct process {
    uint32_t pid;           /* 进程ID */
    uint32_t ppid;          /* 父进程ID */
    uint32_t state;         /* 进程状态 */
    uint32_t* page_directory; /* 页目录 */
    uint32_t esp;           /* 栈指针 */
    uint32_t ebp;           /* 基址指针 */
    uint32_t eip;           /* 指令指针 */
    uint32_t priority;      /* 优先级 */
    uint32_t time_slice;    /* 时间片 */
    uint32_t session_id;    /* 会话ID */
    uint32_t group_id;      /* 进程组ID */
    uint32_t working_dir;   /* 工作目录 */
    uint32_t* file_table;   /* 文件描述符表 */
    uint32_t signal_mask;   /* 信号掩码 */
    uint32_t exit_code;     /* 退出码 */
    char name[32];          /* 进程名 */
};
```

### 2. 调度算法
- **时间片轮转** - 公平的CPU分配
- **优先级调度** - 支持进程优先级
- **多级反馈队列** - 自适应调度

## 性能优化

### 1. 编译器优化
- **O2优化级别** - 平衡性能和代码大小
- **内联函数** - 减少函数调用开销
- **常量折叠** - 编译时计算

### 2. 算法优化
- **位图操作** - 高效的内存管理
- **缓冲区复用** - 减少内存分配
- **缓存友好** - 优化数据访问模式

### 3. 系统优化
- **零拷贝** - 减少数据复制
- **延迟分配** - 按需分配资源
- **批量处理** - 提高I/O效率

## 测试和验证

### 1. 单元测试
- **ELF加载测试** - 验证程序加载功能
- **文件系统测试** - 验证文件操作
- **管道测试** - 验证IPC功能
- **系统调用测试** - 验证接口完整性

### 2. 集成测试
- **多进程测试** - 验证进程管理
- **内存压力测试** - 验证内存管理
- **并发测试** - 验证同步机制

### 3. 性能测试
- **上下文切换开销** - 测量调度性能
- **系统调用延迟** - 测量接口性能
- **内存分配速度** - 测量内存管理效率

## 部署和使用

### 1. 构建系统
```bash
# 编译高级内核
make kernel-advanced

# 创建启动镜像
make floppy

# 运行测试
make run-advanced
```

### 2. 开发环境
- **交叉编译器** - gcc-x86_64-elf
- **调试器** - GDB + QEMU
- **版本控制** - Git

### 3. 文档工具
- **代码文档** - Doxygen
- **测试框架** - 自定义测试套件
- **性能分析** - 自定义监控工具

## 总结

Phase 6 实现了一个功能完整的类Unix操作系统内核，具有以下特点：

1. **完整的用户空间支持** - ELF文件加载和执行
2. **丰富的文件系统** - 类Unix文件操作接口
3. **高效的进程间通信** - 管道机制
4. **全面的系统监控** - 实时性能统计
5. **扩展的系统调用** - 82个标准接口
6. **先进的内存管理** - 分页和保护机制

该阶段的实现为Phase 7的网络功能提供了坚实的基础，展示了现代操作系统核心功能的完整实现。

## 技术规格

- **内核大小**: ~25KB
- **支持的系统调用**: 82个
- **最大进程数**: 64个
- **文件系统最大文件**: 1024个
- **管道缓冲区大小**: 4KB
- **页大小**: 4KB
- **支持的最大内存**: 4GB