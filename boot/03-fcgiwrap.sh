#!/bin/sh

# Remove stale unix sockets, pid files and locks
if [ -S "/var/run/fcgiwrap.sock" ]; then
    rm -f /var/run/fcgiwrap.sock
fi
