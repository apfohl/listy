#!/bin/sh

# Initialization
INIT_TOKEN=/data/.initialized
if [ ! -f "$INIT_TOKEN" ]; then
    echo "Initializing..."

    cp -a /init/mailman /data

    HOSTNAME=`hostname -f`
    DOMAIN=`hostname -d`

    LISTY_DOMAIN="${LISTY_DOMAIN:-$HOSTNAME}"

    sed -i "s/lists.example.com/$LISTY_DOMAIN/g" /usr/local/mailman/Mailman/mm_cfg.py
    sed -i "s/lists.example.com/$LISTY_DOMAIN/g" /etc/supervisord.d/postsrsd.conf
    sed -i "s/example.com/$DOMAIN/g" /etc/supervisord.d/postsrsd.conf

    postconf -e "mydestination = \$myhostname, localhost.\$mydomain, localhost, $LISTY_DOMAIN"

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

if [ -f "/data/mailman/data/master-qrunner.pid" ]; then
    rm -f /data/mailman/data/master-qrunner.pid
fi

rm -f /data/mailman/locks/*

# Start services
/usr/bin/supervisord -c /etc/supervisord.conf
