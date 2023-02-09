#ifndef COROUTINELIB_CC_BASE_QUEUE_H
#define COROUTINELIB_CC_BASE_QUEUE_H

#include <concepts>

namespace crlib {
	template<typename Q, typename V>
	concept is_queue = requires(Q queue, V val) {
		!queue.push(val);
		queue.pull().has_value();
	};

	template<typename Q, typename V>
	concept is_peekable_queue = is_queue<Q, V> && requires(Q queue) {
		queue.peek().has_value();
	};

	template<typename Q, typename V>
	concept has_bounds_queries = is_queue<Q, V> && requires(Q queue) {
		!queue.is_full();
		!queue.is_empty();
	};
}

#endif //COROUTINELIB_CC_BASE_QUEUE_H
