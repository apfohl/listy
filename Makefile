.PHONY: build volume run shell destroy clean

build:
	@docker build -t apfohl/listy .

volume:
	@docker volume create listy

run: volume
	@docker run -d --name=listy -h lists.pfohl.email -p 80:80 -p 25:25 -v listy:/data apfohl/listy

shell:
	@docker exec -it listy sh

destroy:
	@docker rm -f listy

clean:
	@docker image prune -f
