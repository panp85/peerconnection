COBJS += rtsp2mp4.o

CFLAGS = -O2 -Wall -DDEBUG
#CFLAGS += -I/home/panpan/player/newest_ffmpeg/ffmpeg/test/include/ -L/home/panpan/player/newest_ffmpeg/ffmpeg/test/lib/
CFLAGS += -I/home/panpan/player/newest_ffmpeg/ffmpeg/test/include
#CFLAGS	+= -L../test/lib

#LDFLAGS	+= -lmxml -lpthread
LDFLAGS += -lavformat -lavcodec -lswscale -lswresample -lavutil -lm -lz

CROSS_COMPILE	?=

CC = $(CROSS_COMPILE)g++

TARGET = tag

all:$(TARGET)
$(TARGET):$(COBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o:%.c
	#$(CC) $(CFLAGS) -c -o $@ $^ $(LDFLAGS)
	$(CC) -I/home/panpan/player/newest_ffmpeg/ffmpeg/test/include/ -L/home/panpan/player/newest_ffmpeg/ffmpeg/test/lib/ -o $@ $^ $(LDFLAGS)

.PHONY:clean
clean:
	rm -f $(COBJS)
