top_srcdir = ../../../../../

CFLAGS += \
	-Iinclude
CFLAGS += -Wall
CFLAGS += -g -O2

LIBS = -lbsd

all: test

test-drm_dp_helper.o: ../../drm_dp_helper.c
	$(CC) $(CFLAGS) -c -o $@ $^

test-intel_dp_link_training.o: ../intel_dp_link_training.c
	$(CC) $(CFLAGS) -c -o $@ $^

test: test-drm_dp_helper.o test-intel_dp_link_training.o

clean:
	rm *.o
