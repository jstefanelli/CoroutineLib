#ifndef COROUTINELIB_CC_GENERIC_QUEUE_H
#define COROUTINELIB_CC_GENERIC_QUEUE_H

#include "cc_dictionary.h"
#include "cc_queue_2.h"
#include <thread>
#include <memory>
#include <optional>

#ifndef CRLIB_DEFAULT_GENERIC_QUEUE_SIZE
#define CRLIB_DEFAULT_GENERIC_QUEUE_SIZE 256
#endif

namespace crlib {
	template<typename T>
	class GenericQueue {
	protected:
		ConcurrentDictionary<std::thread::id, std::shared_ptr<SingleProducerQueue<T>>> dict;
	public:
		bool push(const T& val) {
			auto id = std::this_thread::get_id();

			auto queue_opt = dict.get(id);
			if (queue_opt.has_value()) {
				return queue_opt.value()->push(val);
			} else {
				auto queue = std::make_shared<SingleProducerQueue<T>>(CRLIB_DEFAULT_GENERIC_QUEUE_SIZE);
				dict.set(id, queue);
				return queue->push(val);
			}
		}

		std::optional<T> pull() {
			for (auto p : dict) {
				auto val = p.second->pull();
				if (val.has_value()) {
					return val;
				}
			}
			return std::nullopt;
		}

		bool empty() {
			for(auto p : dict) {
				if (!p.second->empty()) {
					return false;
				}
			}

			return true;
		}

		//Returns 'true' if the next push operation on the current thread will fail
		bool full() {
			auto id = std::this_thread::get_id();

			auto queue_opt = dict.get(id);
			if (queue_opt.has_value()) {
				return queue_opt.value()->full();
			} else {
				return false;
			}
		}
	};
}

#endif //COROUTINELIB_CC_GENERIC_QUEUE_H
