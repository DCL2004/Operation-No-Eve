// Board C: I2C eavesdropper
// SCL: PTE1, SDA: PTE0, GPIO inputs
// Sniffs all I2C traffic by bit-banging and prints over UART0
// Compile with: BOARD_EAVESDROPPER

#include "MKL46Z4.h"
#include <stdint.h>

#define SCL_PIN  1u
#define SDA_PIN  0u

#define SCL_BIT  (1u << SCL_PIN)
#define SDA_BIT  (1u << SDA_PIN)

static inline uint32_t read_scl(void) { return PTE->PDIR & SCL_BIT; }
static inline uint32_t read_sda(void) { return PTE->PDIR & SDA_BIT; }

// UART0 helpers

void uart0_eaves_init(void)
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
    uart_putc(hex[v >> 4u]);
    uart_putc(hex[v & 0x0Fu]);
}

// GPIO-based I2C sniffer

void gpio_sniff_init(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
    PORTE->PCR[SDA_PIN] = PORT_PCR_MUX(1u);
    PORTE->PCR[SCL_PIN] = PORT_PCR_MUX(1u);
    PTE->PDDR &= ~(SDA_BIT | SCL_BIT);
}

// returns SDA at SCL rising edge, -1 for START, -2 for STOP
static int wait_scl_rise(uint32_t *sda_prev)
{
    uint32_t sda_last = *sda_prev;

    while (read_scl()) {
        uint32_t sda_now = read_sda();
        if (sda_now && !sda_last) { *sda_prev = sda_now; return -2; }
        if (!sda_now && sda_last) { *sda_prev = sda_now; return -1; }
        sda_last = sda_now;
    }

    while (!read_scl()) {
        sda_last = read_sda();
    }

    uint32_t sda_now = read_sda();
    *sda_prev = sda_now;
    return sda_now ? 1 : 0;
}

#ifdef BOARD_EAVESDROPPER

#define MAX_FRAME_BYTES  48u

static uint8_t frame_buf[MAX_FRAME_BYTES];
static uint8_t frame_ack[MAX_FRAME_BYTES];
static uint8_t frame_len;

static void print_frame(void)
{
    if (frame_len == 0u) return;
    if (frame_ack[0]) return;

    uart_puts("[START] ");

    for (uint8_t i = 0u; i < frame_len; i++) {
        uint8_t val = frame_buf[i];
        uint8_t ack = frame_ack[i];

        if (i == 0u) {
            uart_puts("ADDR=0x");
            uart_hex8(val >> 1u);
            uart_putc((val & 1u) ? 'R' : 'W');
        } else {
            uart_puts("0x");
            uart_hex8(val);
            if (val >= 0x20u && val <= 0x7Eu) {
                uart_putc('\'');
                uart_putc((char)val);
                uart_putc('\'');
            }
        }
        uart_putc(ack ? '-' : '+');
        uart_putc(' ');
    }

    uart_puts("[STOP]\r\n");
}

int main(void)
{
    uart0_eaves_init();
    gpio_sniff_init();

    uart_puts("Board C eavesdropper ready\r\n");

    uint32_t sda_prev = read_sda();
    uint8_t  byte_val = 0u;
    uint8_t  bit_count = 0u;

    for (;;) {
        while (1) {
            uint32_t scl = read_scl();
            uint32_t sda = read_sda();
            if (scl && !sda && sda_prev) break;
            sda_prev = sda;
        }

        frame_len = 0u;
        bit_count = 0u;
        byte_val  = 0u;
        sda_prev  = 0u;

        while (1) {
            int bit = wait_scl_rise(&sda_prev);

            if (bit == -1) {
                frame_len = 0u;
                bit_count = 0u;
                byte_val  = 0u;
                continue;
            }
            if (bit == -2) break;

            if (bit_count < 8u) {
                byte_val = (uint8_t)((byte_val << 1u) | (uint8_t)bit);
                bit_count++;
            } else {
                if (frame_len < MAX_FRAME_BYTES) {
                    frame_buf[frame_len] = byte_val;
                    frame_ack[frame_len] = (uint8_t)bit;
                    frame_len++;
                }
                bit_count = 0u;
                byte_val  = 0u;
            }
        }

        print_frame();
    }
}
#endif
