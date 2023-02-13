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
		AsyncMutex(const AsyncMutex&) = delete;
		AsyncMutex& operator=(const AsyncMutex&) = delete;

		ValueTask<std::unique_ptr<AsyncMutexGuard>> await() {
			co_await TaskAwaiter<AsyncMutexLock>(internal_lock);
			co_return std::unique_ptr<AsyncMutexGuard>(new AsyncMutexGuard(internal_lock));
		}
    };

	struct AsyncConditionVariableLock {
		default_queue<std::function<void()>> queue;

		void append_coroutine(std::function<void()> f) {
			queue.push(f);
		}

		void notify_one() {
			auto v = queue.pull();
			if (v.has_value()) {
				v.value()();
			}
		}

		void notify_all() {
			auto v = queue.pull();
			while(v.has_value()) {
				v.value()();

				v = queue.pull();
			}
		}
	};

	struct AsyncConditionVariable {
	protected:
		std::shared_ptr<AsyncConditionVariableLock> lock;
	public:
		AsyncConditionVariable() : lock(new AsyncConditionVariableLock()) {

		}

		AsyncConditionVariable(const AsyncConditionVariable&) = delete;
		AsyncConditionVariable& operator=(const AsyncConditionVariable&) = delete;

		TaskAwaiter<AsyncConditionVariableLock> await() {
			return {lock};
		}

		void notify_all() {
			lock->notify_all();
		}

		void notify_one() {
			lock->notify_one();
		}
	};
}

#endif //COROUTINELIB_CC_SYNC_UTILS_H
