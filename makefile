CC = gcc
CFLAGS = -Wall -O2 -I/mingw64/include
LDFLAGS = -L/mingw64/lib -lopengl32 -lglu32 -lfreeglut -lm -lpthread

SRCS = main.c attendance.c grades.c random_forest.c gui.c utils.c
OBJS = $(SRCS:.c=.o)

TARGET = gui_tracker.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
