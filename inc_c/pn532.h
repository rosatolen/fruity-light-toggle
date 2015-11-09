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
void uart_put_char(uint8_t character);
void gobble_number_of_bytes(int number);
void send_preamble_and_start();
void send_direction();
void send_postamble();
uint8_t uart_get();
bool uart_get_with_timeout();
bool tag_is_present();
void get_ack();
void in_data_exchange(uint8_t address, uint8_t other);

void retarget_init();


#ifdef __cplusplus
}
#endif

/*lint --flb "Leave library region" */
#endif
