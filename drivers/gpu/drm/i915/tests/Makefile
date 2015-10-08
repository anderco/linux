top_srcdir = ../../../../../

CFLAGS += \
	-Iinclude
CFLAGS += -Wall
CFLAGS += -g3 -O0

LDFLAGS = -z muldefs
LIBS = -lbsd

all: test test_signal_level

test-drm_dp_helper.o: ../../drm_dp_helper.c
	$(CC) $(CFLAGS) -c -o $@ $^

test-intel_dp_link_training.o: ../intel_dp_link_training.c
	$(CC) $(CFLAGS) -c -o $@ $^

test_intel_dp_link_training.o: test_intel_dp_link_training.c
	$(CC) $(CFLAGS) -c -o $@ $^

# Note: the order is important to allow the test code to override symbols
# implemented in other object files
test: test_intel_dp_link_training.o test-drm_dp_helper.o test-intel_dp_link_training.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

test_signal_level: test_signal_level.o

clean:
	rm *.o test
