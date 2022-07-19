#include <driver/gpio.h>
#include <driver/uart.h>
#include <math.h>
#include <nvs.h>

#include "bt.h"
#include "bt_client/bt_client.h"
#include "bt_server/bt_server.h"
#include "cb.h"
#include "cobs.h"
#include "crc.h"
#include "defines.h"
#include "esp_log.h"
#include "frskybt.h"
#include "joystick/bt_joystick.h"
#include "settings.h"
#include "terminal.h"

#include "espaudio.h"
#include "espjoystick.h"
#include "esproot.h"
#include "esptrainer.h"
#include "freertos/stream_buffer.h"

#define LOG_UART "UART"

#define UART_RX_BUFFER 1024
#define REUSABLE_BUFFER 250
#define AT_CMD_MAX_LEN 40
#define BT_CMD_MAX_LEN 30

StreamBufferHandle_t uartrxstreamhndl;

volatile bool uartRXTaskStarted = false;

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

void sendBTMode() {}

// Ticks to wait for data stream to come in

uart_config_t uart_config = {
    .baud_rate = BAUD_DEFAULT,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
};

char atcommand[AT_CMD_MAX_LEN];
int atcommandlen = -1;

circular_buffer uartinbuf;

#define ESP_BASE 0
#define ESP_PACKET_TYPE_MSK 0x0F
#define ESP_PACKET_CMD_BIT 6
#define ESP_PACKET_ACK_BIT 7
#define ESP_PACKET_ISCMD(t) (t & (1 << ESP_PACKET_CMD_BIT))
#define ESP_PACKET_ISACKREQ(t) (t & (1 << ESP_PACKET_ACK_BIT))

void processPacket(const packet_s *packet) {
  // Acknowledge we got the command, so radio knows we are running
  if(ESP_PACKET_ISCMD(packet->type)) {
    writeAck();
  }

  switch (packet->type & ESP_PACKET_TYPE_MSK) {
  case ESP_ROOT:
    if (ESP_PACKET_ISCMD(packet->type))
      espRootCommand(packet->data[0], packet->data + 1, packet->len - 1);
    else
      espRootData(packet->data, packet->len);
    break;
  case ESP_TELEMETRY:
    break;
  case ESP_TRAINER:
    if (ESP_PACKET_ISCMD(packet->type))
      espTrainerCommand(packet->data[0], packet->data + 1, packet->len - 1);
    else
      espTrainerData(packet->data, packet->len);
    break;
  case ESP_JOYSTICK:
    if (ESP_PACKET_ISCMD(packet->type))
      espJoystickCommand(packet->data[0], packet->data + 1, packet->len - 1);
    else
      espJoystickData(packet->data, packet->len);
    break;
  case ESP_AUDIO:
    if (ESP_PACKET_ISCMD(packet->type))
      espAudioCommand(packet->data[0], packet->data + 1, packet->len - 1);
    else
      espAudioData(packet->data, packet->len);
    break;
  case ESP_FTP:
    break;
  case ESP_IMU:
    break;
  }
}

#define PACKED_BUFFERS 5

void mainTask(void *stuff) {
  ESP_LOGI(LOG_UART, "Waiting for settings to be read");
  while (!settings_ok) {
    vTaskDelay(50);
  };
  ESP_LOGI(LOG_UART, "Setting initial role");
  if (settings.mode == ESP_ROOT) {
    ESP_LOGE(LOG_UART, "No Role Loaded, Leaving off.. for now");
  }
  //  setRole(settings.role);

  ESP_LOGI(LOG_UART, "Waiting for UART RX Task to start");
  while (!uartRXTaskStarted) {
    vTaskDelay(20);
  }

  packet_s packet;
  uint8_t buffer[sizeof(packet_s) + 1];
  int bufferpos = 0;

  while (1) {
    uint8_t inb;
    int rec = xStreamBufferReceive(uartrxstreamhndl, &inb, 1, 10);
    if (rec) {
      if (inb == 0 && bufferpos != 0) {
        int lenout = cobs_decode(buffer, bufferpos, (uint8_t *)&packet);
        // ESP_LOG_BUFFER_HEX("P", (uint8_t *)&packet, lenout);
        uint16_t packetcrc =
            packet.crcl | (packet.crch << 8); // Store transmitted packet
        packet.crcl = 0xBB;
        packet.crch = 0xAA;
        uint16_t calccrc = crc16(0, (uint8_t *)&packet, lenout, 0);
        packet.len = lenout - PACKET_OVERHEAD;
        if (packetcrc == calccrc) {
          processPacket(&packet);
        } else {
          ESP_LOGE("PM", "CRC Fault");
        }
        bufferpos = 0;
      } else {
        buffer[bufferpos++] = inb;
        if (bufferpos == sizeof(buffer)) {
          printf("Buffer Overflow\r\n");
          bufferpos = 0;
        }
      }
    }

    // TEST SENDING PACKETS
    /*    static uint count=0;
        if(count++ % 20 == 0) {
          printf("Writting: Hello, Mode=2\r\n");
          writePacket("Hello",5,false,2);
          writeCommand(1,3,"DATA",4);
        }*/
  }
  vTaskDelete(NULL);
}

/*void setRole(role_t role)
{
  ESP_LOGI(LOG_UART,"Switching from mode %d to %d", curMode, role);
  if(role == curMode) return;

  if(role > 1)
    role = 0;
  // Shutdown
  switch(curMode) {
    case ROLE_BLE_CENTRAL:
    case ROLE_BLE_PERIPHERAL:
    default:
      break;
  }

  // Update Role
  curMode = role;
  settings.role = curMode;

  // Initialize
  switch(curMode) {
    case ROLE_BLE_CENTRAL:
      btCentralState = CENTRAL_STATE_DISCONNECT;
      BTInit();
      btcInit();
      break;
    case ROLE_BLE_PERIPHERAL:
      btPeripherialState = PERIPHERIAL_STATE_DISCONNECTED;
      BTInit();
      BTJoyInit();
      break;
    default:
      break;
  }

  // Save new role to flash
  saveSettings();
}*/

/*void runBTCentral()
{
  switch(btCentralState) {
    case CENTRAL_STATE_DISCONNECT: {
      // Stop scanning, disconnect from all periferials
      btc_disconnect();
      break;
    }
    case CENTRAL_STATE_SCAN_START:{
      laddcnt = 0;
      btc_start_scan();
      btCentralState = CENTRAL_STATE_SCANNING;
      break;
    }

    case CENTRAL_STATE_SCANNING:{
      // New item(s) added
      for(int i=laddcnt; i < bt_scanned_address_cnt; i++) {
        char addr[13];
        sprintf(reusablebuff, "OK+DISC:%s\r\n",btaddrtostr(addr,
btc_scanned_addresses[i]));
        //printf("%s",reusablebuff);
        uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
      }
      laddcnt = bt_scanned_address_cnt;
      if(btc_scan_complete) {
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
      //esp_bd_addr_t btaddr;
      //if(!readBTAddress(btaddr)) {
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
    case CENTRAL_STATE_WAITING_CONNECTION:{
      if(btc_scan_complete) {
        if(btc_validslavefound) {
          sprintf(reusablebuff, "Connected:%s\r\n", rmtaddress);
          uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
          sprintf(reusablebuff, "MTU Size:65\r\nMTU Size: 65\r\nPHT Update
Complete\r\nCurrent PHY:2M\r\n"); // Fix me uart_write_bytes(uart_num,
reusablebuff, strlen(reusablebuff));
//          sprintf(reusablebuff, "Board:%s\r\n",
str_ble_board_types[btc_board_type]);
//          uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
          btCentralState = CENTRAL_STATE_CONNECTED;
          // TODO: Add

        } else {
          btCentralState = CENTRAL_STATE_DISCONNECT;
        }
      }
      break;
    }

    case CENTRAL_STATE_CONNECTED:{
      if(!btc_connected) { // Connection Lost
        btCentralState = CENTRAL_STATE_CONNECT;
      }
    }
  }
}

void runBTPeripherial()
{
  switch(btPeripherialState) {
    case PERIPHERIAL_STATE_DISCONNECTED:
      if(btjoystickconnected) {
          // Save Remote Address
          btaddrtostr(rmtaddress, rmtbtaddress);
          sprintf(reusablebuff, "Connected:%s\r\n", rmtaddress);
          uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
          btPeripherialState = PERIPHERIAL_STATE_CONNECTED;
      }
      break;
    case PERIPHERIAL_STATE_CONNECTED:
      if(!btjoystickconnected) {
        btPeripherialState = PERIPHERIAL_STATE_DISCONNECTED;
        uart_write_bytes(uart_num, "DisConnected\r\nERROR\r\nERROR\r\n",28);
      }
      break;
  }
}*/

// Builds a packet
void writePacket(const uint8_t *dat, int len, bool iscmd, uint8_t mode) {
  uint8_t encodedbuffer[sizeof(packet_s) + 1];
  packet_s packet;
  packet.type = mode;
  if (iscmd)
    packet.type |= (1 << ESP_PACKET_CMD_BIT);
  packet.crcl = 0xBB;
  packet.crch = 0xAA;
  memcpy(packet.data, dat,
         len); // TODO, Remove me, extra copy for just the crc calc.
  uint16_t crc = crc16(0, (uint8_t *)&packet, len + PACKET_OVERHEAD, 0);
  packet.crcl = crc & 0xFF;
  packet.crch = (crc & 0xFF00) >> 8;

  int wl =
      cobs_encode((uint8_t *)&packet, len + PACKET_OVERHEAD, encodedbuffer);
  encodedbuffer[wl] = '\0';
  uart_write_bytes(uart_num, (void *)&encodedbuffer, wl + 1);
}

// Sends some data
void writeData(uint8_t mode, const uint8_t *dat, int len) {
  writePacket(dat, len, false, mode);
}

// Send a command
void writeCommand(uint8_t mode, uint8_t command, const uint8_t *dat, int len) {
  uint8_t buffer[256];
  buffer[0] = command; // First byte of a command is the command, data follows
  memcpy(buffer + 1, dat, len);
  writePacket(buffer, len + 1, true, mode);
}

// Writes an acknowledge / not-acknowledge and a optional message
void writeAck() {
  writeCommand(ESP_ROOT, 1 << ESP_PACKET_ACK_BIT, NULL, 0);
}

// Read from the UART RX, write to the stream if data available, this task
// should be high Priority
void uartRXTask(void *n) {
  // Setup UART Port
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(uart_driver_install(uart_num, UART_RX_BUFFER * 4,
                                      UART_RX_BUFFER * 4, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_TXPIN, UART_RXPIN,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  uartrxstreamhndl = xStreamBufferCreate(UART_RX_BUFFER * 2, 1);
  if (uartrxstreamhndl == NULL) {
    ESP_LOGE("UARTRX", "NOT ENOUGH HEAP!");
  }
  // TODO - MAKE ME DMA
  char *data = (char *)malloc(UART_RX_BUFFER);
  uartRXTaskStarted = true;
  while (1) {
    int cnt = uart_read_bytes(uart_num, data, UART_RX_BUFFER, 1);
    if (cnt) {
      int len = xStreamBufferSend(uartrxstreamhndl, data, cnt, 0);
      if (cnt != len) {
        printf("Unable to fill the stream, %d written - Dropping all data\r\n",
               len);
        while (xStreamBufferReset(uartrxstreamhndl) == pdFAIL) {
          vTaskDelay(1);
        }
      }
    }
  }
  free(data);
}