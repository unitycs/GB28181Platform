#pragma once
#include<chrono>
#include <thread>
#include <future>
#include <atomic>

template<typename Duration_T>
class CEventTimer
{
public:
	using DELEGATE_FUNC_T = std::function<void()>;
	virtual ~CEventTimer() = default;

	CEventTimer() :
		_time_interval(5),
		_b_exit(false)
	{

	}

	inline void setparameter(DELEGATE_FUNC_T pFuctionProc, unsigned timeinterval = 5)
	{
		this->_time_interval = timeinterval;
		this->_delegateProc = std::move(pFuctionProc);
	}
	//overwrite call operator  
	inline bool operator()()
	{
		return is_timeout();
	}
	//is timeout  
	inline bool is_timeout()
	{
		auto _now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<Duration_T>(_now - _start) > Duration_T(0);
	}
	//reset start time  
	inline void reset()
	{
		_start = std::chrono::high_resolution_clock::now();
	}

	inline void stop()
	{
		_b_exit = true;
	}
	inline void start()
	{

		timerThread = thread([&]() {

			while (!_b_exit)
			{
				if (_delegateProc)
				{
					_delegateProc();
				}
				std::this_thread::sleep_for(std::chrono::seconds(_time_interval));
			}


		});
		timerThread.detach();
	}

private:
	//start time_poing  
	std::chrono::time_point<std::chrono::high_resolution_clock> _start{ std::chrono::high_resolution_clock::now() };

	unsigned int _time_interval;

	std::atomic<bool> _b_exit;

	DELEGATE_FUNC_T _delegateProc;

	std::thread timerThread;
};
