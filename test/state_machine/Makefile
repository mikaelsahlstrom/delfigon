CC       = clang++
CFLAGS   = -std=c++11 -Wall -Wfatal-errors -g -I../../include
INCLUDES =../../include/state_machine/hsm.h ../../include/state_machine/state.h
DEFINES  =-D GSL_THROW_ON_CONTRACT_VIOLATION

all: state_test

state_test: state_test.cc $(INCLUDES)
	$(CC) $(CFLAGS) $(DEFINES) $< -o $@


clean:
	rm state_test
