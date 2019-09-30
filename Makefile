.PHONY: build volume run shell destroy clean

build:
	@docker build -t apfohl/listy .

volume:
	@docker volume create listy

run: volume
	@docker run -d --name=listy -h server.pfohl.email -p 8080:80 -p 2525:25 -v listy:/data -e LISTY_DOMAIN='lists.pfohl.email' apfohl/listy

shell:
	@docker exec -it listy sh

destroy:
	@docker rm -f listy

clean:
	@docker image prune -f
