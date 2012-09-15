#!/bin/sh

# 17793793925
# 8816327xxxx

PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin

tail -1 /var/log/imu.0.log  | tr ':' ' ' | awk '{print "t:%3 lt:"$33" ln:"$35}' > /tmp/latlng
tail -1 /var/log/gps.0.log  | tr ':' ' ' | awk '{print "t:$3 lt:"$7" ln:"$9}' >> /tmp/latlng
tail -1 /var/log/helsman_st.0.log | tr ':' ' ' | awk '{print "tji:",$5,$7,$9}' > /tmp/helmsman.sts

for p in `cat /etc/smsphonenr`; do
    sms send $p  "$(head -1 /tmp/latlng) $(cat /tmp/helmsman.sts) $(date +"%m/%d %H:%M")"
done

