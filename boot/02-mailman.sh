#!/bin/sh

# Fix Mailman file permissions
/usr/local/mailman/bin/check_perms -f

# Remove stale unix sockets, pid files and locks
if [ -f "/data/mailman/data/master-qrunner.pid" ]; then
    rm -f /data/mailman/data/master-qrunner.pid
fi

rm -f /data/mailman/locks/*
