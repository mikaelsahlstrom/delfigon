/*
 * \brief  Definitions, classes for designing hierarchical state machines
 * \author Stefan Heinzmann, Pontus Astrom
 * \date   2004-12-01, modified 2016-01-17
 *
 * The implementation is mostly taken from
 * http://accu.org/index.php/journals/252 with small modifications to
 * fix a few bugs, and add some additional UML statechart
 * support. For instance, with this version it is possible to have a CompState
 * as target for a transaction.
 */

namespace hsm {
namespace state {

template<typename H>
struct Top
{
	typedef H Hsm;
	typedef void Base;

	virtual void handler(Hsm &) const = 0;
	virtual unsigned get_id() const = 0;
};

template<typename H, unsigned ID, typename B> struct Composite;

template<typename H, unsigned ID, typename B = Composite<H, 0, Top<H>>>
struct Composite : B
{
	typedef H Hsm;
	typedef B Base;
	typedef Composite<Hsm, ID, Base> This;

	template<typename LEAF>
	void handle(Hsm & h, const LEAF & l) const
	{
		Base::handle(h, l);
	}
	static void entry(Hsm &) {}
	static void init(Hsm &);
	static void exit(Hsm &) {}
};

// Specialization for the top state
template<typename H>
struct Composite<H, 0, Top<H>> : Top<H>
{
	typedef H Hsm;
	typedef Top<Hsm> Base;
	typedef Composite<Hsm, 0, Base> This;

	template<typename THIS>
	void handle(Hsm &, const THIS &) const {}
	static void entry(Hsm &) {}
	static void init(Hsm &); // no implementation
	static void exit(Hsm &) {}
};

template<typename H, unsigned ID, typename B = Composite<H, 0, Top<H>>>
struct Leaf : B
{
	typedef H Hsm;
	typedef B Base;
	typedef Leaf<Hsm, ID, Base> This;
	
	template<typename THIS>
	void handle(Hsm & h, const THIS & t) const
	{
		Base::handle(h, t);
	}
	virtual void handler(Hsm & h) const override 
	{
		handle(h, * this);
	}
	virtual unsigned get_id() const override
	{
		return ID;
	}
	static void init(Hsm & h)
	{
		h.next(obj);
	}
	// don't specialize this
	static void entry(Hsm &) {}
	static void exit(Hsm &) {}
	static const Leaf obj;
};

template<typename H, unsigned ID, typename B> 
const Leaf<H, ID, B> Leaf<H, ID, B>::obj;

template<bool> class Bool {};

template<typename D, typename B>
class Is_derived
{
	typedef D Derived;
	typedef B Base;
	
	class Yes {	char a[1]; };
	class No  {	char a[5]; };
	static Yes test( Base * ); // undefined
	static No test( ... );    // undefined
public:
	enum { Res = sizeof(test(static_cast<Derived *>(0))) == sizeof(Yes) ? 1 : 0 };
};

template<typename D>
struct Init
{
	typedef D Derived;
	typedef typename D::Hsm Hsm;

	Init(Hsm & h)
		: hsm_(h) {}
	~Init()
	{
		Derived::entry(hsm_);
		Derived::init(hsm_);
	}
	Hsm & hsm_;
};

template<typename C, typename S, typename T>
struct Transition
{
	typedef C Current;
	typedef S Source;
	typedef T Target;
	typedef typename Current::Hsm Hsm;
	typedef typename Current::Base Current_base;
	typedef typename Target::Base Target_base;
	enum { // work out when to terminate template recursion
		eTB_CB = Is_derived<Target_base, Current_base>::Res,
		eS_CB  = Is_derived<S, Current_base>::Res,
		eS_C   = Is_derived<S, C>::Res,
		eC_S   = Is_derived<C, S>::Res,
		exitStop = eTB_CB && eS_C,
		entryStop = eS_C || eS_CB && !eC_S
	};
	Transition(Hsm & h)
		: hsm_(h)
	{
		exit_actions(hsm_, Bool<false>());
	}
	~Transition()
	{
		typedef Transition<Target, Source, Target> Trans;

		Trans::entry_actions(hsm_, Bool<false>());
		T::init(hsm_);
	}
	// We use overloading to stop recursion. The more natural template
	// specialization method would require to specialize the inner
	// template without specializing the outer one, which is
	// forbidden.
	static void exit_actions(Hsm &, Bool<true>) {}
	static void exit_actions(Hsm & h, Bool<false>)
	{
		typedef Transition<Current_base, Source, Target> Trans; 

		C::exit(h);
		Trans::exit_actions(h, Bool<exitStop>());
	}
	static void entry_actions(Hsm &, Bool<true>) {}
	static void entry_actions(Hsm & h, Bool<false>)
	{
		typedef Transition<Current_base, Source, Target> Trans; 

		Trans::entry_actions(h, Bool<entryStop>());
		C::entry(h);
	}

private:
	Hsm & hsm_;
};
} /* closes namespace state */
} /* closes namespace hsm */
