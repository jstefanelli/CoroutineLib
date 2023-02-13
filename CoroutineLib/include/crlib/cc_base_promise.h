#ifndef COROUTINELIB_CC_BASE_PROMISE_H
#define COROUTINELIB_CC_BASE_PROMISE_H

#include "cc_task_locks.h"
#include "cc_task_types.h"
#include "cc_awaitables.h"

namespace crlib {
	template<IsSchedulable TaskType, typename LockType>
	struct BasePromise {
		using Scheduler = typename TaskType::Scheduler;
		std::shared_ptr<LockType> lock;

		BasePromise() : lock(new LockType()) {

		}

		TaskType get_return_object() {
			return TaskType(lock);
		}

		crlib::TaskAwaitable initial_suspend() {
			return {};
		}

        template<HasLock LocalTaskType>
		crlib::TaskAwaiter<typename LocalTaskType::Lock> await_transform(const LocalTaskType& task) {
			return {task.lock};
		}

		template<Awaitable T>
		T await_transform(const T& t) {
			return t;
		}

		template<typename TT, IsTaskScheduler Scheduler>
		crlib::GeneratorTask_Awaiter<TT> await_transform(const crlib::GeneratorTask<TT, Scheduler>& task) {
			return crlib::GeneratorTask_Awaiter<TT>(task.lock);
		}

		template<typename TT, IsTaskScheduler Scheduler>
		crlib::ValueTaskAwaiter<TT> await_transform(const crlib::ValueTask<TT, Scheduler>& task) {
			return crlib::ValueTaskAwaiter<TT>(task.lock);
		}

		std::suspend_never final_suspend() noexcept {
			lock->complete();
			return {};
		}

		void unhandled_exception() {
			lock->exception = std::current_exception();
		}
	};
}

#endif //COROUTINELIB_CC_BASE_PROMISE_H
