// Board A: I2C1 master
// SCL: D15 (PTE1), SDA: D14 (PTE0), ALT6
// Sends { 0xAB, counter } to slave 0x48 every ~100 ms
// Compile with: BOARD_MASTER

#include "MKL46Z4.h"
#include <stdint.h>

#define SLAVE_ADDR   0x48u
#define BUS_TIMEOUT  100000u

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

void i2c0_master_init(void)
{
    SIM->SCGC4 |= SIM_SCGC4_I2C1_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
    PORTE->PCR[1] = PORT_PCR_MUX(6u) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTE->PCR[0] = PORT_PCR_MUX(6u) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    I2C1->F  = 0x14u;
    I2C1->C1 = I2C_C1_IICEN_MASK;
}

// returns 0=ok, -1=timeout, -2=NACK
int i2c_master_send(uint8_t addr, const uint8_t *buf, uint8_t len)
{
    int ret = 0;

    uint32_t t = BUS_TIMEOUT;
    while ((I2C1->S & I2C_S_BUSY_MASK) && --t);
    if (t == 0u) return -1;

    I2C1->C1 |= (uint8_t)(I2C_C1_MST_MASK | I2C_C1_TX_MASK);
    I2C1->D = (uint8_t)((addr << 1u) | 0u);
    ret = i2c_wait_tcf_ack();
    if (ret != 0) goto generate_stop;

    for (uint8_t i = 0u; i < len; i++) {
        I2C1->D = buf[i];
        ret = i2c_wait_tcf_ack();
        if (ret != 0) goto generate_stop;
    }

generate_stop:
    I2C1->C1 &= (uint8_t)~(I2C_C1_MST_MASK | I2C_C1_TX_MASK);
    return ret;
}

#ifdef BOARD_MASTER
int main(void)
{
    i2c0_master_init();

    static const uint8_t msg[] = "Hello from Board A!";
    const uint8_t len = (uint8_t)(sizeof(msg) - 1u);

    for (;;) {
        i2c_master_send(SLAVE_ADDR, msg, len);
        for (volatile uint32_t d = 0u; d < 400000u; d++);
    }
}
#endif
