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

namespace hsm { namespace state {
	template<typename H> struct Top;
   template<typename H, unsigned ID, typename P> struct Composite;
	template<typename H, unsigned ID, typename P> struct Leaf;
	template<bool>                                class  Bool {};
	template<typename C, typename P>              class  Is_child;
	template<typename C>                          struct Init;
	template<typename C, typename S, typename T>	 struct Transition;
	}
}

/******************************************************************************
 ** Top declaration
 ******************************************************************************/
template<typename H>
struct hsm::state::Top
{
	typedef H Hsm;
	typedef void Parent;

	virtual void     event_handler(Hsm &) const = 0;
	virtual void     do_handler(Hsm &) const = 0;
	virtual unsigned get_id() const = 0;
};


/******************************************************************************
 ** Composite declaration
 ******************************************************************************/
template<typename H, unsigned ID,
			typename P = hsm::state::Composite<H, 0, hsm::state::Top<H>>>
struct hsm::state::Composite : P
{
	typedef H Hsm;
	typedef P Parent;
	typedef Composite<Hsm, ID, Parent> This;

	virtual void do_handler(Hsm &) const;

	template<typename LEAF>	void handle_do   (Hsm &, const LEAF &) const {}
	template<typename LEAF>	void handle_event(Hsm &, const LEAF &) const;

	/* Provide no default implementation as 1) no default
	 * implementation makes sense and 2) to achieve compile time
	 * failure in case specialization is forgotten. */
	static void handle_init (Hsm &);
	static void handle_entry(Hsm &) {}
	static void handle_exit (Hsm &) {}
};


/******************************************************************************
 ** Composite specialization for the Top state
 ******************************************************************************/
namespace hsm { namespace state {
	template<typename H>
	struct Composite<H, 0, Top<H>> : Top<H>
	{
		typedef H Hsm;
		typedef Top<Hsm> Parent;
		typedef Composite<Hsm, 0, Parent> This;
		
		virtual void do_handler(Hsm &) const {};
		
		template<typename THIS>	void handle_do   (Hsm &, const THIS &) const {}
		template<typename THIS>	void handle_event(Hsm &, const THIS &) const {}
		
		static void handle_init (Hsm &);
		static void handle_entry(Hsm &) {}
		static void handle_exit (Hsm &) {}
	};
}
}

/******************************************************************************
 ** Leaf declaration
 ******************************************************************************/
template<typename H, unsigned ID,
			typename P = hsm::state::Composite<H, 0, hsm::state::Top<H>>>
struct hsm::state::Leaf : P
{
	typedef H Hsm;
	typedef P Parent;
	typedef Leaf<Hsm, ID, Parent> This;
	
	virtual void     do_handler   (Hsm &) const override;
	virtual void     event_handler(Hsm &) const override;
	virtual unsigned get_id()             const override;

	template<typename THIS>	void handle_do   (Hsm &, const THIS &) const {}
	template<typename THIS>	void handle_event(Hsm &, const THIS &) const;

	static void handle_init (Hsm &);
	static void handle_entry(Hsm &) {} 	
	static void handle_exit (Hsm &) {}

	static const Leaf obj;
};


/******************************************************************************
 ** Init declaration
 ******************************************************************************/
template<typename C>
struct hsm::state::Init
{
	typedef C Child;
	typedef typename C::Hsm Hsm;

	Init(Hsm &);
	~Init();
	
	Hsm & _hsm;
};

/******************************************************************************
 ** Is_child definition
 ** See GOTW #71 for design
 ******************************************************************************/
template<typename C, typename P>
class hsm::state::Is_child
{
	typedef C   Child;
	typedef P   Parent;
	class   Yes { char a[1]; };
	class   No  { char a[5]; };
	static  Yes test( Parent * ); // undefined
	static  No  test( ... );    // undefined

public:
	enum { Res = sizeof(test(static_cast<Child *>(0))) == sizeof(Yes) ? 1 : 0 };
};

/******************************************************************************
 ** Transition declaration
 ******************************************************************************/
template<typename C, typename S, typename T>
struct hsm::state::Transition
{
	typedef C Current;
	typedef S Source;
	typedef T Target;
	typedef typename Current::Hsm    Hsm;
	typedef typename Current::Parent Current_parent;
	typedef typename Target::Parent  Target_parent;
	enum { // work out when to terminate template recursion
		eTB_CB    = Is_child<Target_parent, Current_parent>::Res,
		eS_CB     = Is_child<S, Current_parent>::Res,
		eS_C      = Is_child<S, C>::Res,
		eC_S      = Is_child<C, S>::Res,
		exitStop  = eTB_CB && eS_C,
		entryStop = eS_C || (eS_CB && !eC_S)
	};

	Transition(Hsm &);
	~Transition();
	// We use overloading to stop recursion. The more natural template
	// specialization method would require to specialize the inner
	// template without specializing the outer one, which is
	// forbidden.
	static void exit_actions (Hsm &  , Bool<true>) {}
	static void exit_actions (Hsm & h, Bool<false>);
	static void entry_actions(Hsm &  , Bool<true>) {}
	static void entry_actions(Hsm & h, Bool<false>);

private:
	Hsm & _hsm;
};

/******************************************************************************
 ** Composite definition
 ******************************************************************************/
template<typename H, unsigned ID, typename P>
void
hsm::state::Composite<H, ID, P>::do_handler(Hsm & arg) const
{
	Parent::do_handler(arg);
	handle_do(arg, * this);
};

template<typename H, unsigned ID, typename P>
template<typename LEAF>
void
hsm::state::Composite<H, ID, P>::handle_event(Hsm & h, const LEAF & l) const
{
	Parent::handle_event(h, l);
}

/******************************************************************************
 ** Leaf definition
 ******************************************************************************/
template<typename H, unsigned ID, typename P>
void
hsm::state::Leaf<H, ID, P>::do_handler(Hsm & arg) const
{
	Parent::do_handler(arg);
	handle_do(arg, * this);
}

template<typename H, unsigned ID, typename P>
template<typename THIS>
void
hsm::state::Leaf<H, ID, P>::handle_event(Hsm & h, const THIS & t) const
{
	Parent::handle_event(h, t);
}

template<typename H, unsigned ID, typename P>
void
hsm::state::Leaf<H, ID, P>::event_handler(Hsm & arg) const
{
	handle_event(arg, * this);
}

template<typename H, unsigned ID, typename P>
unsigned
hsm::state::Leaf<H, ID, P>::get_id() const
{
	return ID;
}

template<typename H, unsigned ID, typename P>
void
hsm::state::Leaf<H, ID, P>::handle_init(Hsm & arg)
{
	arg.next(obj);
}

template<typename H, unsigned ID, typename P> 
const hsm::state::Leaf<H, ID, P> hsm::state::Leaf<H, ID, P>::obj {};

/******************************************************************************
 * Init definition
 ******************************************************************************/
template<typename C>
hsm::state::Init<C>::Init(Hsm & arg)
	: _hsm(arg)
{
}

template<typename C>
hsm::state::Init<C>::~Init()
{
	Child::handle_entry(_hsm);
	Child::handle_init (_hsm);
}

/******************************************************************************
 ** Transition definition
 ******************************************************************************/
template<typename C, typename S, typename T>
hsm::state::Transition<C, S, T>::Transition(Hsm & arg)
	: _hsm(arg)
{
	exit_actions(_hsm, Bool<false>());
}

template<typename C, typename S, typename T>
hsm::state::Transition<C, S, T>::~Transition()
{
	typedef Transition<Target, Source, Target> Trans;
	Trans::entry_actions(_hsm, Bool<false>());
	T::handle_init(_hsm);
}

template<typename C, typename S, typename T>
void
hsm::state::Transition<C, S, T>::exit_actions(Hsm & h, Bool<false>)
{
	typedef Transition<Current_parent, Source, Target> Trans; 
	C::handle_exit(h);
	Trans::exit_actions(h, Bool<exitStop>());
}

template<typename C, typename S, typename T>
void
hsm::state::Transition<C, S, T>::entry_actions(Hsm & h, Bool<false>)
{
	typedef Transition<Current_parent, Source, Target> Trans; 
	Trans::entry_actions(h, Bool<entryStop>());
	C::handle_entry(h);
}
