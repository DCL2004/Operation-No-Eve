// Board A: I2C1 master — text message sender
// SCL: D15 (PTE1), SDA: D14 (PTE0), ALT6
// Cycles through predefined messages sent to slave 0x48 every ~1 s
// Packet format: [0xAA][len][ascii bytes...]
// Compile with: BOARD_A_TEXT

#ifdef BOARD_A_TEXT

#include "MKL46Z4.h"
#include <stdint.h>
#include <stddef.h>

#define SLAVE_ADDR        0x48u
#define BUS_TIMEOUT       100000u
#define MSG_MARKER        0xAAu
#define SENDS_PER_MESSAGE 1u

#ifndef I2C_C1_MST_MASK
#define I2C_C1_MST_MASK   0x20u
#endif
#ifndef I2C_C1_TX_MASK
#define I2C_C1_TX_MASK    0x10u
#endif
#ifndef I2C_S_TCF_MASK
#define I2C_S_TCF_MASK    0x80u
#endif
#ifndef I2C_S_BUSY_MASK
#define I2C_S_BUSY_MASK   0x20u
#endif
#ifndef I2C_S_IICIF_MASK
#define I2C_S_IICIF_MASK  0x02u
#endif
#ifndef I2C_S_RXAK_MASK
#define I2C_S_RXAK_MASK   0x01u
#endif

// returns 0=ok, -1=timeout, -2=NACK
static int i2c_wait_tcf_ack(void)
{
    uint32_t t = BUS_TIMEOUT;
    while (!(I2C1->S & I2C_S_TCF_MASK) && --t);
    I2C1->S = I2C_S_IICIF_MASK;
    if (t == 0u)                    return -1;
    if (I2C1->S & I2C_S_RXAK_MASK) return -2;
    return 0;
}

static void i2c_init(void)
{
    SIM->SCGC4 |= SIM_SCGC4_I2C1_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
    PORTE->PCR[1] = PORT_PCR_MUX(6u) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTE->PCR[0] = PORT_PCR_MUX(6u) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    I2C1->F  = 0x94u;
    I2C1->C1 = I2C_C1_IICEN_MASK;
}

// returns 0=ok, negative=error
static int send_text(const char *msg, uint8_t len)
{
    int ret = 0;

    uint32_t t = BUS_TIMEOUT;
    while ((I2C1->S & I2C_S_BUSY_MASK) && --t);
    if (t == 0u) return -1;

    I2C1->C1 |= (uint8_t)(I2C_C1_MST_MASK | I2C_C1_TX_MASK);
    I2C1->D = (uint8_t)((SLAVE_ADDR << 1u) | 0u);
    ret = i2c_wait_tcf_ack();
    if (ret != 0) goto stop;

    I2C1->D = MSG_MARKER;
    ret = i2c_wait_tcf_ack();
    if (ret != 0) goto stop;

    I2C1->D = len;
    ret = i2c_wait_tcf_ack();
    if (ret != 0) goto stop;

    for (uint8_t i = 0u; i < len; i++) {
        I2C1->D = (uint8_t)msg[i];
        ret = i2c_wait_tcf_ack();
        if (ret != 0) goto stop;
    }

stop:
    I2C1->C1 &= (uint8_t)~(I2C_C1_MST_MASK | I2C_C1_TX_MASK);
    return ret;
}

static void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0u; i < ms; i++)
        for (volatile uint32_t d = 0u; d < 2000u; d++);
}

int main(void)
{
    i2c_init();

    static const char *messages[] = {
        "HELLO",
        "ECE3140",
        "PHASE 1",
        "I2C WORKS",
    };
    static const uint8_t lengths[] = { 5u, 7u, 7u, 9u };
    const uint8_t msg_count = (uint8_t)(sizeof(lengths) / sizeof(lengths[0]));

    uint8_t idx = 0u;

    for (;;) {
        for (uint8_t r = 0u; r < SENDS_PER_MESSAGE; r++) {
            send_text(messages[idx], lengths[idx]);
            delay_ms(250u);
        }
        idx = (uint8_t)((idx + 1u) % msg_count);
    }
}

#endif // BOARD_A_TEXT
