FROM apfohl/service-base:latest

ARG MAILMAN_MINOR_VERSION=2.1
ARG MAILMAN_PATCH_VERSION=29
ARG MAILMAN_VERSION=$MAILMAN_MINOR_VERSION.$MAILMAN_PATCH_VERSION

# General
RUN apk update

# NGINX
RUN addgroup -S www-data && adduser -S www-data -G www-data
RUN apk add nginx
RUN rm /etc/nginx/conf.d/default.conf
COPY nginx/nginx.conf /etc/nginx/nginx.conf
COPY nginx/supervisord.conf /etc/supervisord.d/nginx.conf

# Postfix
RUN apk add postfix
RUN postalias /etc/postfix/aliases
COPY postfix/master.cf /etc/postfix/master.cf
COPY postfix/main.cf /etc/postfix/main.cf
COPY postfix/supervisord.conf /etc/supervisord.d/postfix.conf

# Foreground
RUN apk add alpine-sdk flex fcgi-dev libconfig-dev
COPY foreground /foreground
RUN make -C foreground
RUN mv /foreground/foreground /usr/local/bin/foreground
RUN rm -r /foreground

# JZON
ADD https://github.com/apfohl/jzon/archive/v1.0.0.tar.gz /
RUN tar xf v1.0.0.tar.gz
RUN make -C jzon-1.0.0 CFLAGS='-std=gnu11 -Os -Wall -Wextra -Wpedantic -Wstrict-overflow' install
RUN rm -r jzon-1.0.0 v1.0.0.tar.gz

# C HTML Template Library
ADD https://github.com/apfohl/ctemplate/archive/v1.0.0.tar.gz /
RUN tar xf v1.0.0.tar.gz
RUN make -C ctemplate-1.0.0 install
RUN rm -r ctemplate-1.0.0 v1.0.0.tar.gz

# Panel
COPY panel /panel
RUN make -C panel
RUN mv /panel/panel /usr/local/bin/panel
COPY panel/templates /usr/local/share/panel
RUN rm -r /panel
RUN echo 'template_directory = "/usr/local/share/panel";' >> /etc/panel.conf
RUN echo 'postqueue_command = "/usr/sbin/postqueue -j";' >> /etc/panel.conf
COPY panel/supervisord.conf /etc/supervisord.d/panel.conf
COPY panel/auth_nginx /etc/nginx/auth

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
COPY mailman/path.sh /etc/profile.d/path.sh
RUN mkdir -p /init
RUN mv /data/mailman /init

# FCGI Wrap
RUN apk add spawn-fcgi fcgiwrap
COPY fcgiwrap/supervisord.conf /etc/supervisord.d/fcgiwrap.conf

# Boot
COPY boot/* /boot.d/

# Network
EXPOSE 25/tcp 80/tcp
