.PHONY: clean all default

default:
	make -C Origin default

all: default

run:
	./origin

clean:
	make -C Origin clean