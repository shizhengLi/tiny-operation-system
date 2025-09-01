/*
 * NE2000 Network Device Driver
 * Implements NE2000 compatible Ethernet card driver
 */

#include <stdint.h>

/* Network constants from kernel */
#define ETH_MTU 1500
#define DEVICE_TYPE_NETWORK 1

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

/* NE2000 Registers */
#define NE2000_DATA_PORT 0x10
#define NE2000_RESET_PORT 0x1F
#define NE2000_COMMAND 0x00
#define NE2000_PAGE_START 0x01
#define NE2000_PAGE_STOP 0x02
#define NE2000_BOUNDARY 0x03
#define NE2000_TRANSMIT_STATUS 0x04
#define NE2000_TRANSMIT_PAGE 0x04
#define NE2000_TRANSMIT_COUNT 0x05
#define NE2000_INTERRUPT_STATUS 0x07
#define NE2000_REMOTE_COUNT 0x0D
#define NE2000_CONFIG 0x0E
#define NE2000_REMOTE_DMA 0x0F

/* NE2000 Commands */
#define NE2000_CMD_STOP 0x21
#define NE2000_CMD_START 0x22
#define NE2000_CMD_TRANSMIT 0x26
#define NE2000_CMD_READ 0x0A
#define NE2000_CMD_WRITE 0x12

/* NE2000 Interrupt Masks */
#define NE2000_INT_RX 0x01
#define NE2000_INT_TX 0x02
#define NE2000_INT_RXE 0x04
#define NE2000_INT_TXE 0x08
#define NE2000_INT_OVW 0x10
#define NE2000_INT_CNTD 0x20
#define NE2000_INT_RDC 0x40

/* NE2000 Configuration */
#define NE2000_CFG_BOS 0x01
#define NE2000_CFG_WTS 0x02
#define NE2000_CFG_DAS 0x08
#define NE2000_CFG_FDL0 0x10
#define NE2000_CFG_FDL1 0x20
#define NE2000_CFG_LAS 0x40
#define NE2000_CFG_MON 0x80

/* NE2000 Constants */
#define NE2000_START_PAGE 0x40
#define NE2000_STOP_PAGE 0x80
#define NE2000_BUFFER_SIZE 8192
#define NE2000_HEADER_SIZE 4

/* NE2000 device structure */
struct ne2000_device {
    uint16_t base_port;
    uint16_t irq;
    uint8_t mac_address[6];
    uint8_t current_page;
    uint8_t next_packet;
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint32_t rx_errors;
    uint32_t tx_errors;
    uint8_t buffer[NE2000_BUFFER_SIZE];
};

/* Global NE2000 device instance */
static struct ne2000_device ne2000_dev;

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

/* NE2000 helper functions */
static void ne2000_write_cr(uint8_t value) {
    outb(ne2000_dev.base_port + NE2000_COMMAND, value);
}

static __attribute__((used)) uint8_t ne2000_read_cr(void) {
    return inb(ne2000_dev.base_port + NE2000_COMMAND);
}

static void ne2000_write_remote_dma(uint16_t value) {
    outb(ne2000_dev.base_port + NE2000_REMOTE_DMA, value);
}

static void ne2000_write_remote_address(uint16_t addr) {
    outb(ne2000_dev.base_port + NE2000_REMOTE_COUNT, addr & 0xFF);
    outb(ne2000_dev.base_port + NE2000_REMOTE_COUNT + 1, addr >> 8);
}

static __attribute__((used)) uint16_t ne2000_read_remote_address(void) {
    uint16_t addr = inb(ne2000_dev.base_port + NE2000_REMOTE_COUNT);
    addr |= inb(ne2000_dev.base_port + NE2000_REMOTE_COUNT + 1) << 8;
    return addr;
}

static void ne2000_set_page(uint8_t page) {
    outb(ne2000_dev.base_port, page);
}

/* NE2000 initialization */
static uint32_t ne2000_init(uint16_t base_port, uint16_t irq) {
    ne2000_dev.base_port = base_port;
    ne2000_dev.irq = irq;
    ne2000_dev.rx_packets = 0;
    ne2000_dev.tx_packets = 0;
    ne2000_dev.rx_errors = 0;
    ne2000_dev.tx_errors = 0;
    
    /* Reset the card */
    outb(base_port + NE2000_RESET_PORT, 0xFF);
    for (int i = 0; i < 1000; i++) {
        inb(base_port + NE2000_RESET_PORT);
    }
    
    /* Check if card is present */
    outb(base_port, 0x00);
    uint8_t test = inb(base_port);
    if (test != 0x00) {
        return 0; /* Card not found */
    }
    
    /* Stop the NIC */
    ne2000_write_cr(NE2000_CMD_STOP);
    
    /* Set page 0 */
    ne2000_set_page(0x00);
    
    /* Configure the card */
    outb(base_port + NE2000_CONFIG, 0x49); /* 16-bit mode, FIFO normal */
    outb(base_port + NE2000_PAGE_START, NE2000_START_PAGE);
    outb(base_port + NE2000_PAGE_STOP, NE2000_STOP_PAGE);
    outb(base_port + NE2000_BOUNDARY, NE2000_START_PAGE);
    
    /* Set MAC address (default for testing) */
    ne2000_dev.mac_address[0] = 0x52;
    ne2000_dev.mac_address[1] = 0x54;
    ne2000_dev.mac_address[2] = 0x00;
    ne2000_dev.mac_address[3] = 0x12;
    ne2000_dev.mac_address[4] = 0x34;
    ne2000_dev.mac_address[5] = 0x56;
    
    /* Write MAC address to card */
    ne2000_set_page(0x01);
    for (int i = 0; i < 6; i++) {
        outb(base_port + 0x01 + i, ne2000_dev.mac_address[i]);
    }
    
    /* Set multicast mask (accept all) */
    for (int i = 0; i < 8; i++) {
        outb(base_port + 0x08 + i, 0xFF);
    }
    
    /* Set page 0 */
    ne2000_set_page(0x00);
    
    /* Initialize current page and next packet */
    ne2000_dev.current_page = NE2000_START_PAGE;
    ne2000_dev.next_packet = NE2000_START_PAGE;
    
    /* Start the NIC */
    ne2000_write_cr(NE2000_CMD_START);
    
    /* Clear interrupts */
    outb(base_port + NE2000_INTERRUPT_STATUS, 0xFF);
    
    return 1; /* Success */
}

/* NE2000 transmit function */
static uint32_t ne2000_transmit(const void* data, uint32_t size) {
    if (size > ETH_MTU) {
        return 0;
    }
    
    /* Stop the NIC */
    ne2000_write_cr(NE2000_CMD_STOP);
    
    /* Set page 0 */
    ne2000_set_page(0x00);
    
    /* Set transmit parameters */
    uint8_t transmit_page = NE2000_STOP_PAGE - (size + NE2000_HEADER_SIZE + 255) / 256;
    outb(ne2000_dev.base_port + NE2000_TRANSMIT_PAGE, transmit_page);
    outb(ne2000_dev.base_port + NE2000_TRANSMIT_COUNT, size & 0xFF);
    outb(ne2000_dev.base_port + NE2000_TRANSMIT_COUNT + 1, size >> 8);
    
    /* Set remote DMA for write */
    ne2000_write_cr(NE2000_CMD_WRITE);
    ne2000_write_remote_dma(size);
    ne2000_write_remote_address(0);
    
    /* Write data */
    const uint8_t* byte_data = (const uint8_t*)data;
    for (uint32_t i = 0; i < size; i++) {
        outb(ne2000_dev.base_port + NE2000_DATA_PORT, byte_data[i]);
    }
    
    /* Start transmission */
    ne2000_write_cr(NE2000_CMD_TRANSMIT);
    
    /* Wait for transmission complete */
    uint32_t timeout = 10000;
    while (timeout--) {
        uint8_t status = inb(ne2000_dev.base_port + NE2000_INTERRUPT_STATUS);
        if (status & NE2000_INT_TX) {
            outb(ne2000_dev.base_port + NE2000_INTERRUPT_STATUS, NE2000_INT_TX);
            ne2000_dev.tx_packets++;
            return size;
        }
        if (status & NE2000_INT_TXE) {
            outb(ne2000_dev.base_port + NE2000_INTERRUPT_STATUS, NE2000_INT_TXE);
            ne2000_dev.tx_errors++;
            return 0;
        }
    }
    
    ne2000_dev.tx_errors++;
    return 0;
}

/* NE2000 receive function */
static uint32_t ne2000_receive(void* data, uint32_t max_size) {
    /* Set page 0 */
    ne2000_set_page(0x00);
    
    /* Check if there are packets */
    uint8_t boundary = inb(ne2000_dev.base_port + NE2000_BOUNDARY);
    uint8_t current = ne2000_dev.current_page;
    
    if (current == boundary) {
        return 0; /* No packets */
    }
    
    /* Read packet header */
    ne2000_write_cr(NE2000_CMD_READ);
    ne2000_write_remote_dma(4);
    ne2000_write_remote_address(current << 8);
    
    uint8_t header[4];
    for (int i = 0; i < 4; i++) {
        header[i] = inb(ne2000_dev.base_port + NE2000_DATA_PORT);
    }
    
    uint16_t status = (header[1] << 8) | header[0];
    uint16_t next_page = header[2];
    uint16_t packet_size = (header[3] << 8) | header[2];
    
    if (packet_size > max_size) {
        packet_size = max_size;
    }
    
    /* Read packet data */
    ne2000_write_remote_dma(packet_size);
    ne2000_write_remote_address((current << 8) + 4);
    
    uint8_t* byte_data = (uint8_t*)data;
    for (uint16_t i = 0; i < packet_size; i++) {
        byte_data[i] = inb(ne2000_dev.base_port + NE2000_DATA_PORT);
    }
    
    /* Update current page */
    ne2000_dev.current_page = next_page;
    outb(ne2000_dev.base_port + NE2000_BOUNDARY, next_page - 1);
    
    if (status & 0x01) {
        ne2000_dev.rx_packets++;
    } else {
        ne2000_dev.rx_errors++;
    }
    
    return packet_size;
}

/* NE2000 interrupt handler */
static __attribute__((used)) void ne2000_interrupt_handler(void) {
    uint8_t status = inb(ne2000_dev.base_port + NE2000_INTERRUPT_STATUS);
    
    if (status & NE2000_INT_RX) {
        /* Packet received */
        outb(ne2000_dev.base_port + NE2000_INTERRUPT_STATUS, NE2000_INT_RX);
    }
    
    if (status & NE2000_INT_TX) {
        /* Packet transmitted */
        outb(ne2000_dev.base_port + NE2000_INTERRUPT_STATUS, NE2000_INT_TX);
    }
    
    if (status & NE2000_INT_RXE) {
        /* Receive error */
        outb(ne2000_dev.base_port + NE2000_INTERRUPT_STATUS, NE2000_INT_RXE);
        ne2000_dev.rx_errors++;
    }
    
    if (status & NE2000_INT_TXE) {
        /* Transmit error */
        outb(ne2000_dev.base_port + NE2000_INTERRUPT_STATUS, NE2000_INT_TXE);
        ne2000_dev.tx_errors++;
    }
}

/* NE2000 device driver interface functions */
static __attribute__((used)) uint32_t ne2000_read(uint32_t device_id, void* buffer, uint32_t size) {
    (void)device_id; /* Suppress unused warning */
    return ne2000_receive(buffer, size);
}

static __attribute__((used)) uint32_t ne2000_write(uint32_t device_id, const void* buffer, uint32_t size) {
    (void)device_id; /* Suppress unused warning */
    return ne2000_transmit(buffer, size);
}

static __attribute__((used)) uint32_t ne2000_ioctl(uint32_t device_id, uint32_t request, void* arg) {
    (void)device_id; /* Suppress unused warning */
    
    switch (request) {
        case 1: /* Get MAC address */
            if (arg) {
                uint8_t* mac = (uint8_t*)arg;
                for (int i = 0; i < 6; i++) {
                    mac[i] = ne2000_dev.mac_address[i];
                }
            }
            return 1;
        case 2: /* Get statistics */
            if (arg) {
                uint32_t* stats = (uint32_t*)arg;
                stats[0] = ne2000_dev.rx_packets;
                stats[1] = ne2000_dev.tx_packets;
                stats[2] = ne2000_dev.rx_errors;
                stats[3] = ne2000_dev.tx_errors;
            }
            return 1;
        default:
            return 0;
    }
}

/* Mark functions as used to avoid warnings */

/* NE2000 device registration function */
uint32_t ne2000_register_device(uint16_t base_port, uint16_t irq) {
    if (!ne2000_init(base_port, irq)) {
        return 0;
    }
    
    /* Device registration would be handled by the main kernel */
    /* For now, just initialize the hardware and return success */
    return 1;
}

/* NE2000 test functions */
uint32_t ne2000_test_loopback(void) {
    uint8_t test_packet[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* Destination MAC (broadcast) */
        0x52, 0x54, 0x00, 0x12, 0x34, 0x56, /* Source MAC */
        0x08, 0x00, /* Type: IP */
        0x45, 0x00, 0x00, 0x1C, 0x12, 0x34, 0x40, 0x00, 0x40, 0x01, /* IP header */
        0x0A, 0x00, 0x01, 0x01, /* Source IP */
        0x0A, 0x00, 0x01, 0x02, /* Dest IP */
        0x08, 0x00, 0xF7, 0xFF, /* ICMP header */
        0x12, 0x34, 0x56, 0x78 /* Test data */
    };
    
    uint32_t sent = ne2000_transmit(test_packet, sizeof(test_packet));
    if (sent != sizeof(test_packet)) {
        return 0;
    }
    
    /* Try to receive the packet (should be looped back) */
    uint8_t recv_buffer[64];
    uint32_t received = ne2000_receive(recv_buffer, sizeof(recv_buffer));
    
    return (received == sizeof(test_packet));
}

uint32_t ne2000_get_statistics(uint32_t* rx_packets, uint32_t* tx_packets, 
                               uint32_t* rx_errors, uint32_t* tx_errors) {
    if (rx_packets) *rx_packets = ne2000_dev.rx_packets;
    if (tx_packets) *tx_packets = ne2000_dev.tx_packets;
    if (rx_errors) *rx_errors = ne2000_dev.rx_errors;
    if (tx_errors) *tx_errors = ne2000_dev.tx_errors;
    return 1;
}

uint32_t ne2000_get_mac_address(uint8_t* mac) {
    if (mac) {
        for (int i = 0; i < 6; i++) {
            mac[i] = ne2000_dev.mac_address[i];
        }
    }
    return 1;
}