#include "terminal.h"

#include <driver/gpio.h>
#include <driver/uart.h>
#include <math.h>
#include <nvs.h>

#include "bt.h"
#include "bt_client.h"
#include "bt_server.h"
#include "cb.h"
#include "defines.h"
#include "esp_log.h"
#include "frskybt.h"
#include "settings.h"

#define LOG_UART "UART"

#define UART_RX_BUFFER 512
#define REUSABLE_BUFFER 250
#define AT_CMD_MAX_LEN 40
#define BT_CMD_MAX_LEN 30

void runBT();
void setBaudRate(uint32_t baudRate);
void setRole(role_t role);

const uart_port_t uart_num = UART_NUM;

#define UART_WRITE_STRING(x, y) uart_write_bytes(x, y, sizeof(y) - 1)

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

typedef enum {
  PERIPHERIAL_STATE_DISCONNECTED,
  PERIPHERIAL_STATE_CONNECTED,
} btperipheralstate;

role_t curMode = ROLE_UNKNOWN;
btcentralstate btCentralState = CENTRAL_STATE_DISCONNECT;
btperipheralstate btPeripherialState = PERIPHERIAL_STATE_DISCONNECTED;
int64_t baudTimer = 0;
int laddcnt = 0;

char rmtaddress[13] = "000000000000";
char reusablebuff[REUSABLE_BUFFER];

void sendBTMode()
{
  char lcladdress[13] = "000000000000";
  btaddrtostr(lcladdress, localbtaddress);
  ESP_LOGI(LOG_UART, "Local Addr %s", lcladdress);
  if (curMode == ROLE_BLE_PERIPHERAL) {
    snprintf(reusablebuff, sizeof(reusablebuff), "Peripheral:%s\r\n", lcladdress);
    uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
  } else if (curMode == ROLE_BLE_CENTRAL) {
    snprintf(reusablebuff, sizeof(reusablebuff), "Central:%s\r\n", lcladdress);
    uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
  }
}

void parserATCommand(char atcommand[])
{
  // Strip trailing whitespace
  bool done = false;
  while (!done) {
    int len = strlen(atcommand);
    if (len > 0 && (atcommand[len - 1] == '\n' || atcommand[len - 1] == '\r'))
      atcommand[len - 1] = '\0';
    else
      done = true;
  }

  if (strncmp(atcommand, "+ROLE0", 6) == 0) {
    ESP_LOGI(LOG_UART, "Setting role as Peripheral");
    UART_WRITE_STRING(uart_num, "OK+Role:0\r\n");
    setRole(ROLE_BLE_PERIPHERAL);
    sendBTMode();

  } else if (strncmp(atcommand, "+ROLE1", 6) == 0) {
    ESP_LOGI(LOG_UART, "Setting role as Central");
    UART_WRITE_STRING(uart_num, "OK+Role:1\r\n");
    setRole(ROLE_BLE_CENTRAL);
    sendBTMode();

  } else if (strncmp(atcommand, "+CON", 4) == 0) {
    if (curMode == ROLE_BLE_CENTRAL) {
      // Connect to device specified
      snprintf(reusablebuff, sizeof(reusablebuff), "OK+CONNA\r\nConnecting to:%s\r\n",
               atcommand + 4);
      uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
      // Store Remote Address to Connect to
      strcpy(rmtaddress, atcommand + 4);
      // Start connection
      btCentralState = CENTRAL_STATE_CONNECT;
    } else {
      UART_WRITE_STRING(uart_num, "ERROR");
    }

  } else if (strncmp(atcommand, "+NAME", 5) == 0) {
    ESP_LOGI(LOG_UART, "Setting Name to %s", atcommand + 5);
    btSetName(atcommand + 5);
    snprintf(reusablebuff, sizeof(reusablebuff), "OK+Name:%s\r\n", atcommand + 5);
    uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
    sendBTMode();

  } else if (strncmp(atcommand, "+TXPW", 5) == 0) {
    ESP_LOGI(LOG_UART, "Setting Power to %s", atcommand + 5);
    UART_WRITE_STRING(uart_num, "OK+Txpw:0\r\n");
    sendBTMode();

  } else if (strncmp(atcommand, "+DISC?", 6) == 0) {
    if (curMode == ROLE_BLE_CENTRAL) {
      ESP_LOGI(LOG_UART, "Discovery Requested");
      UART_WRITE_STRING(uart_num, "OK+DISCS\r\n");
      laddcnt = 0;
      if (btCentralState != CENTRAL_STATE_SCAN_START && btCentralState != CENTRAL_STATE_SCANNING)
        btCentralState = CENTRAL_STATE_SCAN_START;
    }

  } else if (strncmp(atcommand, "+CLEAR", 6) == 0) {
    if (curMode == ROLE_BLE_CENTRAL) {
      btCentralState = CENTRAL_STATE_DISCONNECT;
      UART_WRITE_STRING(uart_num, "OK+CLEAR\r\n");
    }

  } else if (strncmp(atcommand, "+BAUD", 5) == 0) {
    strncpy(reusablebuff, &atcommand[6], sizeof(reusablebuff));
    int baudrate = atoi(reusablebuff);
    ESP_LOGI(LOG_UART, "Baud Rate Change Requested to %d", baudrate);

    baudTimer = esp_timer_get_time() + BAUD_RESET_TIMER;
    // setBaudRate(baudrate); TODO
    // UART_WRITE_STRING(uart_num, "OK+BAUD\r\n");

    // TO DO: We need to check if the new baud worked. If the above timer elapses
    // before an AT+ACK or something is seen. Then revert back to the default
    // baud

  } else if (strncmp(atcommand, "+HTRESET", 8) == 0) {
    if (btc_board_type == BLE_BOARD_HEADTRACKER) {
      ESP_LOGI(LOG_UART, "Reset Head Tracker Requested");
      UART_WRITE_STRING(uart_num, "OK+HTRESET\r\n");
      btc_dohtreset();
    } else {
      UART_WRITE_STRING(uart_num, "ERROR");
    }

  } else {
    ESP_LOGE(LOG_UART, "Unknown AT Cmd: %s", atcommand);
  }
}

// Ticks to wait for data stream to come in

uart_config_t uart_config = {
    .baud_rate = BAUD_DEFAULT,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
};

void setBaudRate(uint32_t baudRate)
{
  if (baudRate < BAUD_DEFAULT || baudRate > BAUD_MAXIMUM) return;
  uart_config.baud_rate = baudRate;
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
}

char atcommand[AT_CMD_MAX_LEN];
int atcommandlen = -1;

circular_buffer uartinbuf;

void runUARTHead()
{
  // Setup UART Port
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(
      uart_driver_install(uart_num, UART_RX_BUFFER * 2, UART_RX_BUFFER * 2, 0, NULL, 0));
  ESP_ERROR_CHECK(
      uart_set_pin(uart_num, UART_TXPIN, UART_RXPIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  cb_init(&uartinbuf, UART_RX_BUFFER * 2);

  ESP_LOGI(LOG_UART, "Waiting for settings to be read");
  while (!settings_ok) {
    vTaskDelay(50);
  };  // Pause until settings are read
  ESP_LOGI(LOG_UART, "Setting initial role");
  if (settings.role == ROLE_UNKNOWN) {
    ESP_LOGE(LOG_UART, "Invalid role loaded, defaulting to central");
    settings.role = ROLE_BLE_CENTRAL;
  }
  setRole(settings.role);

  char* data = (char*)malloc(UART_RX_BUFFER + 1);
  if (data == NULL) {
    ESP_LOGE(LOG_UART, "No Memory!!!!!\nHALT");
    for (;;) {
    }
  }

  while (1) {
    int cnt = uart_read_bytes(uart_num, data, UART_RX_BUFFER, 0);
    for (int i = 0; i < cnt; i++) cb_push_back(&uartinbuf, &data[i]);

    char c;
    while (!cb_pop_front(&uartinbuf, &c)) {
      if (atcommandlen >= 0) {
        atcommand[atcommandlen++] = c;
        // Check for buffer overflow
        if (atcommandlen >= sizeof(atcommand) - 1) {
          ESP_LOGE(LOG_UART, "AT Command Buffer Overflow");
          atcommandlen = -1;
          continue;
        }
        // AT Command Termination
        if (c == '\n') {
          atcommand[atcommandlen] = '\0';
          parserATCommand(atcommand);
          atcommandlen = -1;
        }
      } else {
        // Scan for characters AT in the byte stream
        static char lc = 0;
        if (lc == 'A' && c == 'T') {
          atcommandlen = 0;
        } else {
          frSkyProcessByte(c);
        }

        lc = c;
      }
    }

    runBT();
    vTaskDelay(1);
  }
  free(data);
  vTaskDelete(NULL);
}

void setRole(role_t role)
{
  ESP_LOGI(LOG_UART, "Switching from mode %d to %d", curMode, role);
  if (role == curMode) return;

  // Shutdown
  switch (curMode) {
    case ROLE_BLE_CENTRAL:
    case ROLE_BLE_PERIPHERAL:
      bt_disable();
    default:
      break;
  }

  // Update Role
  curMode = role;
  settings.role = curMode;

  // Initialize
  switch (curMode) {
    case ROLE_BLE_CENTRAL:
      btCentralState = CENTRAL_STATE_DISCONNECT;
      bt_init();
      btcInit();
      break;
    case ROLE_BLE_PERIPHERAL:
      btPeripherialState = PERIPHERIAL_STATE_DISCONNECTED;
      bt_init();
      btpInit();
      break;
    default:
      break;
  }

  // Save new role to flash
  saveSettings();
}

void runBTCentral()
{
  switch (btCentralState) {
    case CENTRAL_STATE_DISCONNECT: {
      // Stop scanning, disconnect from all periferials
      btc_disconnect();
      break;
    }
    case CENTRAL_STATE_SCAN_START: {
      laddcnt = 0;
      btc_start_scan();
      btCentralState = CENTRAL_STATE_SCANNING;
      break;
    }

    case CENTRAL_STATE_SCANNING: {
      // New item(s) added
      for (int i = laddcnt; i < bt_scanned_address_cnt; i++) {
        char addr[13];
        sprintf(reusablebuff, "OK+DISC:%s\r\n", btaddrtostr(addr, btc_scanned_addresses[i].addr));
        // printf("%s",reusablebuff);
        uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
      }
      laddcnt = bt_scanned_address_cnt;
      if (btc_scan_complete) {
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
      // TODO Automatically try to connect to the last known bluetooth address
      // esp_bd_addr_t btaddr;
      // if(!readBTAddress(btaddr)) {
      //  btCentralState = CENTRAL_STATE_CONNECT;
      //}
      // Do Nothing
      break;
    }

    // Connection was requested
    case CENTRAL_STATE_CONNECT: {
      esp_bd_addr_t addr;
      strtobtaddr(addr, rmtaddress);
      btc_connect(addr);
      btCentralState = CENTRAL_STATE_WAITING_CONNECTION;
      break;
    }
    case CENTRAL_STATE_WAITING_CONNECTION: {
      if (btc_scan_complete) {
        if (btc_validslavefound) {
          sprintf(reusablebuff, "Connected:%s\r\n", rmtaddress);
          uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
          sprintf(
              reusablebuff,
              "MTU Size:65\r\nMTU Size: 65\r\nPHT Update Complete\r\nCurrent PHY:2M\r\n");  // Fix
                                                                                            // me
          uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
          sprintf(reusablebuff, "Board:%s\r\n", str_ble_board_types[btc_board_type]);
          uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
          btCentralState = CENTRAL_STATE_CONNECTED;
          // TODO: Add

        } else {
          btCentralState = CENTRAL_STATE_DISCONNECT;
        }
      }
      break;
    }

    case CENTRAL_STATE_CONNECTED: {
      if (!btc_connected) {  // Connection Lost
        btCentralState = CENTRAL_STATE_CONNECT;
      }
    }
  }
}

void runBTPeripherial()
{
  switch (btPeripherialState) {
    case PERIPHERIAL_STATE_DISCONNECTED:
      if (btp_connected) {
        // Save Remote Address
        btaddrtostr(rmtaddress, rmtbtaddress);
        sprintf(reusablebuff, "Connected:%s\r\n", rmtaddress);
        uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
        btPeripherialState = PERIPHERIAL_STATE_CONNECTED;
      }
      break;
    case PERIPHERIAL_STATE_CONNECTED:
      if (!btp_connected) {
        btPeripherialState = PERIPHERIAL_STATE_DISCONNECTED;
        uart_write_bytes(uart_num, "DisConnected\r\nERROR\r\nERROR\r\n", 28);
      }
      break;
  }
}

void runBT()
{
  switch (curMode) {
    case ROLE_BLE_CENTRAL:
      runBTCentral();
      break;
    case ROLE_BLE_PERIPHERAL:
      runBTPeripherial();
      break;
    /*case ROLE_BTEDR_AUDIO_SOURCE:
      break;
    case ROLE_ESPNOW_CENTRAL:
      break;
    case ROLE_ESPNOW_PERIPHERAL:
      break;*/
    default:
      break;
  }
}