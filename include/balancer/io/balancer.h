#pragma once
#include "coroutine/executor.h"
#include "base/nocopyable.h"
#include <any>

namespace lon::io {
	class IOWorkBalancer : public Noncopyable
	{
	public:
		virtual ~IOWorkBalancer();
		// virtual void schedule(coroutine::Executor::Ptr) = 0;
		virtual void schedule(coroutine::Executor::Ptr, [[maybe_unused]] std::any arg = std::any()) = 0;
	};
}
