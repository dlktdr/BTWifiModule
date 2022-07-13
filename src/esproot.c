#include <stdint.h>
#include "esproot.h"
#include "esptrainer.h"
#include "espjoystick.h"
#include "espaudio.h"
#include "terminal.h"
#include "espdefs.h"

#define LOG_ESPR "ESPROOT"

// TODO, Replace me with git SHA
espversion espVersion = {1,0,0,"GITTAG"};

int g_rv;
bool g_radioIsBTEDR=false;
bool g_radioIsBLE=false;
bool g_radioIsWIFI=false;

#define STARTBLE_MODE(m,x) g_rv=x; writeAckNak(m,g_rv,""); if(g_rv==0) g_radioIsBLE=true;
#define STARTBTEDR_MODE(m,x) g_rv=x; writeAckNak(m,g_rv,""); if(g_rv==0) g_radioIsBTEDR=true;
#define STARTWIFI_MODE(m,x) g_rv=x; writeAckNak(m,g_rv,""); if(g_rv==0) g_radioIsWIFI=true;
#define START_MODE(m,x) writeAckNak(m,x,"");
#define RADIO_USED() (g_radioIsBTEDR || g_radioIsBLE || g_radioIsWIFI)

uint8_t runningModes;

void espRootData(const uint8_t *data, uint8_t len) 
{
 
}

void espRootCommand(uint8_t command, const uint8_t *data, uint8_t len)
{
  ESP_LOGI(LOG_ESPR, "Root Command %d", command, data[0]);    

  switch(command) {
  case ESP_ROOTCMD_START_MODE:
    if(len != 1) break;
    ESP_LOGI(LOG_ESPR, "Starting Mode %d", data[0]);    
    if(RADIO_USED() && ((data[0] == ESP_TELEMETRY || 
                         data[0] == ESP_TRAINER || 
                         data[0] == ESP_JOYSTICK || 
                         data[0] == ESP_AUDIO || 
                         data[0] == ESP_FTP))) { 
      ESP_LOGE(LOG_ESPR, "Cannot start Radio already used");
      writeAckNak(mode,false, "Radio already used"); 
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
    if(len != 1) break;
    ESP_LOGI(LOG_ESPR, "Stopping Mode %d", data[0]);
    switch (data[0]) {
    case ESP_TELEMETRY:
   //   if(espTelemetryRunning()) {
   //     espTelemetryStop();
   //  g_radioIsBTEDR = false; // WHAT TO DO, I COULD SEE BOTH.. 
   //   probably EDR with a serial port tho
   //   }
      break;
    case ESP_TRAINER:
      if(espTrainerRunning()) {
        espTrainerStop();
        g_radioIsBLE = false;
      }
      break;
    case ESP_JOYSTICK:
      if(espJoystickRunning()) {
        espJoystickStop();
        g_radioIsBLE = false;
      }
      break;
    case ESP_AUDIO:
      if(espAudioRunning()) {
        espAudioStop();
        g_radioIsBTEDR = false;
      }
      break;
    case ESP_FTP:
  //    if(espFTPRunning()) {
  //      espFTPStop();
  //      radioUsed = false;
  //    }
      break;
    case ESP_IMU:      
  //    if(espFTPRunning()) {
  //      espFTPStop();
  //    }
      break;         

    default:
      break;
    }
    break;

  case ESP_ROOTCMD_RESTART:
    ESP_LOGI(LOG_ESPR, "Rebooting...");
    esp_restart();
    break;
  case ESP_ROOTCMD_VERSION:    
    writeCommand(ESP_ROOT,
                 ESP_ROOTCMD_VERSION,
                 (const uint8_t *)&espVersion,
                 sizeof(espversion));
    break;
  case ESP_ROOTCMD_CON_EVENT: // Shouldn't be much here, events are generated here
    if(len < 1) break;
    ESP_LOGI(LOG_ESPR, "Con Mgr. Event %d", data[0]);
    connectionEventRX(data[0], data + 1, len -1); 
    break;  
  case ESP_ROOTCMD_CON_MGMNT:
    if(len < 1) break;
    ESP_LOGI(LOG_ESPR, "Con Mgr. Cmd %d", data[0]);
    connectionCommandRX(data[0], data + 1, len -1); 
    break;
  }

}


