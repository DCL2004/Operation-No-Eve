// Flash to either board. Blinks green LED (PTD5, active-low) once per second.
// Compile with: TEST_LED_BLINK

#ifdef TEST_LED_BLINK

#include "MKL46Z4.h"
#include <stdint.h>

int main(void)
{
    SIM->SCGC5    |= SIM_SCGC5_PORTD_MASK;
    PORTD->PCR[5]  = PORT_PCR_MUX(1u);
    PTD->PDDR     |= (1u << 5);
    PTD->PSOR      = (1u << 5);

    for (;;) {
        PTD->PTOR = (1u << 5);
        for (volatile uint32_t i = 0u; i < 400000u; i++);
    }
}

#endif
