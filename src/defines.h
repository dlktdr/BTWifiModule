#pragma once

#define DEBUG

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

#define UART_NUM UART_NUM_1
#define UART_TXPIN 18
#define UART_RXPIN 19
//#define LEDPIN 6

#else

#error "PCB NOT Configured"

#endif

#define UART_RX_BUFFER 512
#define UART_TX_BUFFER 512
#define BAUD_RESET_TIMER 1000000 // us
#define BAUD_DEFAULT 115200
#define BAUD_MAXIMUM 921600

// Optional Debugging Options
#define DEBUG_TIMERS

#define LEN_BLUETOOTH_NAME 10

#include <driver/uart.h>
extern const uart_port_t uart_num;

typedef enum {
  ROLE_UNKNOWN=0,
  ROLE_BLE_PERIPHERAL,
  ROLE_BLE_CENTRAL,
  ROLE_BLE_AUDIO,
  ROLE_ADVANCE,
  ROLE_COUNT
} role_t;

enum features {
  FEAT_BLUETOOTH,
  FEAT_ESPNOW,
};

#define PACKED __attribute__((packed))