#include <stdint.h>
#include "esproot.h"
#include "esptrainer.h"
#include "espjoystick.h"
#include "espaudio.h"
#include "terminal.h"
#include "espdefs.h"

#define LOG_ESPR "ESPROOT"

int rv;
#define STARTBLE_MODE(m,x) rv=x; writeAckNak(m,rv,""); if(rv==0) radioused=true;
#define STARTBTEDR_MODE(m,x) rv=x; writeAckNak(m,rv,""); if(rv==0) radioused=true;
#define STARTWIFI_MODE(m,x) rv=x; writeAckNak(m,rv,""); if(rv==0) radioused=true;
#define START_MODE(m,x) writeAckNak(m,x,"");

uint8_t runningModes;
bool radioused=false;

void espRootData(const uint8_t *data, uint8_t len) 
{
 
}

void espRootCommand(uint8_t command, const uint8_t *data, uint8_t len)
{
  ESP_LOGI(LOG_ESPR, "Root Command %d, Extra %d", command, data[0]);    
  uint8_t mode = data[0];
  switch(command) {

  // Startup Command
  case ESP_ROOTCMD_START_MODE:
    ESP_LOGI(LOG_ESPR, "Starting Mode %d", data[0]);    
    if(radioused && ((mode == ESP_TELEMETRY || 
                      mode == ESP_TRAINER || 
                      mode == ESP_JOYSTICK || 
                      mode == ESP_AUDIO || 
                      mode == ESP_FTP))) { 
      ESP_LOGE(LOG_ESPR, "Cannot start Radio already used");
      writeAckNak(mode,false, "Radio already used"); // Write error
    } else {
      switch (mode)
      {
      case ESP_TELEMETRY:
        //STARTRADIO_MODE(ESP_TELEMETRY, espTelemetryStart());
        break;
      case ESP_TRAINER:
        STARTBLE_MODE(ESP_TRAINER, espTrainerStart());
        break;
      case ESP_JOYSTICK:
        STARTBLE_MODE(ESP_JOYSTICK, espJoystickStart());
        break;
      case ESP_AUDIO:
        STARTBTEDR_MODE(ESP_AUDIO, espAudioStart());      
        break;
      case ESP_FTP:
        //STARTWIFI_MODE(ESP_FTP, espFTPStart());
        break;
      case ESP_IMU:      
        //START_MODE(ESP_IMU, espIMUStart());
        break;          
      default:
        break;
      }
    }
    break;

  case ESP_ROOTCMD_STOP_MODE:
    ESP_LOGI(LOG_ESPR, "Stopping Mode %d", data[0]);
    switch (mode) {
    case ESP_TELEMETRY:
   //   if(espTrainerRunning()) {
   //     espTrainerStop();
   //     radioused = false;
   //   }
      break;
    case ESP_TRAINER:
      if(espTrainerRunning()) {
        espTrainerStop();
        radioused = false;
      }
      break;
    case ESP_JOYSTICK:
      if(espJoystickRunning()) {
        espJoystickStop();
        radioused = false;
      }
      break;
    case ESP_AUDIO:
      if(espAudioRunning()) {
        espAudioStop();
        radioused = false;
      }
      break;
    case ESP_FTP:
  //    if(espTrainerRunning()) {
  //      espTrainerStop();
  //      radioused = false;
  //    }
      break;
    case ESP_IMU:      
  //    if(espIMURunning()) {
  //      espTIMUStop();
  //    }
      break;         

    default:
      break;
    }
    break;

  case ESP_ROOTCMD_RESTART:
    ESP_LOGI(LOG_ESPR, "Rebooting");
    esp_restart();
    break;
  case ESP_ROOTCMD_GET_VER:    
    break;
  }
}

