#pragma once

extern volatile bool btjoystickconnected;
extern uint16_t btj_conn_id;

#ifdef __cplusplus
extern "C" {
#endif

void BTJoyInit();

#ifdef __cplusplus
}
#endif


