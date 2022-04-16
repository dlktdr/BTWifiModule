#if PCB==PICO
#define LEDPIN 4
#define UART_NUM UART_NUM_2
#define UART_TXPIN 4
#define UART_RXPIN 5

#elif PCB==C3MINI
#error "DEFINES.h not Configured"

#elif PCB==WROOM
#error "DEFINES.h not Configured"

#endif