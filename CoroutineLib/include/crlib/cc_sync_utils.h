//
// Created by John Stefanelli on 11/01/23.
//

#ifndef COROUTINELIB_CC_SYNC_UTILS_H
#define COROUTINELIB_CC_SYNC_UTILS_H

#include "cc_api.h"
#include "cc_task.h"
#include <functional>
#include <atomic>

namespace crlib {
    struct AsyncMutexLock {
        GenericQueue<std::function<void()>> waiting_queue;
		std::atomic_bool busy;

		void append_coroutine(std::function<void()> h) {
			bool is_busy = busy.load();
			if (!is_busy && busy.compare_exchange_weak(is_busy, true)) {
				h();
			} else {
				waiting_queue.write(h);
			}
		}

		void release() {
			auto is_busy = busy.load();
			if (busy) {
				auto next = waiting_queue.read();
				if (next.has_value()) {
					next.value()();
				} else {
					busy.compare_exchange_strong(is_busy, false);
				}
			}
		}
    };

    struct AsyncMutex {
        std::shared_ptr<AsyncMutexLock> internal_lock;

		struct AsyncMutexGuard {
			std::shared_ptr<AsyncMutexLock> internal_lock;
			std::shared_ptr<std::atomic_bool> released;

			AsyncMutexGuard(std::shared_ptr<AsyncMutexLock> lock) : internal_lock(lock), released(new std::atomic_bool(false)) {

			}

			inline void release() {
				bool is_released = released->load();
				if (!is_released && released->compare_exchange_strong(is_released, true)) {
					internal_lock->release();
				}
			}

			~AsyncMutexGuard() {
				release();
			}
		};

		inline Task<AsyncMutexGuard> await() {
			co_await TaskAwaiter<AsyncMutexLock>(internal_lock);
			co_return AsyncMutexGuard(internal_lock);
		}


    };
}

#endif //COROUTINELIB_CC_SYNC_UTILS_H
