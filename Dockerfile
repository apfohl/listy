FROM alpine:3.10

RUN apk update
RUN apk add bash vim

# Mailman
RUN apk add wget python2 python2-dev py2-pip alpine-sdk bash vim
RUN addgroup -S mailman && adduser -S mailman -G mailman
RUN pip install dnspython
RUN wget https://launchpad.net/mailman/2.1/2.1.29/+download/mailman-2.1.29.tgz
RUN tar xf mailman-2.1.29.tgz
RUN mkdir -p /usr/local/mailman
RUN chown root:mailman /usr/local/mailman
RUN chmod g+s /usr/local/mailman
RUN chmod 02775 /usr/local/mailman
RUN cd mailman-2.1.29 && ./configure && make install
RUN cd .. && rm -r mailman-2.1.29 mailman-2.1.29.tgz

# Postfix
RUN apk add postfix

# Supervisor
RUN apk add supervisor

# Entrypoint
ENTRYPOINT ["supervisord", "-c", "/etc/supervisord.conf", "-n", "-u", "root"]
