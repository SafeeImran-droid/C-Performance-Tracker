CC = gcc
CFLAGS = -Wall -O2 `pkg-config --cflags sdl3 sdl3-ttf`
LDFLAGS = `pkg-config --libs sdl3 sdl3-ttf` -lm

SRCS = main.c sdl_gui.c attendance.c grades.c random_forest.c utils.c
OBJS = $(SRCS:.c=.o)

TARGET = sdl3_tracker

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
