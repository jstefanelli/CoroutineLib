#ifndef COROUTINELIB_CC_SYNCHRONOUS_QUEUE_H
#define COROUTINELIB_CC_SYNCHRONOUS_QUEUE_H

#include <mutex>
#include <queue>
#include <optional>

namespace crlib {
	template<typename T>
	class SyncQueue {
	private:
		std::queue<T> queue;
		std::mutex mutex;

	public:
		bool push(T item) {
			std::lock_guard guard(mutex);

			queue.push(std::move(item));
			return true;
		}

		std::optional<T> pull() {
			std::lock_guard guard(mutex);

			if (queue.empty()) {
				return std::nullopt;
			}

			T v = queue.front();
			queue.pop();
			return v;
		}
	};
}

#endif //COROUTINELIB_CC_SYNCHRONOUS_QUEUE_H
