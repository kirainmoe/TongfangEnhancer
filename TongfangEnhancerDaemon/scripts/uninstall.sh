#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

launchctl unload /Library/LaunchAgents/io.github.goshin.and.com.kirainmoe.TongfangEnhancerDaemon.plist > /dev/null 2>&1
sudo rm /Library/LaunchAgents/io.github.goshin.and.com.kirainmoe.TongfangEnhancerDaemon.plist > /dev/null 2>&1
sudo rm /usr/local/bin/TongfangEnhancerDaemon > /dev/null 2>&1
sudo rm /usr/local/bin/fancli > /dev/null 2>&1

