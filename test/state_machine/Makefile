CC       = g++
CFLAGS   = -std=c++11 -Wfatal-errors -g
INCLUDES =-I../../include
DEFINES  =-D GSL_THROW_ON_CONTRACT_VIOLATION

all: state_test

state_test: state_test.cc
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@


clean:
	rm state_test
