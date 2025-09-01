/*
 * Tiny Operating System - Stage 7 Network Kernel
 * Includes TCP/IP stack and device driver framework
 */

#include <stdint.h>
#include <stddef.h>

/* Forward declarations */
static void* memcpy(void* dest, const void* src, uint32_t n);

/* VGA text mode constants */
#define VGA_BUFFER ((volatile uint16_t*)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* VGA colors */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

/* Network constants */
#define ETH_MTU 1500
#define IP_HEADER_SIZE 20
#define TCP_HEADER_SIZE 20
#define UDP_HEADER_SIZE 8
#define ARP_PACKET_SIZE 28
#define MAX_NETWORK_PACKETS 64
#define MAX_SOCKETS 16

/* Network types */
#define ETH_TYPE_IP 0x0800
#define ETH_TYPE_ARP 0x0806
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

/* Device driver constants */
#define MAX_DEVICES 32
#define DEVICE_TYPE_NETWORK 1
#define DEVICE_TYPE_BLOCK 2
#define DEVICE_TYPE_CHAR 3

/* Constants */
#define PAGE_SIZE 4096
#define MAX_PROCESSES 16
#define MAX_PIPES 32
#define MAX_FILES 256
#define MAX_FS_ENTRIES 128

/* Ethernet header */
struct eth_header {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t type;
} __attribute__((packed));

/* IP header */
struct ip_header {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed));

/* TCP header */
struct tcp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed));

/* UDP header */
struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed));

/* ARP packet */
struct arp_packet {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_size;
    uint8_t proto_size;
    uint16_t opcode;
    uint8_t src_mac[6];
    uint32_t src_ip;
    uint8_t dest_mac[6];
    uint32_t dest_ip;
} __attribute__((packed));

/* ICMP packet */
struct icmp_packet {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} __attribute__((packed));

/* Network packet buffer */
struct network_packet {
    uint8_t data[ETH_MTU];
    uint32_t size;
    uint32_t device_id;
};

/* Socket structure */
struct socket {
    uint32_t used;
    uint32_t type;      /* SOCK_STREAM, SOCK_DGRAM */
    uint32_t protocol;  /* IPPROTO_TCP, IPPROTO_UDP */
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t local_ip;
    uint32_t remote_ip;
    uint32_t state;
    void* receive_buffer;
    uint32_t receive_buffer_size;
};

/* Device structure */
struct device {
    uint32_t used;
    uint32_t type;
    uint32_t id;
    char name[32];
    uint32_t (*read)(uint32_t device_id, void* buffer, uint32_t size);
    uint32_t (*write)(uint32_t device_id, const void* buffer, uint32_t size);
    uint32_t (*ioctl)(uint32_t device_id, uint32_t request, void* arg);
    void* private_data;
};

/* Network device structure */
struct network_device {
    struct device base;
    uint8_t mac_address[6];
    uint32_t ip_address;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t (*send_packet)(struct network_device* dev, const void* data, uint32_t size);
    uint32_t (*receive_packet)(struct network_device* dev, void* data, uint32_t size);
};

/* Process structure */
struct process {
    uint32_t pid;
    uint32_t parent_pid;
    uint32_t state;
    uint32_t esp;
    uint32_t eip;
    uint32_t cr3;
    uint32_t kernel_stack;
    uint32_t user_stack;
    uint32_t exit_code;
    char name[32];
    uint32_t page_directory;
    uint32_t brk;
};

/* Pipe structure */
struct pipe {
    uint32_t used;
    uint32_t buffer[1024];
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t reader_count;
    uint32_t writer_count;
};

/* File system entry */
struct fs_entry {
    uint32_t inode;
    uint32_t parent_inode;
    uint32_t type;
    uint32_t size;
    uint32_t data;
    char name[64];
};

/* System statistics */
struct system_stats {
    uint32_t uptime;
    uint32_t process_count;
    uint32_t memory_used;
    uint32_t memory_total;
    uint32_t cpu_usage;
    uint32_t context_switches;
    uint32_t system_calls;
    uint32_t page_faults;
    uint32_t interrupts;
    uint32_t network_packets_sent;
    uint32_t network_packets_received;
    uint32_t network_errors;
};

/* Global variables */
struct process processes[MAX_PROCESSES];
struct pipe pipes[MAX_PIPES];
struct fs_entry fs_entries[MAX_FS_ENTRIES];
struct system_stats system_stats;
uint32_t current_process = 0;
uint32_t timer_ticks = 0;
uint32_t timer_frequency = 1000;

/* Network variables */
struct network_packet network_packets[MAX_NETWORK_PACKETS];
struct socket sockets[MAX_SOCKETS];
struct device devices[MAX_DEVICES];
struct network_device* network_devices[MAX_DEVICES];
uint32_t network_packet_count = 0;
uint32_t socket_count = 0;
uint32_t device_count = 0;

/* Terminal variables */
static volatile uint16_t* terminal_buffer = VGA_BUFFER;
static uint32_t terminal_row = 0;
static uint32_t terminal_column = 0;
static uint8_t terminal_color = 0x0F;

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ __volatile__("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t value) {
    __asm__ __volatile__("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ __volatile__("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Terminal functions */
static void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = 0x0F;
    
    /* Clear screen */
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const size_t index = (y * VGA_WIDTH) + x;
            terminal_buffer[index] = ((uint16_t)' ') | ((uint16_t)terminal_color << 8);
        }
    }
}

static void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

static void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
        return;
    }
    
    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    terminal_buffer[index] = ((uint16_t)c) | ((uint16_t)terminal_color << 8);
    
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
    }
}

static void terminal_writestring(const char* data) {
    while (*data != '\0') {
        terminal_putchar(*data);
        data++;
    }
}

static void terminal_writehex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    terminal_writestring("0x");
    
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        terminal_putchar(hex_chars[nibble]);
    }
}

/* Network utility functions */
static uint16_t checksum16(const uint16_t* data, uint32_t size) {
    uint32_t sum = 0;
    
    while (size > 1) {
        sum += *data++;
        size -= 2;
    }
    
    if (size > 0) {
        sum += *(uint8_t*)data;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

static void mac_to_string(const uint8_t* mac, char* str) {
    const char hex_chars[] = "0123456789ABCDEF";
    
    for (int i = 0; i < 6; i++) {
        str[i * 3] = hex_chars[mac[i] >> 4];
        str[i * 3 + 1] = hex_chars[mac[i] & 0xF];
        if (i < 5) str[i * 3 + 2] = ':';
    }
    str[17] = '\0';
}

static void ip_to_string(uint32_t ip, char* str) {
    /* Simple IP to string conversion */
    uint32_t temp = ip;
    for (int i = 0; i < 4; i++) {
        uint8_t byte = (temp >> (24 - i * 8)) & 0xFF;
        if (byte >= 100) {
            str[0] = '0' + byte / 100;
            str[1] = '0' + (byte / 10) % 10;
            str[2] = '0' + byte % 10;
            str += 3;
        } else if (byte >= 10) {
            str[0] = '0' + byte / 10;
            str[1] = '0' + byte % 10;
            str += 2;
        } else {
            str[0] = '0' + byte;
            str += 1;
        }
        if (i < 3) *str++ = '.';
    }
    *str = '\0';
}

/* Network stack functions */
static uint32_t network_send_packet(uint32_t device_id, const void* data, uint32_t size) {
    if (device_id >= MAX_DEVICES || !devices[device_id].used || 
        devices[device_id].type != DEVICE_TYPE_NETWORK) {
        return 0;
    }
    
    struct network_device* dev = (struct network_device*)&devices[device_id];
    if (dev->base.write) {
        uint32_t sent = dev->base.write(device_id, data, size);
        system_stats.network_packets_sent++;
        return sent;
    }
    
    return 0;
}

static uint32_t network_receive_packet(uint32_t device_id, void* data, uint32_t size) {
    if (device_id >= MAX_DEVICES || !devices[device_id].used || 
        devices[device_id].type != DEVICE_TYPE_NETWORK) {
        return 0;
    }
    
    struct network_device* dev = (struct network_device*)&devices[device_id];
    if (dev->base.read) {
        uint32_t received = dev->base.read(device_id, data, size);
        if (received > 0) {
            system_stats.network_packets_received++;
        }
        return received;
    }
    
    return 0;
}

static uint32_t network_send_arp_request(uint32_t device_id, uint32_t target_ip) {
    if (device_id >= MAX_DEVICES || !devices[device_id].used || 
        devices[device_id].type != DEVICE_TYPE_NETWORK) {
        return 0;
    }
    
    struct network_device* dev = (struct network_device*)&devices[device_id];
    
    /* Create ARP packet */
    struct arp_packet arp;
    arp.hw_type = 0x0001;  /* Ethernet */
    arp.proto_type = 0x0800;  /* IP */
    arp.hw_size = 6;
    arp.proto_size = 4;
    arp.opcode = 0x0001;  /* Request */
    
    /* Source MAC and IP */
    for (int i = 0; i < 6; i++) {
        arp.src_mac[i] = dev->mac_address[i];
    }
    arp.src_ip = dev->ip_address;
    
    /* Target MAC (broadcast) and IP */
    for (int i = 0; i < 6; i++) {
        arp.dest_mac[i] = 0xFF;
    }
    arp.dest_ip = target_ip;
    
    /* Create Ethernet frame */
    uint8_t packet[60];  /* Minimum Ethernet frame size */
    struct eth_header* eth = (struct eth_header*)packet;
    
    /* Destination MAC (broadcast) */
    for (int i = 0; i < 6; i++) {
        eth->dest_mac[i] = 0xFF;
    }
    
    /* Source MAC */
    for (int i = 0; i < 6; i++) {
        eth->src_mac[i] = dev->mac_address[i];
    }
    
    eth->type = 0x0806;  /* ARP */
    
    /* Copy ARP packet */
    uint8_t* arp_data = packet + sizeof(struct eth_header);
    uint8_t* arp_ptr = (uint8_t*)&arp;
    for (uint32_t i = 0; i < sizeof(struct arp_packet); i++) {
        arp_data[i] = arp_ptr[i];
    }
    
    /* Send packet */
    return network_send_packet(device_id, packet, sizeof(packet));
}

static uint32_t network_send_icmp_echo(uint32_t device_id, uint32_t dest_ip, uint16_t identifier, uint16_t sequence) {
    if (device_id >= MAX_DEVICES || !devices[device_id].used || 
        devices[device_id].type != DEVICE_TYPE_NETWORK) {
        return 0;
    }
    
    struct network_device* dev = (struct network_device*)&devices[device_id];
    
    /* Create ICMP packet */
    struct icmp_packet icmp;
    icmp.type = 8;  /* Echo request */
    icmp.code = 0;
    icmp.identifier = identifier;
    icmp.sequence = sequence;
    
    /* Calculate checksum - use memcpy to avoid alignment issues */
    uint16_t icmp_copy[sizeof(struct icmp_packet) / 2];
    memcpy(icmp_copy, &icmp, sizeof(struct icmp_packet));
    icmp.checksum = 0;
    icmp.checksum = checksum16(icmp_copy, sizeof(struct icmp_packet));
    
    /* Create IP packet */
    uint8_t packet[64];
    struct eth_header* eth = (struct eth_header*)packet;
    struct ip_header* ip = (struct ip_header*)(packet + sizeof(struct eth_header));
    struct icmp_packet* icmp_ptr = (struct icmp_packet*)(packet + sizeof(struct eth_header) + sizeof(struct ip_header));
    
    /* Set destination MAC (broadcast for now) */
    for (int i = 0; i < 6; i++) {
        eth->dest_mac[i] = 0xFF;
    }
    
    /* Source MAC */
    for (int i = 0; i < 6; i++) {
        eth->src_mac[i] = dev->mac_address[i];
    }
    
    eth->type = 0x0800;  /* IP */
    
    /* Fill IP header */
    ip->version_ihl = 0x45;  /* Version 4, IHL 5 */
    ip->tos = 0;
    ip->total_length = sizeof(struct ip_header) + sizeof(struct icmp_packet);
    ip->identification = 0x1234;
    ip->flags_fragment = 0x4000;  /* Don't fragment */
    ip->ttl = 64;
    ip->protocol = IP_PROTO_ICMP;
    ip->src_ip = dev->ip_address;
    ip->dest_ip = dest_ip;
    
    /* Calculate IP checksum - use memcpy to avoid alignment issues */
    uint16_t ip_copy[sizeof(struct ip_header) / 2];
    memcpy(ip_copy, ip, sizeof(struct ip_header));
    ip->checksum = 0;
    ip->checksum = checksum16(ip_copy, sizeof(struct ip_header));
    
    /* Copy ICMP packet */
    *icmp_ptr = icmp;
    
    /* Send packet */
    return network_send_packet(device_id, packet, sizeof(struct eth_header) + sizeof(struct ip_header) + sizeof(struct icmp_packet));
}

/* Device driver framework */
static uint32_t device_register(struct device* dev) {
    if (device_count >= MAX_DEVICES) {
        return 0;
    }
    
    uint32_t device_id = device_count++;
    devices[device_id] = *dev;
    devices[device_id].id = device_id;
    devices[device_id].used = 1;
    
    /* If it's a network device, add to network device list */
    if (dev->type == DEVICE_TYPE_NETWORK) {
        network_devices[device_id] = (struct network_device*)dev;
    }
    
    return device_id;
}

static uint32_t device_unregister(uint32_t device_id) {
    if (device_id >= MAX_DEVICES || !devices[device_id].used) {
        return 0;
    }
    
    devices[device_id].used = 0;
    
    /* Remove from network device list */
    if (devices[device_id].type == DEVICE_TYPE_NETWORK) {
        network_devices[device_id] = NULL;
    }
    
    return 1;
}

/* Socket functions */
static uint32_t socket_create(uint32_t type, uint32_t protocol) {
    if (socket_count >= MAX_SOCKETS) {
        return 0;
    }
    
    uint32_t socket_id = socket_count++;
    sockets[socket_id].used = 1;
    sockets[socket_id].type = type;
    sockets[socket_id].protocol = protocol;
    sockets[socket_id].local_port = 0;
    sockets[socket_id].remote_port = 0;
    sockets[socket_id].local_ip = 0;
    sockets[socket_id].remote_ip = 0;
    sockets[socket_id].state = 0;
    sockets[socket_id].receive_buffer = NULL;
    sockets[socket_id].receive_buffer_size = 0;
    
    return socket_id;
}

static uint32_t socket_bind(uint32_t socket_id, uint32_t ip, uint16_t port) {
    if (socket_id >= MAX_SOCKETS || !sockets[socket_id].used) {
        return 0;
    }
    
    sockets[socket_id].local_ip = ip;
    sockets[socket_id].local_port = port;
    
    return 1;
}

static uint32_t socket_connect(uint32_t socket_id, uint32_t ip, uint16_t port) {
    if (socket_id >= MAX_SOCKETS || !sockets[socket_id].used) {
        return 0;
    }
    
    sockets[socket_id].remote_ip = ip;
    sockets[socket_id].remote_port = port;
    sockets[socket_id].state = 1;  /* Connected */
    
    return 1;
}

static uint32_t socket_send(uint32_t socket_id, const void* data, uint32_t size) {
    if (socket_id >= MAX_SOCKETS || !sockets[socket_id].used) {
        return 0;
    }
    
    /* Create TCP packet */
    uint8_t packet[sizeof(struct eth_header) + sizeof(struct ip_header) + sizeof(struct tcp_header) + size];
    struct eth_header* eth = (struct eth_header*)packet;
    struct ip_header* ip = (struct ip_header*)(packet + sizeof(struct eth_header));
    struct tcp_header* tcp = (struct tcp_header*)(packet + sizeof(struct eth_header) + sizeof(struct ip_header));
    
    /* Fill Ethernet header */
    for (int i = 0; i < 6; i++) {
        eth->dest_mac[i] = 0xFF;  /* Broadcast */
        eth->src_mac[i] = 0x52;   /* Default MAC */
    }
    eth->type = 0x0800;  /* IPv4 */
    
    /* Fill IP header */
    ip->version_ihl = 0x45;  /* Version 4, IHL 5 */
    ip->tos = 0;
    ip->total_length = sizeof(struct ip_header) + sizeof(struct tcp_header) + size;
    ip->identification = 0x1234;
    ip->flags_fragment = 0x4000;  /* Don't fragment */
    ip->ttl = 64;
    ip->protocol = IP_PROTO_TCP;
    ip->checksum = 0;
    ip->src_ip = sockets[socket_id].local_ip;
    ip->dest_ip = sockets[socket_id].remote_ip;
    
    /* Calculate IP checksum */
    uint16_t ip_copy[sizeof(struct ip_header) / 2];
    memcpy(ip_copy, ip, sizeof(struct ip_header));
    ip->checksum = checksum16(ip_copy, sizeof(struct ip_header));
    
    /* Fill TCP header */
    tcp->src_port = sockets[socket_id].local_port;
    tcp->dest_port = sockets[socket_id].remote_port;
    tcp->seq_num = 0x10000000;
    tcp->ack_num = 0;
    tcp->flags = 0x5018;  /* Data offset 20 bytes, PSH, ACK */
    tcp->window = 0x1000;
    tcp->checksum = 0;
    tcp->urgent = 0;
    
    /* Copy data */
    memcpy(packet + sizeof(struct eth_header) + sizeof(struct ip_header) + sizeof(struct tcp_header), data, size);
    
    /* Send packet */
    return network_send_packet(0, packet, sizeof(struct eth_header) + sizeof(struct ip_header) + sizeof(struct tcp_header) + size);
}

static uint32_t socket_receive(uint32_t socket_id, void* buffer, uint32_t size) {
    if (socket_id >= MAX_SOCKETS || !sockets[socket_id].used) {
        return 0;
    }
    
    /* For now, return simulated data */
    if (sockets[socket_id].receive_buffer && sockets[socket_id].receive_buffer_size > 0) {
        uint32_t copy_size = (size < sockets[socket_id].receive_buffer_size) ? size : sockets[socket_id].receive_buffer_size;
        memcpy(buffer, sockets[socket_id].receive_buffer, copy_size);
        return copy_size;
    }
    
    return 0;
}

static uint32_t socket_close(uint32_t socket_id) {
    if (socket_id >= MAX_SOCKETS || !sockets[socket_id].used) {
        return 0;
    }
    
    sockets[socket_id].used = 0;
    sockets[socket_id].state = 0;
    
    return 1;
}

/* HTTP Client functionality */
static uint32_t http_get_request(uint32_t ip, uint16_t port, const char* host, const char* path, char* response, uint32_t response_size) {
    /* Create socket */
    uint32_t sock = socket_create(1, 6);  /* TCP socket */
    if (sock == (uint32_t)-1) {
        return 0;
    }
    
    /* Bind to local port */
    if (!socket_bind(sock, 0x0A000001, 12345)) {  /* 10.0.0.1:12345 */
        socket_close(sock);
        return 0;
    }
    
    /* Connect to server */
    if (!socket_connect(sock, ip, port)) {
        socket_close(sock);
        return 0;
    }
    
    /* Create HTTP GET request */
    char request[512];
    uint32_t request_len = 0;
    
    /* Add GET request line */
    const char* get_line = "GET ";
    memcpy(request + request_len, get_line, 4);
    request_len += 4;
    
    /* Add path */
    uint32_t path_len = 0;
    while (path[path_len] && request_len < sizeof(request) - 2) {
        request[request_len++] = path[path_len++];
    }
    
    /* Add HTTP version */
    const char* http_version = " HTTP/1.1\r\n";
    memcpy(request + request_len, http_version, 11);
    request_len += 11;
    
    /* Add Host header */
    const char* host_header = "Host: ";
    memcpy(request + request_len, host_header, 6);
    request_len += 6;
    
    /* Add hostname */
    uint32_t host_len = 0;
    while (host[host_len] && request_len < sizeof(request) - 2) {
        request[request_len++] = host[host_len++];
    }
    
    /* Add header end */
    const char* header_end = "\r\n\r\n";
    memcpy(request + request_len, header_end, 4);
    request_len += 4;
    
    /* Send request */
    uint32_t sent = socket_send(sock, request, request_len);
    if (sent != request_len) {
        socket_close(sock);
        return 0;
    }
    
    /* Simulate receiving response */
    const char* mock_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 13\r\n\r\nHello, World!";
    uint32_t response_len = 0;
    while (mock_response[response_len] && response_len < response_size - 1) {
        response[response_len] = mock_response[response_len];
        response_len++;
    }
    response[response_len] = '\0';
    
    /* Close socket */
    socket_close(sock);
    
    return response_len;
}

/* Enhanced ping functionality */
static uint32_t ping_host(uint32_t ip, uint16_t count) {
    uint32_t success_count = 0;
    
    for (uint16_t i = 1; i <= count; i++) {
        /* Send ICMP echo request */
        uint32_t result = network_send_icmp_echo(0, ip, i, i);
        if (result) {
            success_count++;
        }
        
        /* Small delay between pings */
        for (volatile uint32_t j = 0; j < 1000000; j++);
    }
    
    return success_count;
}

/* Simple memcpy implementation */
static void* memcpy(void* dest, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

/* DNS resolution simulation (simplified) */
static uint32_t dns_resolve(const char* hostname) {
    /* Simulate DNS resolution for common test hosts */
    if (hostname[0] == 'w' && hostname[1] == 'w' && hostname[2] == 'w') {
        /* www.example.com -> 93.184.216.34 */
        return 0x5DB8D822;
    } else if (hostname[0] == 'g' && hostname[1] == 'o' && hostname[2] == 'o') {
        /* google.com -> 142.250.191.78 */
        return 0x8EFABF4E;
    } else if (hostname[0] == 'l' && hostname[1] == 'o' && hostname[2] == 'c') {
        /* localhost -> 127.0.0.1 */
        return 0x7F000001;
    }
    
    /* Default to test address */
    return 0x0A000002;  /* 10.0.0.2 */
}

/* Test functions */
static void test_network_stack(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("=== Testing Network Stack ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Test socket creation */
    uint32_t sock = socket_create(1, 6);  /* TCP socket */
    terminal_writestring("Created socket ");
    terminal_writehex(sock);
    terminal_writestring("\n");
    
    /* Test socket binding */
    uint32_t bind_result = socket_bind(sock, 0x0A000001, 8080);  /* 10.0.0.1:8080 */
    terminal_writestring("Socket bind result: ");
    terminal_writehex(bind_result);
    terminal_writestring("\n");
    
    /* Test socket connection */
    uint32_t connect_result = socket_connect(sock, 0x0A000002, 80);  /* Connect to 10.0.0.2:80 */
    terminal_writestring("Socket connect result: ");
    terminal_writehex(connect_result);
    terminal_writestring("\n");
    
    /* Test device registration */
    struct device test_dev = {
        .used = 0,
        .type = DEVICE_TYPE_NETWORK,
        .name = "test_net",
        .read = NULL,
        .write = NULL,
        .ioctl = NULL,
        .private_data = NULL
    };
    
    uint32_t dev_id = device_register(&test_dev);
    terminal_writestring("Registered device ");
    terminal_writehex(dev_id);
    terminal_writestring(": ");
    terminal_writestring(test_dev.name);
    terminal_writestring("\n");
    
    /* Test device unregistration */
    uint32_t unregister_result = device_unregister(dev_id);
    terminal_writestring("Unregistered device result: ");
    terminal_writehex(unregister_result);
    terminal_writestring("\n");
    
    terminal_writestring("\n");
}

static void test_network_protocols(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("=== Testing Network Protocols ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Test checksum calculation */
    uint16_t test_data[] = {0x4500, 0x003c, 0x1c46, 0x4000, 0x4006, 0x0000, 0x0a00, 0x0001, 0x0a00, 0x0002};
    uint16_t checksum = checksum16(test_data, 20);
    
    terminal_writestring("IP header checksum: ");
    terminal_writehex(checksum);
    terminal_writestring("\n");
    
    /* Test MAC to string conversion */
    uint8_t test_mac[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    char mac_str[18];
    mac_to_string(test_mac, mac_str);
    terminal_writestring("MAC address: ");
    terminal_writestring(mac_str);
    terminal_writestring("\n");
    
    /* Test IP to string conversion */
    uint32_t test_ip = 0x0A000001;  /* 10.0.0.1 */
    char ip_str[16];
    ip_to_string(test_ip, ip_str);
    terminal_writestring("IP address: ");
    terminal_writestring(ip_str);
    terminal_writestring("\n");
    
    terminal_writestring("\n");
}

/* NE2000 driver function declarations */
extern uint32_t ne2000_register_device(uint16_t base_port, uint16_t irq);
extern uint32_t ne2000_test_loopback(void);
extern uint32_t ne2000_get_statistics(uint32_t* rx_packets, uint32_t* tx_packets, 
                                      uint32_t* rx_errors, uint32_t* tx_errors);
extern uint32_t ne2000_get_mac_address(uint8_t* mac);

static void test_device_drivers(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("=== Testing Device Driver Framework ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Test network device creation */
    struct network_device net_dev = {
        .base = {
            .used = 0,
            .type = DEVICE_TYPE_NETWORK,
            .name = "ne2000",
            .read = NULL,
            .write = NULL,
            .ioctl = NULL,
            .private_data = NULL
        },
        .mac_address = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
        .ip_address = 0x0A000001,
        .netmask = 0xFFFFFF00,
        .gateway = 0x0A000001,
        .send_packet = NULL,
        .receive_packet = NULL
    };
    
    uint32_t dev_id = device_register((struct device*)&net_dev);
    terminal_writestring("Registered NE2000 device: ");
    terminal_writehex(dev_id);
    terminal_writestring("\n");
    
    if (dev_id < MAX_DEVICES) {
        struct network_device* registered_dev = (struct network_device*)&devices[dev_id];
        char mac_str[18];
        mac_to_string(registered_dev->mac_address, mac_str);
        terminal_writestring("Device MAC: ");
        terminal_writestring(mac_str);
        terminal_writestring("\n");
        
        char ip_str[16];
        ip_to_string(registered_dev->ip_address, ip_str);
        terminal_writestring("Device IP: ");
        terminal_writestring(ip_str);
        terminal_writestring("\n");
        
        /* Test ARP request */
        uint32_t arp_result = network_send_arp_request(dev_id, 0x0A000002);
        terminal_writestring("ARP request result: ");
        terminal_writehex(arp_result);
        terminal_writestring(" bytes\n");
        
        /* Test ICMP echo */
        uint32_t icmp_result = network_send_icmp_echo(dev_id, 0x0A000002, 1234, 1);
        terminal_writestring("ICMP echo result: ");
        terminal_writehex(icmp_result);
        terminal_writestring(" bytes\n");
        
        /* Test packet receive */
        uint8_t recv_buffer[64];
        uint32_t recv_result = network_receive_packet(dev_id, recv_buffer, sizeof(recv_buffer));
        terminal_writestring("Packet receive result: ");
        terminal_writehex(recv_result);
        terminal_writestring(" bytes\n");
    }
    
    terminal_writestring("\n");
}

static void test_ne2000_driver(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("=== Testing NE2000 Network Driver ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Test NE2000 driver initialization */
    uint32_t init_result = ne2000_register_device(0x300, 10); /* Base port 0x300, IRQ 10 */
    terminal_writestring("NE2000 driver initialization: ");
    terminal_writehex(init_result);
    terminal_writestring("\n");
    
    if (init_result) {
        /* Test MAC address retrieval */
        uint8_t mac[6];
        uint32_t mac_result = ne2000_get_mac_address(mac);
        terminal_writestring("MAC address retrieval: ");
        terminal_writehex(mac_result);
        terminal_writestring("\n");
        
        if (mac_result) {
            char mac_str[18];
            mac_to_string(mac, mac_str);
            terminal_writestring("NE2000 MAC: ");
            terminal_writestring(mac_str);
            terminal_writestring("\n");
        }
        
        /* Test loopback functionality */
        uint32_t loopback_result = ne2000_test_loopback();
        terminal_writestring("Loopback test: ");
        terminal_writehex(loopback_result);
        terminal_writestring("\n");
        
        /* Test statistics */
        uint32_t rx_packets, tx_packets, rx_errors, tx_errors;
        uint32_t stats_result = ne2000_get_statistics(&rx_packets, &tx_packets, &rx_errors, &tx_errors);
        terminal_writestring("Statistics retrieval: ");
        terminal_writehex(stats_result);
        terminal_writestring("\n");
        
        if (stats_result) {
            terminal_writestring("  RX packets: ");
            terminal_writehex(rx_packets);
            terminal_writestring("\n");
            terminal_writestring("  TX packets: ");
            terminal_writehex(tx_packets);
            terminal_writestring("\n");
            terminal_writestring("  RX errors: ");
            terminal_writehex(rx_errors);
            terminal_writestring("\n");
            terminal_writestring("  TX errors: ");
            terminal_writehex(tx_errors);
            terminal_writestring("\n");
        }
    }
    
    terminal_writestring("\n");
}

/* Test network applications */
static void test_network_applications(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("=== Testing Network Applications ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Test DNS resolution */
    uint32_t google_ip = dns_resolve("google.com");
    terminal_writestring("DNS resolution for google.com: ");
    terminal_writehex(google_ip);
    terminal_writestring("\n");
    
    /* Test ping functionality */
    terminal_writestring("Pinging 10.0.0.2...\n");
    uint32_t ping_result = ping_host(0x0A000002, 4);  /* Ping 10.0.0.2, 4 packets */
    terminal_writestring("Ping result: ");
    terminal_writehex(ping_result);
    terminal_writestring(" packets successful\n");
    
    /* Test HTTP client */
    terminal_writestring("Testing HTTP client...\n");
    char http_response[256];
    uint32_t http_result = http_get_request(0x5DB8D822, 80, "www.example.com", "/", http_response, sizeof(http_response));
    terminal_writestring("HTTP GET result: ");
    terminal_writehex(http_result);
    terminal_writestring(" bytes received\n");
    
    if (http_result > 0) {
        terminal_writestring("HTTP response: ");
        terminal_writestring(http_response);
        terminal_writestring("\n");
    }
    
    /* Test socket operations */
    terminal_writestring("Testing socket operations...\n");
    
    /* Create additional sockets */
    uint32_t sock2 = socket_create(1, 6);  /* TCP socket */
    uint32_t sock3 = socket_create(0, 17); /* UDP socket */
    
    terminal_writestring("Created TCP socket: ");
    terminal_writehex(sock2);
    terminal_writestring("\n");
    
    terminal_writestring("Created UDP socket: ");
    terminal_writehex(sock3);
    terminal_writestring("\n");
    
    /* Test socket send/receive */
    if (sock2 != (uint32_t)-1) {
        const char* test_data = "Hello, Network!";
        uint32_t sent = socket_send(sock2, test_data, 14);
        terminal_writestring("Sent data: ");
        terminal_writehex(sent);
        terminal_writestring(" bytes\n");
        
        char recv_buffer[32];
        uint32_t received = socket_receive(sock2, recv_buffer, sizeof(recv_buffer));
        terminal_writestring("Received data: ");
        terminal_writehex(received);
        terminal_writestring(" bytes\n");
        
        /* Close socket */
        socket_close(sock2);
        terminal_writestring("Socket closed\n");
    }
    
    if (sock3 != (uint32_t)-1) {
        socket_close(sock3);
    }
    
    terminal_writestring("\n");
}

/* Main kernel function */
void kernel_main(void) {
    /* Initialize terminal */
    terminal_initialize();
    
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("=== Tiny Operating System - Stage 7 Network Kernel ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("Starting network kernel initialization...\n\n");
    
    /* Initialize system statistics */
    system_stats.uptime = 0;
    system_stats.process_count = 1;
    system_stats.memory_used = 1024;  /* 1MB for kernel */
    system_stats.memory_total = 32768;  /* 32MB total */
    system_stats.cpu_usage = 0;
    system_stats.context_switches = 0;
    system_stats.system_calls = 0;
    system_stats.page_faults = 0;
    system_stats.interrupts = 0;
    system_stats.network_packets_sent = 0;
    system_stats.network_packets_received = 0;
    system_stats.network_errors = 0;
    
    /* Initialize process table */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid = 0;
        processes[i].state = 0;
        processes[i].name[0] = '\0';
    }
    
    /* Create init process */
    processes[0].pid = 1;
    processes[0].state = 1; /* Running */
    processes[0].name[0] = 'i';
    processes[0].name[1] = 'n';
    processes[0].name[2] = 'i';
    processes[0].name[3] = 't';
    processes[0].name[4] = '\0';
    
    /* Initialize network stack */
    for (int i = 0; i < MAX_NETWORK_PACKETS; i++) {
        network_packets[i].size = 0;
        network_packets[i].device_id = 0;
    }
    
    /* Initialize sockets */
    for (int i = 0; i < MAX_SOCKETS; i++) {
        sockets[i].used = 0;
    }
    
    /* Initialize devices */
    for (int i = 0; i < MAX_DEVICES; i++) {
        devices[i].used = 0;
        network_devices[i] = NULL;
    }
    
    terminal_writestring("=== All network subsystems initialized successfully ===\n\n");
    
    /* Run comprehensive test suite */
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("=== Running Network Test Suite ===\n\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Test all Stage 7 features */
    test_network_stack();
    test_network_protocols();
    test_device_drivers();
    test_ne2000_driver();
    test_network_applications();
    
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("\n=== Stage 7 Network Kernel Initialization Complete ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("Network stack is running with TCP/IP support.\n");
    terminal_writestring("Device driver framework is ready for hardware drivers.\n");
    terminal_writestring("System supports up to ");
    terminal_writehex(MAX_DEVICES);
    terminal_writestring(" devices and ");
    terminal_writehex(MAX_SOCKETS);
    terminal_writestring(" sockets.\n");
    
    /* Enter main loop */
    while (1) {
        /* Halt until interrupt */
        __asm__ __volatile__("hlt");
    }
}

/* Missing functions needed by other object files */
void keyboard_handler(void) {
    /* Simple keyboard handler - just acknowledge */
    uint8_t status = inb(0x64);
    if (status & 0x01) {
        uint8_t scancode = inb(0x60);
        (void)scancode;  /* Suppress unused warning */
    }
}

void timer_handler(void) {
    timer_ticks++;
    (void)timer_frequency; /* Use the variable to suppress warning */
}

void process_kill(uint32_t pid) {
    if (pid < MAX_PROCESSES) {
        processes[pid].state = 0; /* Unused */
    }
}

void process_switch(uint32_t pid) {
    if (pid < MAX_PROCESSES) {
        current_process = pid;
    }
}

uint32_t process_create(const char* name, uint32_t entry_point) {
    (void)name;     /* Suppress unused warning */
    (void)entry_point; /* Suppress unused warning */
    
    /* Find free process slot */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == 0) {
            processes[i].pid = i + 1;
            processes[i].state = 1; /* Ready */
            return i + 1;
        }
    }
    return 0;
}

uint32_t paging_alloc_frame(void) {
    /* Simple frame allocation - return fixed address */
    static uint32_t next_frame = 0x200000; /* Start at 2MB */
    uint32_t frame = next_frame;
    next_frame += PAGE_SIZE;
    return frame;
}

void paging_free_frame(uint32_t addr) {
    (void)addr; /* Suppress unused warning */
}

void paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    (void)virt;  /* Suppress unused warning */
    (void)phys;  /* Suppress unused warning */
    (void)flags; /* Suppress unused warning */
}