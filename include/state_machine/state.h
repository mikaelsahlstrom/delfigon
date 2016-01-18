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
	typedef void Parent;

	virtual void event_handler(Hsm &) const = 0;
	virtual void do_handler(Hsm &) const = 0;
	virtual unsigned get_id() const = 0;
};

template<typename H, unsigned ID, typename P> struct Composite;

template<typename H, unsigned ID, typename P = Composite<H, 0, Top<H>>>
struct Composite : P
{
	typedef H Hsm;
	typedef P Parent;
	typedef Composite<Hsm, ID, Parent> This;

	virtual void do_handler(Hsm & h) const
		{
			Parent::do_handler(h);
			handle_do(h, * this);
		};

	template<typename LEAF>	void handle_do(Hsm & h, const LEAF & l) const {}
	template<typename LEAF>	void handle_event(Hsm & h, const LEAF & l) const
	{
		Parent::handle_event(h, l);
	}
	static void handle_entry(Hsm &) {}
	static void handle_init(Hsm &);
	static void handle_exit(Hsm &) {}
};

// Specialization for the top state
template<typename H>
struct Composite<H, 0, Top<H>> : Top<H>
{
	typedef H Hsm;
	typedef Top<Hsm> Parent;
	typedef Composite<Hsm, 0, Parent> This;

	virtual void do_handler(Hsm &) const {};
	template<typename THIS>	void handle_do   (Hsm &, const THIS &) const {}
	template<typename THIS>	void handle_event(Hsm &, const THIS &) const {}
	static void handle_entry(Hsm &) {}
	static void handle_init(Hsm &); // no implementation
	static void handle_exit(Hsm &) {}
};

template<typename H, unsigned ID, typename P = Composite<H, 0, Top<H>>>
struct Leaf : P
{
	typedef H Hsm;
	typedef P Parent;
	typedef Leaf<Hsm, ID, Parent> This;
	
	virtual void do_handler(Hsm & h) const override
	{
		Parent::do_handler(h);
		handle_do(h, * this);
	}
	template<typename THIS>
	void handle_do(Hsm & h, const THIS & t) const
	{
	}
	template<typename THIS>
	void handle_event(Hsm & h, const THIS & t) const
	{
		Parent::handle_event(h, t);
	}
	virtual void event_handler(Hsm & h) const override 
	{
		handle_event(h, * this);
	}
	virtual unsigned get_id() const override
	{
		return ID;
	}
	static void handle_init(Hsm & h)
	{
		h.next(obj);
	}
	// don't specialize this
	static void handle_entry(Hsm &) {}
	static void handle_exit(Hsm &) {}
	static const Leaf obj;
};

template<typename H, unsigned ID, typename P> 
const Leaf<H, ID, P> Leaf<H, ID, P>::obj;

template<bool> class Bool {};

template<typename C, typename P>
class Is_child
{
	typedef C Child;
	typedef P Parent;
	
	class Yes {	char a[1]; };
	class No  {	char a[5]; };
	static Yes test( Parent * ); // undefined
	static No test( ... );    // undefined
public:
	enum { Res = sizeof(test(static_cast<Child *>(0))) == sizeof(Yes) ? 1 : 0 };
};

template<typename C>
struct Init
{
	typedef C Child;
	typedef typename C::Hsm Hsm;

	Init(Hsm & h)
		: hsm_(h) {}
	~Init()
	{
		Child::handle_entry(hsm_);
		Child::handle_init(hsm_);
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
	typedef typename Current::Parent Current_parent;
	typedef typename Target::Parent Target_parent;
	enum { // work out when to terminate template recursion
		eTB_CB = Is_child<Target_parent, Current_parent>::Res,
		eS_CB  = Is_child<S, Current_parent>::Res,
		eS_C   = Is_child<S, C>::Res,
		eC_S   = Is_child<C, S>::Res,
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
		T::handle_init(hsm_);
	}
	// We use overloading to stop recursion. The more natural template
	// specialization method would require to specialize the inner
	// template without specializing the outer one, which is
	// forbidden.
	static void exit_actions(Hsm &, Bool<true>) {}
	static void exit_actions(Hsm & h, Bool<false>)
	{
		typedef Transition<Current_parent, Source, Target> Trans; 

		C::handle_exit(h);
		Trans::exit_actions(h, Bool<exitStop>());
	}
	static void entry_actions(Hsm &, Bool<true>) {}
	static void entry_actions(Hsm & h, Bool<false>)
	{
		typedef Transition<Current_parent, Source, Target> Trans; 

		Trans::entry_actions(h, Bool<entryStop>());
		C::handle_entry(h);
	}

private:
	Hsm & hsm_;
};
} /* closes namespace state */
} /* closes namespace hsm */
