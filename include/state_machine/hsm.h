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
/*
#include <assert.h>
#include <deque>
*/
#include "state.h"

namespace hsm
{ 
	template<typename H, typename E> class Basic;
	template<typename H, typename E> class Queued;
}	

/******************************************************************************
 ** Basic declaration
 ******************************************************************************/
template<typename H, typename E>
class hsm::Basic
{
public:
	typedef E Event;
	
protected:
	typedef H Hsm;
	typedef state::Top<Hsm> State;
	
	const State * _state;
	Event         _event;

public:
	Basic();
	~Basic();

	void         next(const State &);
	Event        get_event() const;
	virtual void dispatch(Event);
};

/******************************************************************************
 ** Queued declaration
 ******************************************************************************/

/*
template<typename H, typename E>
class hsm::Queued : hsm::Basic<H, E>
{
public:
	typedef H Hsm;
	typedef E Event;
	typedef hsm::Basic<H, E> Base;
	typedef unsigned char Priority;
	static const Priority default_priority = static_cast<Priority>(1);
	
private:
	typedef std::deque<Event> Event_queue;

	Priority    _priority;
	Event_queue _event_queue;
	
public:
	Queued(Priority arg = default_priority);
	~Queued() {}

	void     dispatch();
	void     post(Event);
	void     post_front(Event);
	Priority get_priority() const;
	void     set_priority(Priority);
};

*/
/******************************************************************************
 ** Basic definition
 ******************************************************************************/
template<typename H, typename E>
hsm::Basic<H, E>::Basic()
	: _state(nullptr),
	  _event(Event())
{
}

template<typename H, typename E>
hsm::Basic<H, E>::~Basic() {}
	
template<typename H, typename E>
void
hsm::Basic<H, E>::next(const State & arg)
{
	_state = & arg;
}

template<typename H, typename E>
typename hsm::Basic<H, E>::Event
hsm::Basic<H, E>::get_event() const
{
	return _event;
}

template<typename H, typename E>
void
hsm::Basic<H, E>::dispatch(Event arg)
{
	/*	assert(_state != nullptr); */
	_event = arg;
	_state->do_handler(*static_cast<Hsm*>(this));
	_state->event_handler(*static_cast<Hsm*>(this));
}


/******************************************************************************
 ** Queued definition
 ******************************************************************************/

/*

template<typename H, typename E>
hsm::Queued<H, E>::Queued(Priority arg)
	: Base(),
	  _priority(arg)
{
}

template<typename H, typename E>
void
hsm::Queued<H, E>::dispatch()
{
	if (! _event_queue.empty()) {
		dispatch(_event_queue.front());
		_event_queue.pop_front();
	}
}

template<typename H, typename E>
void
hsm::Queued<H, E>::post(Event arg)
{
	_event_queue.push_back(arg);
}

template<typename H, typename E>
void
hsm::Queued<H, E>::post_front(Event arg)
{
	_event_queue.push_front(arg);
}

template<typename H, typename E>
typename hsm::Queued<H, E>::Priority
hsm::Queued<H, E>::get_priority() const
{
	return _priority;
}

template<typename H, typename E>
void
hsm::Queued<H, E>::set_priority(Priority arg)
{
	_priority = arg;
}

*/
