#include <driver/gpio.h>
#include <driver/uart.h>
#include <math.h>

#include "btterminal.h"
#include "frskybt.h"
#include "cb.h"
#include "bt.h"
#include "defines.h"

void runBT();

void parserBTData(const char btdata[], int len)
{
  printf("BT DATA: ");
  for(int i=0; i < len; i++) {
    printf("%x ", btdata[i]);
  }
  printf("\n");
}

const uart_port_t uart_num = UART_NUM;

#define UART_WRITE_STRING(x,y) uart_write_bytes(x, y, sizeof(y)-1)

typedef enum {
  BT_MODE_PERIPHERAL,
  BT_MODE_CENTRAL
} btmode;

typedef enum {
  CENTRAL_STATE_DISCONNECT,
  CENTRAL_STATE_IDLE,
  CENTRAL_STATE_SCAN_START,
  CENTRAL_STATE_SCANNING,
  CENTRAL_STATE_SCAN_COMPLETE,
  CENTRAL_STATE_CONNECT,
  CENTRAL_STATE_WAITING_CONNECTION,
  CENTRAL_STATE_CONNECTED,
} btcentralstate;

btmode curMode = BT_MODE_CENTRAL;
btcentralstate btCentralState = CENTRAL_STATE_DISCONNECT;

int laddcnt = 0;
char lcladdress[13] = "806FB0BF6629";
char rmtaddress[13] = "000000000000";
char reusablebuff[200];

inline void sendBTMode()
{
  if(curMode == BT_MODE_PERIPHERAL) {
    snprintf(reusablebuff, sizeof(reusablebuff), "Peripheral:%s\r\n", lcladdress);
    uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
  } else if(curMode == BT_MODE_CENTRAL) {
    snprintf(reusablebuff, sizeof(reusablebuff), "Central:%s\r\n", lcladdress);
    uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
  }
}

void parserATCommand(char atcommand[])
{
  // Strip trailing whitespace
  bool done=false;
  while(!done) {
    int len = strlen(atcommand);
    if(len > 0 && (atcommand[len-1] == '\n' || atcommand[len-1] == '\r'))
      atcommand[len-1] = '\0';
    else
      done = true;
  }

  if(strncmp(atcommand, "+ROLE0", 6) == 0) {
    printf("Setting role as Peripheral\n");
    curMode = BT_MODE_PERIPHERAL;
    btCentralState = CENTRAL_STATE_DISCONNECT;
    UART_WRITE_STRING(uart_num, "OK+Role:0\r\n");
    sendBTMode();

  } else if (strncmp(atcommand, "+ROLE1", 6) == 0) {
    printf("Setting role as Central\n");
    curMode = BT_MODE_CENTRAL;
    btCentralState = CENTRAL_STATE_DISCONNECT;
    UART_WRITE_STRING(uart_num, "OK+Role:1\r\n");
    sendBTMode();
    // Should auto start discovery here too

  } else if (strncmp(atcommand, "+CON", 4) == 0) {
    if(curMode == BT_MODE_CENTRAL) {
      // Connect to device specified
      snprintf(reusablebuff, sizeof(reusablebuff), "OK+CONNA\r\nConnecting to:%s\r\n", atcommand + 4);
      uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
      // Store Remote Address to Connect to
      strcpy(rmtaddress, atcommand + 4);
      // Start connection
      btCentralState = CENTRAL_STATE_CONNECT;
    } else {
      UART_WRITE_STRING(uart_num, "ERROR");
    }

  } else if (strncmp(atcommand, "+NAME", 5) == 0) {
    printf("Setting Name to %s\n", atcommand + 5);
    snprintf(reusablebuff, sizeof(reusablebuff), "OK+Name:%s\r\n", atcommand +5);
    uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
    sendBTMode();

  } else if (strncmp(atcommand, "+TXPW", 5) == 0) {
    printf("Setting Power to %s\n", atcommand + 5);
    UART_WRITE_STRING(uart_num, "OK+Txpw:0\r\n");
    sendBTMode();

  } else if (strncmp(atcommand, "+DISC?", 6) == 0) {
    if(curMode == BT_MODE_CENTRAL) {
      printf("Discovery Requested\n");
      UART_WRITE_STRING(uart_num, "OK+DISCS\r\n");
      laddcnt = 0;
      if(btCentralState != CENTRAL_STATE_SCAN_START && 
         btCentralState != CENTRAL_STATE_SCANNING)
        btCentralState = CENTRAL_STATE_SCAN_START;
    }

  } else if (strncmp(atcommand, "+CLEAR", 6) == 0) {
    if(curMode == BT_MODE_CENTRAL) {
      btCentralState = CENTRAL_STATE_DISCONNECT;
      UART_WRITE_STRING(uart_num, "OK+CLEAR\r\n");
    }

  } else if (strncmp(atcommand, "+HTRESET", 8) == 0) {
    if(bt_board_type == BLE_BOARD_HEADTRACKER) {
      printf("Restting Head Tracker Board\r\n");
      UART_WRITE_STRING(uart_num, "OK+HTRESET\r\n");
      bt_dohtreset();
    } else {
      UART_WRITE_STRING(uart_num, "ERROR");
    }

  } else {
    printf("Unknown AT Cmd: %s\n", atcommand);
  }
}

// Ticks to wait for data stream to come in
#define UART_DELAY 10
#define DEBUG

void runUARTHead() {

  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 122,
  };

  // Configure UART parameters
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

  ESP_ERROR_CHECK(
      uart_set_pin(uart_num, UART_TXPIN, UART_RXPIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  // Setup UART buffered IO with event queue
  const int uart_buffer_size = (1024 * 2);
  QueueHandle_t uart_queue;

  // Install UART driver using an event queue here
  ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size,
                                      uart_buffer_size, 10, &uart_queue, 0));

  circular_buffer uartinbuf;

  cb_init(&uartinbuf, 200);

  char atcommand[40];
  int atcommandlen=-1;

  char btcommand[40];
  int btcommandlen=-1;

  while (1) {
    char buffer[50];
    int cnt = uart_read_bytes(uart_num, buffer, sizeof(buffer), 0);
    for (int i = 0; i < cnt; i++)
      cb_push_back(&uartinbuf, &buffer[i]);

    char c;
    while (!cb_pop_front(&uartinbuf, &c)) {
      if (atcommandlen >= 0) {
        atcommand[atcommandlen++] = c;
        // Check for buffer overflow
        if(atcommandlen == sizeof(atcommand)-1) {
          printf("AT Command Buffer Overflow\n");
          atcommandlen = -1;
          continue;
        }
        // AT Command Termination
        if(c == '\n') {
          atcommand[atcommandlen] = '\0';
          parserATCommand(atcommand);
          atcommandlen = -1;
        }
      } else if (btcommandlen >= 0) {
        btcommand[btcommandlen++] = c;
        // Check for buffer overflow
        if(btcommandlen == sizeof(btcommand)) {
          printf("BT Data Buffer Overflow\n");
          btcommandlen = -1;
          continue;
        }
        // BT Command Termination
        if(c == START_STOP) {
          parserBTData(btcommand, btcommandlen);
          btcommandlen = -1;
        }
      } else {
        static char lc=0;
        if(lc == 'A' && c == 'T') {
          atcommandlen = 0;
        }
        else if(c == START_STOP) {
          btcommand[0] = START_STOP; // Be sure to include start stop in stream
          btcommandlen = 1;
        }
        lc = c;
      }
    }

    runBT();
    vTaskDelay(1);
  }
}

// Handle Scanning/Connecting and Sending Data
void runBT() {
    if(curMode == BT_MODE_CENTRAL) {
        switch(btCentralState) {
        case CENTRAL_STATE_DISCONNECT: {
            // Stop scanning, disconnect from all periferials
            bt_disconnect();
            break;
        }
        case CENTRAL_STATE_SCAN_START:{
            laddcnt = 0;
            bt_start_scan();
            btCentralState = CENTRAL_STATE_SCANNING;
            break;
        }

        case CENTRAL_STATE_SCANNING:{
            // New item(s) added
            for(int i=laddcnt; i < bt_scanned_address_cnt; i++) {
              char addr[13];
              sprintf(reusablebuff, "OK+DISC:%s\r\n",btaddrtostr(addr, bt_scanned_addresses[i]));
              //printf("%s",reusablebuff);
              uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
            }
            laddcnt = bt_scanned_address_cnt;
            if(bt_scan_complete) {
                btCentralState = CENTRAL_STATE_SCAN_COMPLETE;
            }
            break;
        }
        case CENTRAL_STATE_SCAN_COMPLETE: {
            UART_WRITE_STRING(uart_num, "OK+DISCE\r\n");
            btCentralState = CENTRAL_STATE_IDLE;
            break;
        }
        case CENTRAL_STATE_IDLE: {
            // Do Nothing
            break;
        }

        // Connection was requested
        case CENTRAL_STATE_CONNECT: {
            esp_bd_addr_t addr;
            strtobtaddr(addr, rmtaddress);
            bt_connect(addr);
            btCentralState = CENTRAL_STATE_WAITING_CONNECTION;
            break;
        }
        case CENTRAL_STATE_WAITING_CONNECTION:{
          if(bt_scan_complete) {
            if(bt_validslavefound) {
              sprintf(reusablebuff, "Connected:%s\r\n", rmtaddress);
              uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
              sprintf(reusablebuff, "MTU Size:65\r\nMTU Size: 65\r\nPHT Update Complete\r\nCurrent PHY:2M\r\n"); //Fixme
              uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
              sprintf(reusablebuff, "Board:%s\r\n", str_ble_board_types[bt_board_type]);
              uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));           
              btCentralState = CENTRAL_STATE_CONNECTED;
              // TODO: Add 

            } else {
              btCentralState = CENTRAL_STATE_DISCONNECT;
            }
          }
          break;
        }

        case CENTRAL_STATE_CONNECTED:{
          if(!bt_connected) { // Connection Lost
            btCentralState = CENTRAL_STATE_CONNECT;
          }
        }
        }
    }
}