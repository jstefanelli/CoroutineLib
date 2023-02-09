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
        default_queue<std::function<void()>> waiting_queue;
		std::atomic_bool busy;

		void append_coroutine(std::function<void()> h) {
			bool is_busy = busy.load();
			if (!is_busy && busy.compare_exchange_strong(is_busy, true)) {
				h();
			} else {
				waiting_queue.push(h);
			}
		}

		void release() {
			auto is_busy = busy.load();
			if (busy) {
				auto next = waiting_queue.pull();
				if (next.has_value()) {
					next.value()();
				} else {
					busy.compare_exchange_strong(is_busy, false);
				}
			}
		}

		AsyncMutexLock() : busy(false) {

		}
    };

    struct AsyncMutex {
        std::shared_ptr<AsyncMutexLock> internal_lock;

		struct AsyncMutexGuard {
			friend AsyncMutex;
		protected:
			std::shared_ptr<AsyncMutexLock> internal_lock;
			std::atomic_bool released;

			explicit AsyncMutexGuard(std::shared_ptr<AsyncMutexLock> lock) : internal_lock(std::move(lock)), released(false) {

			}
		public:
			AsyncMutexGuard() = delete;
			AsyncMutexGuard(const AsyncMutexGuard&) = delete;
			AsyncMutexGuard& operator=(const AsyncMutexGuard&) = delete;

			inline void release() {
				bool is_released = released.load();
				if (!is_released && released.compare_exchange_strong(is_released, true)) {
					internal_lock->release();
				}
			}

			~AsyncMutexGuard() {
				release();
			}
		};

		AsyncMutex() : internal_lock(new AsyncMutexLock()) {

		}

		ValueTask<std::unique_ptr<AsyncMutexGuard>> await() {
			co_await TaskAwaiter<AsyncMutexLock>(internal_lock);
			co_return std::unique_ptr<AsyncMutexGuard>(new AsyncMutexGuard(internal_lock));
		}
    };
}

#endif //COROUTINELIB_CC_SYNC_UTILS_H
