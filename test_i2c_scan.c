// Flash to Board A. Board B must be running BOARD_SLAVE.
// Scans I2C addresses 0x08-0x77 and prints results over UART0 at 9600 8N1.
// Expected output: "Found device at: 0x48" when wiring is correct.
// Compile with: TEST_I2C_SCAN

#ifdef TEST_I2C_SCAN

#include "MKL46Z4.h"
#include <stdint.h>

#define BUS_TIMEOUT 50000u

static void uart0_init(void)
{
    SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    SIM->SOPT2  = (SIM->SOPT2 & ~SIM_SOPT2_UART0SRC_MASK)
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
    static const char h[] = "0123456789ABCDEF";
    uart_putc('0'); uart_putc('x');
    uart_putc(h[v >> 4u]); uart_putc(h[v & 0x0Fu]);
}

static void uart_print_u8(uint8_t v)
{
    if (v >= 100u) uart_putc((char)('0' + v / 100u));
    if (v >=  10u) uart_putc((char)('0' + (v / 10u) % 10u));
    uart_putc((char)('0' + v % 10u));
}

static void i2c0_init(void)
{
    SIM->SCGC4 |= SIM_SCGC4_I2C1_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
    PORTE->PCR[1] = PORT_PCR_MUX(6u) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTE->PCR[0] = PORT_PCR_MUX(6u) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    I2C1->F  = 0x14u;
    I2C1->C1 = I2C_C1_IICEN_MASK;
}

// returns 1=ACK, 0=NACK/timeout
static int i2c_probe(uint8_t addr)
{
    int ack = 0;

    uint32_t t = BUS_TIMEOUT;
    while ((I2C1->S & I2C_S_BUSY_MASK) && --t);
    if (t == 0u) return 0;

    I2C1->C1 |= (uint8_t)(I2C_C1_MST_MASK | I2C_C1_TX_MASK);
    I2C1->D = (uint8_t)((addr << 1u) | 0u);

    t = BUS_TIMEOUT;
    while (!(I2C1->S & I2C_S_TCF_MASK) && --t);
    I2C1->S = I2C_S_IICIF_MASK;

    if (t != 0u && !(I2C1->S & I2C_S_RXAK_MASK)) ack = 1;

    I2C1->C1 &= (uint8_t)~(I2C_C1_MST_MASK | I2C_C1_TX_MASK);

    for (volatile uint32_t d = 0u; d < 2000u; d++);

    return ack;
}

int main(void)
{
    uart0_init();
    i2c0_init();

    for (;;) {
        uart_puts("\r\nI2C scan (0x08 - 0x77)...\r\n");

        uint8_t found = 0u;
        for (uint8_t addr = 0x08u; addr <= 0x77u; addr++) {
            if (i2c_probe(addr)) {
                uart_puts("  Found device at: ");
                uart_hex8(addr);
                uart_puts("\r\n");
                found++;
            }
        }

        uart_puts("Scan complete. ");
        uart_print_u8(found);
        uart_puts(" device(s).\r\n");

        for (volatile uint32_t d = 0u; d < 2400000u; d++);
    }
}

#endif
