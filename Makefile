IMAGE = apfohl/listy:development
CONTAINER = listy

RUNARGS = \
	-d \
	--name=$(CONTAINER) \
	-h server.pfohl.email \
	-p 8080:80 \
	-p 2525:25 \
	-v $(CONTAINER):/data \
	-e LISTY_DOMAIN="lists.pfohl.email" \
	-e TZ="Europe/Berlin"

.PHONY: build volume run shell destroy clean

build:
	@docker build -t $(IMAGE) .

volume:
	@docker volume create $(CONTAINER)

run: volume
	@docker run $(RUNARGS) $(IMAGE)

shell:
	@docker exec -it $(CONTAINER) sh

destroy:
	@docker rm -f $(CONTAINER)

clean:
	@docker image prune -f
