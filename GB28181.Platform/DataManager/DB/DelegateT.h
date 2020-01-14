#pragma once
#include <map>
#include <functional>

template<typename Arg1, typename... Arg2>
class Event
{
	typedef void HandlerT(Arg1, Arg2);
	int m_handlerId;

public:
	Event() : m_handlerId(0) {}
	~Event() = default;

	template<class FuncT>
	int addHandler(FuncT func)
	{
		m_handlers.emplace(m_handlerId, forward<FuncT>(func));
		return m_handlerId++;
	}

	void removeHandler(int handlerId)
	{
		m_handlers.erase(handlerId);
	}

	void operator ()(Arg1 p1, Arg2 p2)
	{
		for (const auto& i : m_handlers)
			i.second(p1, p2);
	}

private:
	std::map<int, std::function<HandlerT>> m_handlers;
};

template<typename ReturnT, typename DelegateT, typename... Args>
class EventHandler
{
	typedef ReturnT *EventHandler(Args...);
	int m_handlerId;

public:
	EventHandler() : m_handlerId(0) {}
	~Event() = default;

	int addHandler(EventHandler func_ptr)
	{
		m_handlers.emplace(m_handlerId, forward<EventHandler>(func_ptr));
		return m_handlerId++;
	}

	void removeHandler(int handlerId)
	{
		m_handlers.erase(handlerId);
	}

	void operator ()(Args... args)
	{
		for (const auto& i : m_handlers)
			(*i).second(args...);
	}

private:
	std::map<int, std::function<HandlerT>> m_handlers;
};
///////

class IDelegateHandler {
public:
	virtual ~IDelegateHandler() { }
	virtual void Invoke(int) = 0;
};

template <typename ReturnT, class classT, typename DelegateT, typename... Args>
class  DelegateEventHandlerT : public IDelegateHandler {
public:
	DelegateEventHandlerT(DelegateT* pT, ReturnT(classT::*pFunc)(Args...))
		: m_pT(pT), m_pFunc(pFunc) { }

	ReturnT Invoke(Args... args) {
		return (m_pT->*m_pFunc)(args...);
	}
private:
	classT* m_pT;
	ReturnT(classT::*m_pFunc)(Args...);
};

template<typename ReturnT, typename... Args>
class DelegateEventHandlerT<ReturnT, void, Args...> : public IDelegateHandler {
public:
	DelegateEventHandlerT(ReturnT(*pFunc)(Args...))
		: m_pFunc(pFunc) { }

	ReturnT Invoke(Args... args) {
		return (*m_pFunc)(args...);
	}

private:
	ReturnT(*m_pFunc)(Args...);
};
