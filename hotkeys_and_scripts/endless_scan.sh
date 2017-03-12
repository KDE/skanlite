#!/bin/bash

# Skanlite D-Bus example script
# this script sends 'scan' command to Skanlite via D-Bus
# then listens for scanDone signal and repeats in infinite loop

interface=org.kde.skanlite
sender=org.kde.skanlite
member=scanDone
command=scan


dbus-send --session --dest=$interface --type=method_call  / $interface.$command

dbus-monitor --profile "type='signal',sender='$sender',interface='$interface',member='$member'" --monitor |
while read -r line; do
dbus-send --session --dest=$interface --type=method_call  / $interface.$command
done
