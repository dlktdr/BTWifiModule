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

#include <driver/uart.h>
extern const uart_port_t uart_num;