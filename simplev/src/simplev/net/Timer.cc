/*
 * Timer.cc
 *
 *  Created on: Jun 13, 2014
 *      Author: damonhao
 */

#include <stdio.h>

#include <simplev/base/Logging.h>
#include <simplev/net/Timer.h>
#include <simplev/net/EventLoop.h>

using namespace simplev;
using namespace simplev::net;

AtomicInt64 Timer::s_numCreated_;

Timer::Timer(EventLoop *loop, const TimerCallback& cb, double after,
		double interval) :
		timeWatcher_(loop->getEventLoopRef()),
		sequence_(s_numCreated_.incrementAndGet()),
		callBack_(cb)
{
	LOG_TRACE <<"Timer ctor:" << this;
	timeWatcher_.set<Timer, &Timer::run>(this);
	timeWatcher_.set(after, interval);
//	timeWatcher_.start(after, interval); //FIXME: Timer in incomplete state??
}

void Timer::start()
{
	timeWatcher_.start();
}

void Timer::stop()
{
	timeWatcher_.stop();
}

Timer::~Timer()
{
	LOG_TRACE <<"Timer dtor:" << this;
}
