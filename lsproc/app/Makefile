CC=gcc
TARGET=pstree

all: $(TARGET)

$(TARGET): $(TARGET).c ../share.h
	$(CC) -g $< -o $@

run:
	sudo ./$(TARGET)

clean:
	rm -rf $(TARGET)

