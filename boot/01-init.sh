#!/bin/sh

HOSTNAME=`hostname -f`
DOMAIN=`hostname -d`

LISTY_DOMAIN="${LISTY_DOMAIN:-$HOSTNAME}"

# Initialization
INIT_TOKEN=/data/.initialized
if [ ! -f "$INIT_TOKEN" ]; then
    echo "Initializing..."

    mv /init/mailman /data
    rm -r /init

    sed -i "s/lists.example.com/$LISTY_DOMAIN/g" /usr/local/mailman/Mailman/mm_cfg.py

    touch $INIT_TOKEN

    echo "... DONE."
fi

postconf -e "mydestination = \$myhostname, localhost.\$mydomain, localhost, $LISTY_DOMAIN"
