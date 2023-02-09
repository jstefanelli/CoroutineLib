#ifndef COROUTINELIB_CC_QUEUE_CONFIG_H
#define COROUTINELIB_CC_QUEUE_CONFIG_H

#include "cc_boundless_queue.h"

namespace crlib {
	template<typename T>
	using default_queue = BoundlessQueue<T>;
}

#endif //COROUTINELIB_CC_QUEUE_CONFIG_H
