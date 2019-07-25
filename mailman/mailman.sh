#!/bin/sh
set -eu

pidfile="/data/mailman/data/master-qrunner.pid"
command="/usr/local/mailman/bin/mailmanctl -s start"

# Proxy signals
function kill_app(){
    kill $(cat $pidfile)
    exit 0 # exit okay
}
trap "kill_app" SIGINT SIGTERM

# Launch daemon
$command
sleep 2

# Loop while the pidfile and the process exist
while [ -f $pidfile ] && kill -0 $(cat $pidfile) ; do
    sleep 1
done
exit 1000 # exit unexpected
