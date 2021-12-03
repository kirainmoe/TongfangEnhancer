#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

# remove old daemon and conflict daemon
echo "Removing old and conflict daemons..."
launchctl unload /Library/LaunchAgents/io.github.goshin.and.com.kirainmoe.TongfangEnhancerDaemon.plist > /dev/null 2>&1
sudo rm /Library/LaunchAgents/io.github.goshin.and.com.kirainmoe.TongfangEnhancerDaemon.plist > /dev/null 2>&1
sudo rm /usr/bin/TongfangEnhancerDaemon > /dev/null 2>&1

launchctl unload /Library/LaunchAgents/io.github.goshin.HotkeyDaemon.plist > /dev/null 2>&1
sudo rm /Library/LaunchAgents/io.github.goshin.HotkeyDaemon.plist > /dev/null 2>&1
sudo rm /usr/bin/HotkeyDaemon > /dev/null 2>&1

launchctl unload /Library/LaunchAgents/io.github.goshin.TongfangFnDaemon.plist > /dev/null 2>&1
sudo rm /Library/LaunchAgents/io.github.goshin.TongfangFnDaemon.plist > /dev/null 2>&1
sudo rm /usr/bin/TongfangFnDaemon > /dev/null 2>&1

# copy daemon
echo "Copy executable files..."
sudo mkdir -p /usr/local/bin/
sudo chmod -R 755 /usr/local/bin/
sudo cp $DIR/TongfangEnhancerDaemon /usr/local/bin/
sudo chmod 755 /usr/local/bin/TongfangEnhancerDaemon
sudo chown root:wheel /usr/local/bin/TongfangEnhancerDaemon

sudo cp $DIR/io.github.goshin.and.com.kirainmoe.TongfangEnhancerDaemon.plist /Library/LaunchAgents
sudo chmod 644 /Library/LaunchAgents/io.github.goshin.and.com.kirainmoe.TongfangEnhancerDaemon.plist
sudo chown root:wheel /Library/LaunchAgents/io.github.goshin.and.com.kirainmoe.TongfangEnhancerDaemon.plist

sudo launchctl load /Library/LaunchAgents/io.github.goshin.and.com.kirainmoe.TongfangEnhancerDaemon.plist

# copy fancli
sudo cp $DIR/fancli /usr/local/bin/

echo "Done."
