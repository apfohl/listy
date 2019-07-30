FROM alpine:latest

ARG MAILMAN_MINOR_VERSION=2.1
ARG MAILMAN_PATCH_VERSION=29
ARG MAILMAN_VERSION=$MAILMAN_MINOR_VERSION.$MAILMAN_PATCH_VERSION

RUN apk update
RUN apk add vim

# General
RUN mkdir -p /data/logs /init

# Supervisor
RUN apk add supervisor
RUN mkdir -p /etc/supervisord.d
COPY supervisord/supervisord.conf /etc/supervisord.conf

# Rsyslogd
RUN apk add rsyslog
COPY rsyslogd/rsyslog.conf /etc/rsyslog.conf
COPY rsyslogd/supervisord.conf /etc/supervisord.d/rsyslogd.conf

# NGINX
RUN addgroup -S www-data && adduser -S www-data -G www-data
RUN apk add nginx
RUN rm /etc/nginx/conf.d/default.conf
COPY nginx/nginx.conf /etc/nginx/nginx.conf
COPY nginx/supervisord.conf /etc/supervisord.d/nginx.conf

# PostSRSd
RUN apk add postsrsd
RUN mkdir -p /var/lib/postsrsd
RUN dd if=/dev/urandom bs=18 count=1 | base64 > /etc/postsrsd.secret
COPY postsrsd/supervisord.conf /etc/supervisord.d/postsrsd.conf

# Postfix
RUN apk add postfix
RUN postalias /etc/postfix/aliases
COPY postfix/master.cf /etc/postfix/master.cf
COPY postfix/main.cf /etc/postfix/main.cf
COPY postfix/supervisord.conf /etc/supervisord.d/postfix.conf

# Foreground
RUN apk add alpine-sdk
COPY foreground /foreground
RUN make -C foreground
RUN mv /foreground/foreground /usr/local/bin/foreground
RUN rm -r /foreground

# Mailman
RUN apk add python2 python2-dev py2-pip
RUN addgroup -S mailman && adduser -S mailman -G mailman
RUN pip install dnspython
ADD https://launchpad.net/mailman/$MAILMAN_MINOR_VERSION/$MAILMAN_VERSION/+download/mailman-$MAILMAN_VERSION.tgz /
RUN tar xf mailman-$MAILMAN_VERSION.tgz
RUN mkdir -p /usr/local/mailman /data/mailman
RUN chown root:mailman /usr/local/mailman /data/mailman
RUN chmod g+s /usr/local/mailman /data/mailman
RUN chmod 02775 /usr/local/mailman /data/mailman
RUN cd mailman-$MAILMAN_VERSION && ./configure --with-var-prefix=/data/mailman && make install
RUN /usr/local/mailman/bin/check_perms -f
RUN rm -r mailman-$MAILMAN_VERSION mailman-$MAILMAN_VERSION.tgz
COPY mailman/mm_cfg.py /usr/local/mailman/Mailman/mm_cfg.py
COPY mailman/nginx.conf /etc/nginx/conf.d/mailman.conf
COPY mailman/supervisord.conf /etc/supervisord.d/mailman.conf
RUN mv /data/mailman /init

# FCGI Wrap
RUN apk add spawn-fcgi fcgiwrap
COPY fcgiwrap/supervisord.conf /etc/supervisord.d/fcgiwrap.conf

# Network
EXPOSE 25/tcp 80/tcp

# Entrypoint
COPY boot.sh /boot.sh
ENTRYPOINT ["sh", "boot.sh"]
