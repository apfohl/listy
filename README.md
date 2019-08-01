# Listy

```shell
docker volume create listy
```

```shell
docker run -d --name=listy -h server.example.com -p 80:80 -p 25:25 -v listy:/data -e LISTY_DOMAIN='lists.example.com' apfohl/listy
```
