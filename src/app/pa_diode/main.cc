/*
 * \brief  Diode between two nic interfaces.
 * \author Pontus Åström
 * \date   2015-12-14
 */

/*
 * Copyright (C) 2015 XXX
 */

#include <base/env.h>
#include <base/printf.h>

#include <nic_session/nic_session.h>
#include <nic_session/connection.h>

#include <timer_session/connection.h>

using namespace Genode;

int main(void)
{
	Nic::Connection in;
	Nic::Connection out;

	Timer::Connection timer;

	while (1) {
		h.say_hello();

		int foo = h.add(2, 5);
		PDBG("Added 2 + 5 = %d", foo);
		timer.msleep(1000);
	}

	return 0;
}
