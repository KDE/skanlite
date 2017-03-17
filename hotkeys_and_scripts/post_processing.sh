#!/bin/bash

# Skanlite D-Bus example script.
# this script listens for D-Bus imageSaved signal from Skanlite then
# postprocesses scanned image with help of gimp application,
# prints it and remove file. (the last two commands are disabled by default)

# gimp provides much more powerful tools for image processing
# in particulary for color balance correction
# in this example we decreasing Highlights (brightest pixels) gamma's red by 18

interface=org.kde.skanlite
sender=org.kde.skanlite
member=imageSaved

# listen for imageSaved D-Bus events,
# each time we enter the loop, we just got an event

dbus-monitor --profile "type='signal',sender='$sender',interface='$interface',member='$member'" --monitor |
while read -r line; do

if [[ $line == *"member=imageSaved"* ]]; then
   read -r line;

   if [[ $line == *"string \""*"\"" ]]; then

      [[ $line =~ string[[:space:]]\"(.*)\" ]]

      # Here you can apply some additional filters. For example filter out
      # grayscale scans with help of identify (from imagemagick package)
      # like, identify -verbose $filename | grep 'Type: TrueColor'

      filename=${BASH_REMATCH[1]}
      printf "Processing scan: $filename\n"
      gimp -i -b '(let* ((filename "'$filename'")
                         (image (car (gimp-file-load RUN-NONINTERACTIVE
                                                          filename filename)))
                         (drawable (car (gimp-image-get-active-layer image))))
                        (gimp-color-balance
                                                         drawable 2 1 -18 0 0) ; Change these parameters to adjust color balanse
                        (gimp-file-save RUN-NONINTERACTIVE
                                             image drawable filename filename)
                        (gimp-image-delete image))' \
              -b '(gimp-quit 0)'

       #printf "Printing: $filename\n"
       #lp $filename # Send to default printer. Configure this command if needed

       #rm $filename # Remove file
    fi
  fi
done
