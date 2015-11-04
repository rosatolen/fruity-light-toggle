#ifndef PN532_H
#define PN532_H

#ifdef __cplusplus
extern "C" {
#endif

/*lint ++flb "Enter library region" */

#include <stdbool.h>
#include <stdint.h>


void wakeup();
void powerdown();
unsigned short in_list_passive_target();
void uart_115200_config(uint8_t rts_pin_number, uint8_t txd_pin_number,
						uint8_t cts_pin_number, uint8_t rxd_pin_number);


#ifdef __cplusplus
}
#endif

/*lint --flb "Leave library region" */
#endif