#pragma once

#if defined(PCB_PICO)
//#define LEDPIN 6
#define UART_NUM UART_NUM_2
#define UART_TXPIN 4
#define UART_RXPIN 5

#elif defined(PCB_WROOM)

#define UART_NUM UART_NUM_2
#define UART_TXPIN 18
#define UART_RXPIN 19

#elif defined(PCB_C3MINI)

#define UART_NUM UART_NUM_2
#define UART_TXPIN 18
#define UART_RXPIN 19
//#define LEDPIN 6

#else

#error "PCB NOT Configured"

#endif

#define BAUD_RESET_TIMER 1000000 // us
#define BAUD_DEFAULT 115200
#define BAUD_MAXIMUM 1000000

#include <driver/uart.h>
extern const uart_port_t uart_num;

typedef enum {
  ROLE_UNKNOWN,
  ROLE_BLE_PERIPHERAL,
  ROLE_BLE_CENTRAL,
  ROLE_ESPNOW_PERIPHERAL,
  ROLE_ESPNOW_CENTRAL,
  ROLE_BTEDR_AUDIO_SOURCE,
  ROLE_COUNT
} role_t;