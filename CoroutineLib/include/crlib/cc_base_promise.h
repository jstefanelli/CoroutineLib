#ifndef COROUTINELIB_CC_BASE_PROMISE_H
#define COROUTINELIB_CC_BASE_PROMISE_H

#include "cc_task_locks.h"
#include "cc_task_types.h"
#include "cc_awaitables.h"

namespace crlib {
	template<typename TaskType, typename LockType>
	struct BasePromise {
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
		crlib::TaskAwaiter<typename LocalTaskType::Lock> await_transform(const TaskType& task) {
			return {task.lock};
		}

		crlib::MultiTaskAwaiter await_transform(crlib::MultiTaskAwaiter awaiter) {
			return awaiter;
		}

		template<typename TT>
		crlib::GeneratorTask_Awaiter<TT> await_transform(const crlib::GeneratorTask_t<TT>& task) {
			return crlib::GeneratorTask_Awaiter<TT>(task.lock);
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
