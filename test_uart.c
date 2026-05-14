// Flash to Board B. Prints "UART OK: N" every ~1 s at 9600 8N1.
// Compile with: TEST_UART

#ifdef TEST_UART

#include "MKL46Z4.h"
#include <stdint.h>

// baud = 20971520 / (16 * 136) ≈ 9600
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

static void uart_print_u32(uint32_t v)
{
    char buf[11];
    uint8_t i = 10u;
    buf[i] = '\0';
    if (v == 0u) { buf[--i] = '0'; }
    else { while (v) { buf[--i] = (char)('0' + v % 10u); v /= 10u; } }
    uart_puts(&buf[i]);
}

int main(void)
{
    uart0_init();
    uart_puts("Board B UART test\r\n");

    uint32_t count = 1u;
    for (;;) {
        uart_puts("UART OK: ");
        uart_print_u32(count++);
        uart_puts("\r\n");
        for (volatile uint32_t d = 0u; d < 800000u; d++);
    }
}

#endif
