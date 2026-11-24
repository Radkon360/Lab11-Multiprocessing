CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS=-ljpeg
SOURCES=mandel.c jpegrw.c mandelmovie.c
OBJECTS=$(SOURCES:.c=.o)
MANDEL_OBJS=mandel.o jpegrw.o
MOVIE_OBJS=mandelmovie.o

all: mandel mandelmovie

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

mandel: $(MANDEL_OBJS)
	$(CC) $(MANDEL_OBJS) $(LDFLAGS) -o $@

mandelmovie: $(MOVIE_OBJS)
	$(CC) $(MOVIE_OBJS) -lm -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
	$(CC) -MM $< > $*.d

# stitch the 50 frames into a movie (run mandelmovie first)
movie:
	ffmpeg -y -i mandel%d.jpg mandel.mpg

clean:
	rm -rf $(OBJECTS) mandel mandelmovie *.d

.PHONY: all clean movie
