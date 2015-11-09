#include <NFCModule.h>
#include <NFC.h>

#include <Node.h>

extern "C" {
#include "app_util_platform.h"
#include "app_uart.h"
#include "pn532.h"
}

#define UART_TX_BUF_SIZE 256
#define UART_RX_BUF_SIZE 1


bool UART_CONFIGURED = false;

void UART0_IRQHandler(void) {
    short attendeeId = NFC::GetAttendeeId();
}

void configure_uart(void)
{
//    const app_uart_comm_params_t uart_params {
//		20,
//		19,
//		RTS_PIN_NUMBER,
//		CTS_PIN_NUMBER,
//		false,
//		UART_BAUDRATE_BAUDRATE_Baud115200
//	};
//
//	uint8_t receiver[1] = {0};
//	uint8_t transmit[256] = {0};
//	const app_uart_buffers_t uart_buffers {
//		uart_buffers.rx_buf = receiver;
//		uart_buffers.rx_buf_size = 1;
//		uart_buffers.tx_buf = transmit;
//		uart_buffers.tx_buf_size = 256;
//	};
//
//
//    ret_code_t ret_code;
//    APP_UART_INIT(&uart_params, uart_buffers, UART0_IRQHandler, APP_IRQ_PRIORITY_LOW);
//    APP_ERROR_CHECK(ret_code);
    uart_115200_config(RTS_PIN_NUMBER, /*TX_PIN_NUM*/ 19, CTS_PIN_NUMBER, /*RX_PIN_NUM*/ 20);
    //NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Enabled << UART_INTENSET_RXDRDY_Pos;
    NVIC_SetPriority(UART0_IRQn, APP_IRQ_PRIORITY_LOW);
    NVIC_EnableIRQ(UART0_IRQn);
}

NFCModule::NFCModule(Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
    : Module(moduleID::NFC_MODULE_ID, node, cm, name, storageSlot)
{
  _configuration.moduleId = moduleID::NFC_MODULE_ID;
  _configuration.moduleVersion = 1;
  _configuration.moduleActive = true;
  configurationPointer = &_configuration;
}

void NFCModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
#ifdef ENABLE_NFC
    if (!UART_CONFIGURED) {
//        uart_115200_config(RTS_PIN_NUMBER, /*TX_PIN_NUM*/ 19, CTS_PIN_NUMBER, /*RX_PIN_NUM*/ 20);
        UART_CONFIGURED = true;
    }

    // every second...
    if (appTimer/1000 % 5 && appTimer % 1000 == 0) {
        // read current state of UART
        // schedule state change

//        unsigned short userId = NFC::GetAttendeeId();
//            if (userId != 0) {
//                vote(userId);
//            }
    }
#endif
}
