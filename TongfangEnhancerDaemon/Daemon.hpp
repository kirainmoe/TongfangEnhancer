#ifndef DAEMON_H
#define DAEMON_H

#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <cstdio>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>

extern "C" {
#import <CoreServices/CoreServices.h>
#import <sys/kern_event.h>


#include "smc.h"
#include "common.h"
#include "message.h"
}

// fan controlling points
struct FanControlItem {
    int            speedLevel;
    double         tempLimit;
    FanControlMode mode;
    
    FanControlItem() {}
    FanControlItem(double _t, int _s, FanControlMode _m)
        : speedLevel(_s), tempLimit(_t), mode(_m) {}
    
    bool operator < (const FanControlItem &b) const {
        return tempLimit < b.tempLimit;
    }
};

static std::vector<FanControlItem> fanControlConfig;


// fan controlling config store path
const std::string FAN_CONTROL_CONFIG_PATH = "/usr/local/etc/fan.config";
const int BUF_MAXSIZE = 1024;


#endif
