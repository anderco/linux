top_srcdir = ../../../../../

CFLAGS += \
	-Iinclude
CFLAGS += -Wall
CFLAGS += -g -O2

LIBS = -lbsd

test-drm_dp_helper.o: ../../drm_dp_helper.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm *.o
