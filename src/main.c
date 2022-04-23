/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
 *
 * This demo showcases creating a GATT database using a predefined attribute
 *table. It acts as a GATT server and can send adv data, be connected by client.
 * Run the gatt_client demo, the client demo will automatically connect to the
 *gatt_server_service_table demo. Client demo will enable GATT server's notify
 *after connection. The two devices will then exchange data.
 *
 ****************************************************************************/

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "btterminal.h"
#include "nvs_flash.h"
#include "bt.h"
//#include "wifi/wifi.h"
#include "defines.h"

#if defined(LEDPIN)
void runBlinky() {
  gpio_set_direction(LEDPIN, GPIO_MODE_DEF_OUTPUT);
  while (1) {
    gpio_set_level(LEDPIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LEDPIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
#endif

void app_main(void) {

  TaskHandle_t tUartHnd = NULL;
  xTaskCreate(runUARTHead, "Uart2", 8192, NULL, tskIDLE_PRIORITY + 1, &tUartHnd);
  configASSERT(tUartHnd);

#if defined(LEDPIN)
  TaskHandle_t tBlinkHnd = NULL;
  xTaskCreate(runBlinky, "Blinky", 1024, NULL, tskIDLE_PRIORITY, &tBlinkHnd);
  configASSERT(tBlinkHnd);
#endif

  esp_err_t ret;

  /* Initialize NVS. */
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  bt_init();

  //wifi_init_softap();
}