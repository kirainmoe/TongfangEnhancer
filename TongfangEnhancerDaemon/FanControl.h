#ifndef FanControl_h
#define FanControl_h

#include "common.h"
#include "message.h"

#import <pthread.h>

// lock
extern pthread_mutex_t configLock;

// prototypes
#ifdef __cplusplus
extern "C" {
#endif

int sendMessageToDriver(struct WMIActionMessage message);
void setFanMode(enum FanControlMode mode);
void setFanSpeed(int speedLevel);
void initFanControlConfig(void);
bool loadFanControlConfig(void);
void reloadFanControlConfig(void);
void adjustFanSpeed(double temp);
void* monitorCPUTemperature(unsigned int interval);

#ifdef __cplusplus
}
#endif


#endif /* FanControl_h */
