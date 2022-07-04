#include <driver/gpio.h>
#include <driver/uart.h>
#include <math.h>
#include <nvs.h>

#include "esp_log.h"
#include "terminal.h"
#include "frskybt.h"
#include "cb.h"
#include "bt.h"
#include "bt_client.h"
#include "bt_server.h"
#include "defines.h"
#include "settings.h"
#include "cobs.h"
#include "crc.h"

#include "esproot.h"
#include "esptrainer.h"

#define LOG_UART "UART"

#define UART_RX_BUFFER 512
#define REUSABLE_BUFFER 250
#define AT_CMD_MAX_LEN 40
#define BT_CMD_MAX_LEN 30

void runBT();
void setBaudRate(uint32_t baudRate);
void setRole(role_t role);

const uart_port_t uart_num = UART_NUM;

#define UART_WRITE_STRING(x,y) uart_write_bytes(x, y, sizeof(y)-1)

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
int64_t baudTimer =0;
int laddcnt = 0;

char rmtaddress[13] = "000000000000";
char reusablebuff[REUSABLE_BUFFER];

void sendBTMode()
{

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


char atcommand[AT_CMD_MAX_LEN];
int atcommandlen=-1;

circular_buffer uartinbuf;

enum ESPModes {
  ESP_ROOT,
  ESP_TELEMETRY,
  ESP_TRAINER,
  ESP_JOYSTICK,
  ESP_AUDIO,
  ESP_FTP,
  ESP_IMU,
  ESP_MAX
};

#define ESP_BASE 0
#define ESP_PACKET_TYPE_MSK 0x0F
#define ESP_PACKET_CMD_BIT 6
#define ESP_PACKET_ACK_BIT 7
#define ESP_PACKET_ISCMD(t) (t&(1<<ESP_PACKET_CMD_BIT))
#define ESP_PACKET_ISACKREQ(t) (t&(1<<ESP_PACKET_ACK_BIT))

void processPacket(const packet_s *packet)
{
  switch(packet->type & ESP_PACKET_TYPE_MSK) {
    case ESP_ROOT:
      if(ESP_PACKET_ISCMD(packet->type))
        espRootCommand(packet->data[0], packet->data + 1, packet->len -1);
      else
        espRootData(packet->data, packet->len);
      break;
    case ESP_TELEMETRY:
      break;
    case ESP_TRAINER:
      if(ESP_PACKET_ISCMD(packet->type))
        espTrainerCommand(packet->data[0], packet->data + 1, packet->len -1);
      else
        espTrainerData(packet->data, packet->len);
      break;
      break;
    case ESP_JOYSTICK:
      break;
    case ESP_AUDIO:
      break;
    case ESP_FTP:
      break;
    case ESP_IMU:
      break;
  }
}

#define PACKED_BUFFERS 5

void runUARTHead(void *stuff) {
  // Setup UART Port
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(uart_driver_install(uart_num, UART_RX_BUFFER * 4,
                                      UART_RX_BUFFER * 4, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_TXPIN,
                                         UART_RXPIN,
                                         UART_PIN_NO_CHANGE,
                                         UART_PIN_NO_CHANGE));

  cb_init(&uartinbuf, UART_RX_BUFFER*2);

  ESP_LOGI(LOG_UART, "Waiting for settings to be read");
  while(!settings_ok) {vTaskDelay(50);}; // Pause until settings are read
  ESP_LOGI(LOG_UART, "Setting initial role");
  if(settings.role == ROLE_UNKNOWN) {
    ESP_LOGE(LOG_UART, "Invalid role loaded, defaulting to central");
    settings.role = ROLE_BLE_CENTRAL;
  }
  setRole(settings.role);

  char* data = (char*) malloc(UART_RX_BUFFER+1);
  if(data == NULL) {
    ESP_LOGE(LOG_UART, "No Memory!!!!!\nHALT");
    for(;;) {}
  }

  packet_s packet;
  while (1) {
    uint8_t encodedbuffer[UART_RX_BUFFER];
    int cnt = uart_read_bytes(uart_num, data, UART_RX_BUFFER, 0);

    for (int i = 0; i < cnt; i++) {
      if(!cb_push_back(&uartinbuf, data + i)) {
        ESP_LOGE("UartRX", "Circular buffer full");
      }
      if(data[i] == 0) {
        char c;
        char *ptr = (char*)encodedbuffer;
        int len=0;
        while (!cb_pop_front(&uartinbuf, &c)) {
          *ptr++ = c;
          len++;
          if(len == sizeof(encodedbuffer)) {
            ESP_LOGE("UartRX", "Buffer overflow");
            break;
          }
        }
        cb_clear(&uartinbuf); // Clear the buffer

//        ESP_LOG_BUFFER_HEX("EBL", encodedbuffer, len);
        int lenout = cobs_decode(encodedbuffer,len,(uint8_t *)&packet);
//        ESP_LOG_BUFFER_HEX("P", (uint8_t *)&packet, lenout);
        uint16_t packetcrc = packet.crcl | (packet.crch << 8); // Store transmitted packet
        packet.crcl = 0xBB;
        packet.crch = 0xAA;
        uint16_t calccrc = crc16(0,(uint8_t *)&packet,lenout - 1, 0);
        packet.len = lenout - PACKET_OVERHEAD - 1;
//        printf("in %d out %d data %d\r\n", len, lenout, packet.len);
        if(packetcrc == calccrc) {
          // Successful packet, parse it
          processPacket(&packet);
        } else {
          ESP_LOGE("PM", "CRC Fault");
        }

      }
    }
    //uart_write_bytes(uart_num,"Hello\n",7);

//    runBT();
    vTaskDelay(1);
  }
  free(data);
  vTaskDelete(NULL);
}

void setRole(role_t role)
{
  ESP_LOGI(LOG_UART,"Switching from mode %d to %d", curMode, role);
  if(role == curMode) return;

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
        sprintf(reusablebuff, "OK+DISC:%s\r\n",btaddrtostr(addr, btc_scanned_addresses[i]));
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
          sprintf(reusablebuff, "MTU Size:65\r\nMTU Size: 65\r\nPHT Update Complete\r\nCurrent PHY:2M\r\n"); // Fix me
          uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
//          sprintf(reusablebuff, "Board:%s\r\n", str_ble_board_types[btc_board_type]);
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
      if(btp_connected) {
          // Save Remote Address
          btaddrtostr(rmtaddress, rmtbtaddress);
          sprintf(reusablebuff, "Connected:%s\r\n", rmtaddress);
          uart_write_bytes(uart_num, reusablebuff, strlen(reusablebuff));
          btPeripherialState = PERIPHERIAL_STATE_CONNECTED;
      }
      break;
    case PERIPHERIAL_STATE_CONNECTED:
      if(!btp_connected) {
        btPeripherialState = PERIPHERIAL_STATE_DISCONNECTED;
        uart_write_bytes(uart_num, "DisConnected\r\nERROR\r\nERROR\r\n",28);
      }
      break;
  }
}

void runBT()
{
  switch(curMode) {
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

void write(const uint8_t *dat, int len, bool iscmd, int id)
{
  uint8_t encodedbuffer[sizeof(packet_s) + 1];
  packet_s packet;
  packet.type = id;
  packet.type |= (iscmd << ESP_PACKET_CMD_BIT);
  packet.crcl = 0xBB;
  packet.crch = 0xAA;
  memcpy(packet.data, dat, len); // TODO, Remove me, extra copy for just the crc calc.
  uint16_t crc = crc16(0, (uint8_t *)&packet,len + PACKET_OVERHEAD, 0);
  packet.crcl = crc & 0xFF;
  packet.crch = (crc & 0xFF00) >> 8;
  int wl = cobs_encode((uint8_t *)&packet, packet.len + PACKET_OVERHEAD, encodedbuffer);

  encodedbuffer[wl] = '\0'; // Null terminate packet, used for detection of packet end

  // Write the packet
  uart_write_bytes(uart_num, (uint8_t *)&packet, wl+1);
}