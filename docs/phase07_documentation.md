# Tiny Operating System - Phase 7 技术文档

## 概述

Phase 7 实现了完整的网络功能和设备驱动框架，将操作系统从单机系统扩展为支持网络通信的现代化操作系统。本阶段包含TCP/IP协议栈、网络设备驱动、设备驱动框架和网络应用程序。

## 系统架构

### 核心组件

1. **TCP/IP协议栈** - 完整的网络协议实现
2. **设备驱动框架** - 统一的设备管理和驱动接口
3. **NE2000网络驱动** - 以太网设备驱动程序
4. **网络应用程序** - Ping和HTTP客户端
5. **Socket API** - 标准网络编程接口
6. **网络数据包管理** - 高效的数据包处理

## 详细技术实现

### 1. TCP/IP协议栈

#### 1.1 协议层次结构
```c
/* 以太网帧头 */
struct eth_header {
    uint8_t dest_mac[6];      /* 目标MAC地址 */
    uint8_t src_mac[6];       /* 源MAC地址 */
    uint16_t ethertype;       /* 以太网类型 */
    uint8_t payload[46];      /* 负载数据 */
} __attribute__((packed));

/* IP头部 */
struct ip_header {
    uint8_t version_ihl;     /* 版本和头部长度 */
    uint8_t tos;             /* 服务类型 */
    uint16_t total_length;   /* 总长度 */
    uint16_t identification; /* 标识符 */
    uint16_t flags_fragment; /* 标志和片偏移 */
    uint8_t ttl;             /* 生存时间 */
    uint8_t protocol;        /* 协议类型 */
    uint16_t checksum;       /* 校验和 */
    uint32_t src_ip;         /* 源IP地址 */
    uint32_t dest_ip;        /* 目标IP地址 */
} __attribute__((packed));

/* TCP头部 */
struct tcp_header {
    uint16_t src_port;       /* 源端口 */
    uint16_t dest_port;      /* 目标端口 */
    uint32_t seq_num;        /* 序列号 */
    uint32_t ack_num;        /* 确认号 */
    uint8_t data_offset;     /* 数据偏移 */
    uint8_t flags;           /* 标志位 */
    uint16_t window;         /* 窗口大小 */
    uint16_t checksum;       /* 校验和 */
    uint16_t urgent_ptr;     /* 紧急指针 */
} __attribute__((packed));
```

#### 1.2 支持的协议
- **以太网** (ETH_P_IP: 0x0800)
- **IPv4** (IP_PROTO_TCP: 6, IP_PROTO_UDP: 17, IP_PROTO_ICMP: 1)
- **TCP** - 传输控制协议
- **UDP** - 用户数据报协议
- **ICMP** - 互联网控制报文协议
- **ARP** - 地址解析协议

#### 1.3 网络数据包管理
```c
struct network_packet {
    uint8_t data[ETH_MTU];   /* 数据包数据 */
    uint32_t size;           /* 数据包大小 */
    uint32_t device_id;      /* 设备ID */
    uint32_t protocol;       /* 协议类型 */
    uint32_t timestamp;      /* 时间戳 */
    uint8_t used;            /* 使用标志 */
};
```

#### 1.4 关键函数
- `checksum16()` - 计算16位校验和
- `network_send_packet()` - 发送网络数据包
- `network_receive_packet()` - 接收网络数据包
- `ip_send_packet()` - 发送IP数据包
- `tcp_send_packet()` - 发送TCP数据包

### 2. 设备驱动框架

#### 2.1 设备抽象结构
```c
struct device {
    uint32_t device_id;      /* 设备ID */
    uint32_t device_type;    /* 设备类型 */
    uint32_t status;         /* 设备状态 */
    uint32_t irq;            /* 中断请求号 */
    uint32_t base_port;      /* 基地址址 */
    uint8_t mac_address[6];  /* MAC地址 */
    uint32_t (*init)(uint32_t);      /* 初始化函数 */
    uint32_t (*read)(uint8_t*, uint32_t);  /* 读取函数 */
    uint32_t (*write)(const uint8_t*, uint32_t); /* 写入函数 */
    uint32_t (*ioctl)(uint32_t, uint32_t); /* 控制函数 */
    void (*interrupt_handler)(void);     /* 中断处理函数 */
    const char* name;        /* 设备名称 */
    const char* description; /* 设备描述 */
};
```

#### 2.2 设备类型定义
```c
#define DEVICE_TYPE_NETWORK    0x01  /* 网络设备 */
#define DEVICE_TYPE_BLOCK      0x02  /* 块设备 */
#define DEVICE_TYPE_CHAR       0x03  /* 字符设备 */
#define DEVICE_TYPE_CONSOLE    0x04  /* 控制台设备 */
```

#### 2.3 设备管理功能
- **设备注册** - 动态注册新设备
- **设备查找** - 按类型或ID查找设备
- **设备状态管理** - 监控设备状态
- **即插即用支持** - 动态设备检测

#### 2.4 关键函数
- `device_register()` - 注册设备
- `device_find_by_type()` - 按类型查找设备
- `device_init_all()` - 初始化所有设备
- `device_handle_interrupt()` - 处理设备中断

### 3. NE2000网络驱动

#### 3.1 NE2000寄存器定义
```c
#define NE2000_CMD         0x00  /* 命令寄存器 */
#define NE2000_PSTART      0x01  /* 页起始寄存器 */
#define NE2000_PSTOP       0x02  /* 页停止寄存器 */
#define NE2000_BNRY        0x03  /* 边界寄存器 */
#define NE2000_TPSR        0x04  /* 发送页起始寄存器 */
#define NE2000_TBCR0       0x05  /* 发送字节计数0 */
#define NE2000_TBCR1       0x06  /* 发送字节计数1 */
#define NE2000_ISR         0x07  /* 中断状态寄存器 */
#define NE2000_RSAR0       0x08  /* 远程起始地址0 */
#define NE2000_RSAR1       0x09  /* 远程起始地址1 */
#define NE2000_RBCR0       0x0A  /* 远程字节计数0 */
#define NE2000_RBCR1       0x0B  /* 远程字节计数1 */
#define NE2000_RCR         0x0C  /* 接收配置寄存器 */
#define NE2000_TCR         0x0D  /* 发送配置寄存器 */
#define NE2000_DCR         0x0E  /* 数据配置寄存器 */
#define NE2000_IMR         0x0F  /* 中断屏蔽寄存器 */
```

#### 3.2 NE2000命令定义
```c
#define NE2000_CMD_STOP    0x01  /* 停止命令 */
#define NE2000_CMD_START   0x02  /* 启动命令 */
#define NE2000_CMD_TXP     0x04  /* 发送数据包 */
#define NE2000_CMD_RD0     0x08  /* 远程读取 */
#define NE2000_CMD_RD1     0x10  /* 远程读取1 */
#define NE2000_CMD_RD2     0x20  /* 远程读取2 */
#define NE2000_CMD_RD3     0x40  /* 远程读取3 */
```

#### 3.3 NE2000设备结构
```c
struct ne2000_device {
    uint16_t base_port;      /* 基地址址 */
    uint16_t irq;            /* 中断请求号 */
    uint8_t mac_address[6];  /* MAC地址 */
    uint8_t promiscuous;     /* 混杂模式 */
    uint32_t rx_packets;     /* 接收数据包计数 */
    uint32_t tx_packets;     /* 发送数据包计数 */
    uint32_t rx_errors;      /* 接收错误计数 */
    uint32_t tx_errors;      /* 发送错误计数 */
    uint8_t rx_buffer[ETH_MTU];  /* 接收缓冲区 */
    uint8_t tx_buffer[ETH_MTU];  /* 发送缓冲区 */
};
```

#### 3.4 NE2000驱动功能
- **初始化** - 复位和配置网卡
- **数据包发送** - 通过DMA发送数据包
- **数据包接收** - 中断驱动的数据包接收
- **中断处理** - 处理接收和发送完成中断
- **错误处理** - 处理传输错误和超时

#### 3.5 关键函数
- `ne2000_init()` - 初始化NE2000网卡
- `ne2000_transmit()` - 发送数据包
- `ne2000_receive()` - 接收数据包
- `ne2000_interrupt_handler()` - 中断处理函数
- `ne2000_read_reg()` - 读取寄存器
- `ne2000_write_reg()` - 写入寄存器

### 4. Socket API

#### 4.1 Socket结构
```c
struct socket {
    uint32_t socket_id;      /* Socket ID */
    uint32_t domain;         /* 地址域 (AF_INET) */
    uint32_t type;           /* Socket类型 (SOCK_STREAM, SOCK_DGRAM) */
    uint32_t protocol;       /* 协议类型 */
    uint32_t state;          /* Socket状态 */
    uint32_t local_port;     /* 本地端口 */
    uint32_t remote_port;    /* 远程端口 */
    uint32_t remote_ip;      /* 远程IP地址 */
    uint32_t device_id;      /* 关联的设备ID */
    uint8_t* rx_buffer;      /* 接收缓冲区 */
    uint32_t rx_buffer_size; /* 接收缓冲区大小 */
    uint32_t rx_data_size;   /* 接收数据大小 */
    uint8_t connected;       /* 连接状态 */
};
```

#### 4.2 Socket状态
```c
#define SOCKET_STATE_CLOSED    0  /* 关闭状态 */
#define SOCKET_STATE_LISTEN    1  /* 监听状态 */
#define SOCKET_STATE_SYN_SENT  2  /* SYN已发送 */
#define SOCKET_STATE_SYN_RCVD  3  /* SYN已接收 */
#define SOCKET_STATE_ESTABLISHED 4  /* 已建立连接 */
#define SOCKET_STATE_FIN_WAIT  5  /* FIN等待 */
#define SOCKET_STATE_CLOSE_WAIT 6  /* 关闭等待 */
```

#### 4.3 Socket API函数
- `socket_create()` - 创建Socket
- `socket_connect()` - 建立连接
- `socket_bind()` - 绑定地址
- `socket_listen()` - 监听连接
- `socket_accept()` - 接受连接
- `socket_send()` - 发送数据
- `socket_recv()` - 接收数据
- `socket_close()` - 关闭Socket

### 5. 网络应用程序

#### 5.1 Ping实现
```c
/* ICMP Echo Request */
struct icmp_echo {
    uint8_t type;            /* ICMP类型 */
    uint8_t code;            /* ICMP代码 */
    uint16_t checksum;       /* 校验和 */
    uint16_t identifier;     /* 标识符 */
    uint16_t sequence;       /* 序列号 */
    uint8_t data[32];        /* 数据负载 */
} __attribute__((packed));

#define ICMP_ECHO_REQUEST    8  /* Echo请求 */
#define ICMP_ECHO_REPLY      0  /* Echo应答 */
```

#### 5.2 HTTP客户端
```c
/* HTTP请求结构 */
struct http_request {
    char method[16];         /* HTTP方法 */
    char url[256];           /* URL地址 */
    char host[64];           /* 主机名 */
    char path[128];          /* 路径 */
    char headers[512];       /* HTTP头部 */
    char body[1024];         /* 请求体 */
};

/* HTTP响应结构 */
struct http_response {
    int status_code;         /* 状态码 */
    char status_message[64]; /* 状态消息 */
    char headers[512];       /* 响应头部 */
    char body[2048];         /* 响应体 */
};
```

#### 5.3 DNS解析模拟
```c
/* DNS查询结构 */
struct dns_query {
    uint16_t transaction_id; /* 事务ID */
    uint16_t flags;          /* 标志 */
    uint16_t questions;      /* 问题数 */
    uint16_t answers;        /* 回答数 */
    uint16_t authority;      /* 授权数 */
    uint16_t additional;     /* 附加数 */
    char domain[256];        /* 域名 */
    uint16_t type;           /* 查询类型 */
    uint16_t class;          /* 查询类 */
};
```

#### 5.4 网络应用功能
- **Ping工具** - ICMP Echo请求/应答
- **HTTP客户端** - 简单的HTTP GET请求
- **DNS解析** - 域名到IP地址解析
- **网络测试** - 连通性和性能测试

### 6. 网络初始化和配置

#### 6.1 网络初始化流程
```c
void network_init(void) {
    /* 初始化网络数据包池 */
    for (int i = 0; i < MAX_NETWORK_PACKETS; i++) {
        network_packets[i].size = 0;
        network_packets[i].device_id = 0;
        network_packets[i].used = 0;
    }
    
    /* 初始化Socket池 */
    for (int i = 0; i < MAX_SOCKETS; i++) {
        sockets[i].state = SOCKET_STATE_CLOSED;
        sockets[i].socket_id = i;
    }
    
    /* 初始化设备管理器 */
    device_manager_init();
    
    /* 注册NE2000驱动 */
    ne2000_device_init();
}
```

#### 6.2 网络配置
- **IP地址配置** - 静态IP地址分配
- **子网掩码** - 网络掩码配置
- **默认网关** - 默认路由配置
- **DNS服务器** - DNS服务器地址

## 内存管理

### 1. 网络数据包内存管理
- **预分配缓冲区** - 固定大小的数据包缓冲区
- **内存池管理** - 高效的内存分配和回收
- **零拷贝优化** - 减少数据复制操作

### 2. Socket缓冲区管理
- **动态分配** - 按需分配接收缓冲区
- **流量控制** - 基于窗口大小的流量控制
- **内存保护** - 防止缓冲区溢出

## 中断处理

### 1. 网络中断处理
- **接收中断** - 处理接收到的数据包
- **发送完成中断** - 处理发送完成事件
- **错误中断** - 处理传输错误

### 2. 中断服务例程
```c
void ne2000_interrupt_handler(void) {
    uint8_t isr = ne2000_read_reg(NE2000_ISR);
    
    if (isr & NE2000_ISR_RX) {
        /* 处理接收中断 */
        ne2000_handle_receive();
    }
    
    if (isr & NE2000_ISR_TX) {
        /* 处理发送完成中断 */
        ne2000_handle_tx_complete();
    }
    
    if (isr & NE2000_ISR_ERR) {
        /* 处理错误中断 */
        ne2000_handle_error();
    }
}
```

## 性能优化

### 1. 网络性能优化
- **DMA传输** - 使用DMA提高数据传输效率
- **中断合并** - 减少中断处理开销
- **批量处理** - 批量处理网络数据包

### 2. 内存访问优化
- **缓存对齐** - 优化内存访问模式
- **预取优化** - 减少内存访问延迟
- **零拷贝** - 避免不必要的数据复制

## 测试和验证

### 1. 单元测试
- **协议栈测试** - 验证TCP/IP协议实现
- **设备驱动测试** - 验证NE2000驱动功能
- **Socket API测试** - 验证网络编程接口
- **网络应用测试** - 验证Ping和HTTP功能

### 2. 集成测试
- **端到端测试** - 完整的网络通信测试
- **并发测试** - 多连接并发处理测试
- **压力测试** - 高负载下的性能测试

### 3. 性能测试
- **吞吐量测试** - 测量网络传输速度
- **延迟测试** - 测量网络响应时间
- **并发连接测试** - 测试最大并发连接数

## 错误处理

### 1. 网络错误处理
- **连接超时** - 处理连接建立超时
- **传输错误** - 处理数据传输错误
- **缓冲区溢出** - 防止缓冲区溢出攻击

### 2. 设备错误处理
- **设备故障** - 处理设备硬件故障
- **驱动错误** - 处理驱动程序错误
- **资源不足** - 处理内存资源不足

## 安全特性

### 1. 网络安全
- **输入验证** - 验证所有网络输入
- **缓冲区保护** - 防止缓冲区溢出
- **访问控制** - 基于端口的访问控制

### 2. 系统安全
- **权限分离** - 内核态和用户态分离
- **内存保护** - 页级内存保护
- **中断保护** - 安全的中断处理

## 部署和使用

### 1. 构建系统
```bash
# 编译网络内核
make kernel-network

# 创建启动镜像
make floppy

# 运行测试
make run-network
```

### 2. 网络配置
- **QEMU网络配置** - 配置QEMU虚拟网络
- **TAP设备** - 使用TAP设备连接物理网络
- **用户模式网络** - 使用SLIRP用户模式网络

### 3. 开发环境
- **交叉编译器** - gcc-x86_64-elf
- **网络调试工具** - Wireshark, tcpdump
- **虚拟化平台** - QEMU, VirtualBox

## 总结

Phase 7 实现了一个完整的网络操作系统，具有以下特点：

1. **完整的TCP/IP协议栈** - 支持IPv4、TCP、UDP、ICMP等协议
2. **统一的设备驱动框架** - 可扩展的设备管理架构
3. **高性能网络驱动** - NE2000以太网驱动程序
4. **标准Socket API** - 兼容BSD Socket接口
5. **实用的网络应用** - Ping、HTTP客户端等工具
6. **强大的错误处理** - 完善的错误恢复机制
7. **良好的性能优化** - 高效的网络数据传输

该阶段的实现标志着一个功能完整的微型操作系统的完成，具备了现代操作系统的核心网络功能，为进一步的功能扩展奠定了坚实的基础。

## 技术规格

- **支持的协议**: IPv4, TCP, UDP, ICMP, ARP
- **最大Socket数**: 64个
- **网络数据包池**: 128个数据包
- **最大传输单元**: 1500字节
- **支持的设备**: NE2000兼容网卡
- **最大并发连接**: 32个
- **内存占用**: ~50KB
- **内核大小**: ~35KB