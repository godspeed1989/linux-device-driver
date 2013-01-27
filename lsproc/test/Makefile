CC=gcc
LIBS=-lpthread
TARGET=thread

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) -g $< $(LIBS) -o $@

run:
	sudo ./$(TARGET)

clean:
	rm -rf $(TARGET)
