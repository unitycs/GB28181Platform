#ifndef __ATL_TEMPLATE_BOOL_H_
#define __ATL_TEMPLATE_BOOL_H_

//	Branch select tool
struct TSelector {
	template<class A, class B> struct Selector {
		typedef A RET;
	};
};

struct FSelector {
	template<class A, class B> struct Selector {
		typedef B RET;
	};
};

#define DECLARE_TYPE_SWITCH(if_name, type_name, true_value)	\
	template<##type_name cond>	struct Selector##if_name {	\
		typedef FSelector RET;	\
	};	\
	template<> struct Selector##if_name<true_value> {	\
		typedef TSelector RET;	\
	};	\
	template<##type_name cond, class A, class B> struct Switch##if_name {	\
		typedef typename Selector##if_name<cond>::RET Selector;	\
		typedef typename Selector::Selector<A, B>::RET RET;	\
	};

#define TYPE_SWITCH(if_name, para, true_value, false_value)	\
	Switch##if_name<para, true_value, false_value>::RET 

#endif // __ATL_TEMPLATE_BOOL_H_

