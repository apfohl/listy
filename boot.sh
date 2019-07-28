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

# Remove stale FCGI Wrap socket
if [ -S "/var/run/fcgiwrap.sock" ]; then
    rm -f /var/run/fcgiwrap.sock
fi

# Start services
/usr/bin/supervisord -c /etc/supervisord.conf
