.PHONY: clean all default

default:
	make -C Origin default

all: default

run:
	./originc

clean:
	make -C Origin clean