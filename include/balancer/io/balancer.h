#pragma once
#include "coroutine/executor.h"
#include "base/nocopyable.h"

namespace lon::io {
	class IOWorkBalancer : public Noncopyable
	{
	public:
		virtual ~IOWorkBalancer();
		virtual void schedule(coroutine::Executor::Ptr) = 0;
	};
}
