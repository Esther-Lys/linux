CC = gcc
CFLAGS = -Wall -g -D_FILE_OFFSET_BITS=64
LDFLAGS = -lreadline -lfuse3

TARGET = kubsh1
SOURCES = kubsh1.c vfs1.c
HEADERS = vfs1.h

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
        rm -f $(TARGET)
        rm -f users
        -fusermount3 -u users 2>/dev/null || umount users 2>/dev/null

run: $(TARGET)
	sudo ./$(TARGET)

debug: $(TARGET)
        sudo gdb ./$(TARGET)

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

 
.PHONY: all run clean install 
