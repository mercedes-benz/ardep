target extended-remote /dev/ttyBmpGdb

monitor auto_scan

attach 1

load

break main

continue
