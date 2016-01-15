/*
 * \brief  Definitions, classes for designing hierarchical state machines
 * \author Pontus Astrom
 * \date   2016-01-15
 *
 * The implementation is mostly taken from
 * http://accu.org/index.php/journals/252 with small modifications to
 * fix a few bugs, and add some additional UML statechart
 * support. For instance, with this version it is possible to have a CompState
 * as target for a transaction.
 * TODO: * Switch to GSL library instead of asserts for Expect and Ensure
           terminology
         * Implement Queued HSM and Queued HSM with threads for more advanced
		   state machines.
 */
#include <assert.h>
#include "state.h"

namespace hsm
{ 

template<typename H, typename E>
class Basic
{
public:
	typedef E Event;
	
protected:
	typedef H Hsm;
	typedef state::Top<Hsm> State;
	
	const State * state_;
	Event event_;
	
public:
	Basic()
		: event_(Event()),
		  state_(nullptr)
		{};
	~Basic() {}
	
	void next(const State & state)
	{
		state_ = & state;
	}
	Event get_event() const
	{
		return event_;
	}
	void dispatch(Event e)
	{
		assert(state_ != nullptr);
		event_ = e;
		state_->handler(*static_cast<Hsm*>(this));
	}
};
} /* namespace hsm */
