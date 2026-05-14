// Board B: I2C1 slave + UART0 display
// SCL: D15 (PTE1), SDA: D14 (PTE0), ALT6
// Receives bytes from master at address 0x48, prints hex over UART0
// Compile with: BOARD_SLAVE

#include "MKL46Z4.h"
#include <stdint.h>

#define MY_ADDR       0x48u
#define BUS_TIMEOUT   100000u

#ifndef I2C_C1_TX_MASK
#define I2C_C1_TX_MASK    0x10u
#endif
#ifndef I2C_C1_TXAK_MASK
#define I2C_C1_TXAK_MASK  0x08u
#endif
#ifndef I2C_S_IAAS_MASK
#define I2C_S_IAAS_MASK   0x40u
#endif
#ifndef I2C_S_BUSY_MASK
#define I2C_S_BUSY_MASK   0x20u
#endif
#ifndef I2C_S_SRW_MASK
#define I2C_S_SRW_MASK    0x04u
#endif
#ifndef I2C_S_IICIF_MASK
#define I2C_S_IICIF_MASK  0x02u
#endif

// baud = 20971520 / (16 * 136) ≈ 9600
void uart0_slave_init(void)
{
    SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_UART0SRC_MASK)
               | SIM_SOPT2_UART0SRC(1u);
    UART0->C2  = 0u;
    UART0->BDH = 0u;
    UART0->BDL = 136u;
    UART0->C4  = 0x0Fu;
    UART0->C1  = 0u;
    PORTA->PCR[1] = PORT_PCR_MUX(2u);
    PORTA->PCR[2] = PORT_PCR_MUX(2u);
    UART0->C2 = UART_C2_TE_MASK | UART_C2_RE_MASK;
}

static void uart_putc(char c)
{
    while (!(UART0->S1 & UART_S1_TDRE_MASK));
    UART0->D = (uint8_t)c;
}

static void uart_puts(const char *s) { while (*s) uart_putc(*s++); }

static void uart_hex8(uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    uart_putc('0'); uart_putc('x');
    uart_putc(hex[v >> 4u]);
    uart_putc(hex[v & 0x0Fu]);
}

void i2c0_slave_init(void)
{
    SIM->SCGC4 |= SIM_SCGC4_I2C1_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
    PORTE->PCR[1] = PORT_PCR_MUX(6u) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTE->PCR[0] = PORT_PCR_MUX(6u) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    I2C1->A1 = (uint8_t)(MY_ADDR << 1u);
    I2C1->F  = 0x14u;
    I2C1->C1 = I2C_C1_IICEN_MASK;
}

// blocks until addressed; returns number of bytes received
uint8_t i2c_slave_receive(uint8_t *buf, uint8_t max_len)
{
    uint8_t count = 0u;

    uint32_t t = BUS_TIMEOUT * 20u;
    while (!(I2C1->S & I2C_S_IAAS_MASK) && --t);
    if (t == 0u) return 0u;

    if (I2C1->S & I2C_S_SRW_MASK) {
        I2C1->C1 |= (uint8_t)I2C_C1_TX_MASK;
        I2C1->D   = 0xFFu;
        I2C1->S   = I2C_S_IICIF_MASK;
        return 0u;
    }

    I2C1->C1 &= (uint8_t)~I2C_C1_TX_MASK;
    (void)I2C1->D;               // dummy read releases SCL
    I2C1->S   =  I2C_S_IICIF_MASK;

    while (I2C1->S & I2C_S_BUSY_MASK) {
        t = BUS_TIMEOUT;
        while (!(I2C1->S & I2C_S_IICIF_MASK) &&
                (I2C1->S & I2C_S_BUSY_MASK) && --t);

        if (!(I2C1->S & I2C_S_IICIF_MASK)) break;

        I2C1->S = I2C_S_IICIF_MASK;
        uint8_t byte = I2C1->D;
        // KL46Z leaks the address byte as the first received byte; discard it
        if (count == 0u && byte == (uint8_t)(MY_ADDR << 1u)) continue;
        if (count < max_len) buf[count++] = byte;
    }

    return count;
}

#ifdef BOARD_SLAVE
int main(void)
{
    uart0_slave_init();
    i2c0_slave_init();

    uart_puts("Board B ready — waiting for I2C data on address 0x48\r\n");

    uint8_t rx_buf[16];

    for (;;) {
        uint8_t n = i2c_slave_receive(rx_buf, (uint8_t)sizeof(rx_buf));

        if (n == 0u) {
            uart_puts("(no data / timeout)\r\n");
            continue;
        }

        uart_puts("RX [");
        if (n >= 10u) uart_putc((char)('0' + n / 10u));
        uart_putc((char)('0' + n % 10u));
        uart_puts("]:");

        for (uint8_t i = 0u; i < n; i++) {
            uart_putc(' ');
            uart_hex8(rx_buf[i]);
        }
        uart_puts("\r\n");
    }
}
#endif
