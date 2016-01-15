/*
 * \brief  Test of HSM
 * \author Stefan Heinzmann, Pontus Astrom
 * \date   2016-01-15
 *
 * The implementation is mostly taken from
 * http://accu.org/index.php/journals/252 with modifications to
 * fix a few bugs and improve naming.
 * TODO: * Switch to GSL library instead of asserts for Expect and Ensure
           terminology
 */
#include <iostream>
#include "state_machine/hsm.h"

namespace test
{
enum class Event {A, B, C, D, E, F, G, H};

class Simple_hsm : public hsm::Basic<Simple_hsm, Event>
{
public:
	Simple_hsm()
		: foo_ (0)
		{};
	~Simple_hsm()
		{};
	void foo(int i)
		{
			foo_ = i;
		}
	int foo() const
		{
			return foo_;
		}

private:
	int foo_;
};

/**
 * Definition of the state hierarchy. The numbers are inique unsigned
 * values used to uniquify each type in the case two states have the
 * same parent. */
typedef hsm::state::Composite<Simple_hsm, 0>       Top;
typedef hsm::state::Composite<Simple_hsm, 1, Top>  S0;
typedef hsm::state::Composite<Simple_hsm, 2, S0>   S1;
typedef hsm::state::Leaf     <Simple_hsm, 3, S1>   S11;
typedef hsm::state::Composite<Simple_hsm, 4, S0>   S2;
typedef hsm::state::Composite<Simple_hsm, 5, S2>   S21;
typedef hsm::state::Leaf     <Simple_hsm, 6, S21>  S211;

} /*namespace test */

/* Annotated handler for the state S0 outlining what executes in the
 * different parts of the handler function. */
template<>
template<typename LEAF>
inline void test::S0::handle(test::Simple_hsm & h, const LEAF & l) const
{
	using hsm::state::Transition;
	using namespace test;
	/* UML do/ processing goes here and happens whenever an event is
	 * received */
	switch(h.get_event()) {
    case Event::E:
	{
		/* Guard condition (no guard here)*/
		/* optional Transition if a state change shall be
		 * affected. Transition::Transition() performs UML exit/ actions.*/
		Transition<LEAF, This, S211> t(h);
		/* Transition action goes here */
		std::cout << "Tran(S0, S211, E) - ";
		/* The return statement triggers Transition::~Transition(),
		 * which performs UML entry/ actions.*/
		return;
	}
    default:
		break;
	}
	return Base::handle(h, l);
}

template<>
template<typename LEAF>
inline void test::S1::handle(test::Simple_hsm & h, const LEAF & l) const
{
	using hsm::state::Transition;
	using namespace test;
	switch(h.get_event()) {
    case Event::A:
	{
		Transition<LEAF, This, S1> t(h);
		std::cout << "Tran(S1, S1, A) - ";
		return;
	}
    case Event::B:
	{
		Transition<LEAF, This, S11> t(h);
		std::cout << "Tran(S1, S11, B) - ";
		return;
	}
    case Event::C:
	{
		Transition<LEAF, This, S2> t(h);
		std::cout << "Tran(S1, S2, C) - ";
		return;
	}
    case Event::D:
	{
		Transition<LEAF, This, S0> t(h);
		std::cout << "Tran(S1, S0, D) - ";
		return;
	}
    case Event::F:
	{
		Transition<LEAF, This, S211> t(h);
		std::cout << "Tran(S1, S211, F) - ";
		return;
	}
    default:
		break;
  }
  return Base::handle(h, l);
}

template<>
template<typename LEAF>
inline void test::S11::handle(test::Simple_hsm & h, const LEAF & l) const
{
	using hsm::state::Transition;
	using namespace test;

	switch(h.get_event()) {
    case Event::G:
	{
		Transition<LEAF, This, S211> t(h);
		std::cout << "Tran(S11, S211, G) - ";
		return;
	}
    case Event::H:
		if (h.foo()) {
			std::cout << "foo?/Do(S11, H) - ";
			h.foo(0);
			return;
		}
		break;
    default:
		break;
	}
	return Base::handle(h, l);
}

template<>
template<typename LEAF>
inline void test::S2::handle(test::Simple_hsm & h, const LEAF & l) const
{
	using hsm::state::Transition;
	using namespace test;

	switch(h.get_event()) {
    case Event::C:
	{
		Transition<LEAF, This, S1> t(h);
		std::cout << "Tran(S2, S1, C) - ";
		return;
	}
    case Event::F:
	{
		Transition<LEAF, This, S11> t(h);
		std::cout << "Tran(S2, S11, F) - ";
		return;
	}
    default: break;
	}
	return Base::handle(h, l);
}

template<>
template<typename LEAF>
inline void test::S21::handle(test::Simple_hsm & h, const LEAF & l) const
{
	using hsm::state::Transition;
	using namespace test;

	switch(h.get_event()) {
	case Event::B:
	{
		Transition<LEAF, This, S211> t(h);
		std::cout << "Tran(S21, S211, B) - ";
		return;
	}
    case Event::H:
		if (!h.foo()) {
			Transition<LEAF, This, S21> t(h);
			std::cout << "!foo/Tran(S21, S21, H) - ";
			h.foo(1);
			return;
		}
		break;
	default:
		break;
  }
  return Base::handle(h, l);
}

template<>
template<typename LEAF>
inline void test::S211::handle(test::Simple_hsm & h, const LEAF & l) const
{
	using hsm::state::Transition;
	using namespace test;

	switch(h.get_event()) {
    case Event::D:
	{
		Transition<LEAF, This, S21> t(h);
		std::cout << "Tran(S211, S21, D) - ";
		return;
	}
    case Event::G:
	{
		Transition<LEAF, This, S0> t(h);
		std::cout << "Tran(S211, S0, G) - ";
		return;
	}
    default:
		break;
	}
	return Base::handle(h, l);
}

namespace hsm {
namespace state {
// Top -------------------------------------------------------------------------
template<>
inline void test::Top::init(test::Simple_hsm & arg)
{
	std::cout << "Init(Top) - ";
	Init<test::S0> tmp(arg);
}
// S0 --------------------------------------------------------------------------
template<>
inline void test::S0::entry(test::Simple_hsm &)
{
	std::cout << "Entry(S0) - ";
}
template<>
inline void test::S0::init(test::Simple_hsm & arg)
{
	std::cout << "Init(S0) - ";
	Init<test::S1> tmp(arg);
}
template<>
inline void test::S0::exit(test::Simple_hsm &)
{
	std::cout << "Exit(S0) - ";
}
// S1 --------------------------------------------------------------------------
template<>
inline void test::S1::entry(test::Simple_hsm &)
{
	std::cout << "Entry(S1) - ";
}
template<>
inline void test::S1::init(test::Simple_hsm & arg)
{
	std::cout << "Init(S1) - ";
	Init<test::S11> tmp(arg);
}
template<>
inline void test::S1::exit(test::Simple_hsm &)
{
	std::cout << "Exit(S1) - ";
}
// S11 -------------------------------------------------------------------------
template<>
inline void test::S11::entry(test::Simple_hsm &)
{
	std::cout << "Entry(S11) - ";
}
template<>
inline void test::S11::exit(test::Simple_hsm &)
{
	std::cout << "Exit(S11) - ";
}
// S2 --------------------------------------------------------------------------
template<>
inline void test::S2::entry(test::Simple_hsm &)
{
	std::cout << "Entry(S2) - ";
}
template<>
inline void test::S2::init(test::Simple_hsm & arg)
{
	std::cout << "Init(S2) - ";
	Init<test::S21> tmp(arg);
}
template<>
inline void test::S2::exit(test::Simple_hsm &)
{
	std::cout << "Exit(S2) - ";
}
// S21 -------------------------------------------------------------------------
template<>
inline void test::S21::entry(test::Simple_hsm &)
{
	std::cout << "Entry(S21) - ";
}
template<>
inline void test::S21::init(test::Simple_hsm & arg)
{
	std::cout << "Init(S21) - ";
	Init<test::S211> tmp(arg);
}
template<>
inline void test::S21::exit(test::Simple_hsm &)
{
	std::cout << "Exit(S21) - ";
}
// S211 ------------------------------------------------------------------------
template<>
inline void test::S211::entry(test::Simple_hsm &)
{
	std::cout << "Entry(S211) - ";
}
template<>
inline void test::S211::exit(test::Simple_hsm &)
{
	std::cout << "Exit(S211) - ";
}
}
}

int main()
{
	test::Simple_hsm test;
	test::Top::init(test);
	for(;;) {
		std::cout << "\nEvent: ";
		char c;
		std::cin >> c;
		if (c < 'a' || 'h' < c) {
			return 0;
		}
		test.dispatch((test::Event)(c - 'a'));
	}
}
