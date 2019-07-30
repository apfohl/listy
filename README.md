# Listy

```shell
docker volume create listy
```

```shell
docker run -d --name=listy -h lists.pfohl.email -p 80:80 -p 25:25 -v listy:/data apfohl/listy
```
