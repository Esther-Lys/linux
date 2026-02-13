all: build

build: kubsh1.c vfs1.c
	gcc kubsh1.c vfs1.c -lreadline -lfuse3 -o kubsh1

run: build
	sudo ./kubsh1

clean:
	rm -f kubsh1
