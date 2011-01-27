cur=1; while [ "$cur" -lt "100" ]; do echo "$cur $cur"; cur=`expr $cur + 1`; sleep 1; done | telnet localhost 10000
