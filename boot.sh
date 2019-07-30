#!/bin/sh

# Initialization
INIT_TOKEN=/data/.initialized
if [ ! -f "$INIT_TOKEN" ]; then
    echo "Initializing..."

    cp -a /init/mailman /data

    touch $INIT_TOKEN

    echo "... DONE."
fi

# Fix Mailman file permissions
/usr/local/mailman/bin/check_perms -f

# Remove stale unix sockets, pid files and locks
if [ -S "/var/run/fcgiwrap.sock" ]; then
    rm -f /var/run/fcgiwrap.sock
fi

if [ -f "/var/run/rsyslogd.pid" ]; then
    rm -f /var/run/rsyslogd.pid
fi

rm -f /data/mailman/locks/*

# Start services
/usr/bin/supervisord -c /etc/supervisord.conf
