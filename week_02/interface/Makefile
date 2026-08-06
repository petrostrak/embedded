CC     = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O1

OBJS = main.o mem_stream.o stdout_stream.o stream_util.o

demo: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS): io_stream.h

clean:
	rm -f demo $(OBJS)

.PHONY: clean
