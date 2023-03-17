#ifndef COROUTINELIB_CC_QUEUE_CONFIG_H
#define COROUTINELIB_CC_QUEUE_CONFIG_H

#include "cc_boundless_queue.h"
#include "cc_synchronous_queue.h"

namespace crlib {

	template<typename T>
#if !defined(CRLIB_USE_SYNCHRONOUS_QUEUE)
	using default_queue = BoundlessQueue<T>;
#else
	using default_queue = SyncQueue<T>;
#endif
}

#endif //COROUTINELIB_CC_QUEUE_CONFIG_H
