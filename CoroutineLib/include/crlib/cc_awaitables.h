#ifndef COROUTINELIB_CC_AWAITABLES_H
#define COROUTINELIB_CC_AWAITABLES_H

#include "cc_task_locks.h"
#include "cc_task_scheduler.h"

namespace crlib {
	struct TaskAwaiter {
		std::shared_ptr<Task_lock> lock;

		TaskAwaiter(std::shared_ptr<Task_lock> lock) : lock(lock) {

		}

		bool await_ready() {
			//TODO: Check for threadpool thread
			return lock->completed;
		}

		void await_suspend(std::coroutine_handle<> h) {
			if (!lock->completed) {
				lock->waiting_coroutines.write([h] ()  {
					BaseTaskScheduler::Schedule(h);
				});
			}
			else {
				BaseTaskScheduler::Schedule(h);
			}
		}

		void await_resume() {
			if (lock->exception.has_value()) {
				std::rethrow_exception(lock->exception.value());
			}
		}
	};

	template<typename T>
	struct TaskAwaiter_t {
		std::shared_ptr<Task_lock_t<T>> lock;

		TaskAwaiter_t(std::shared_ptr<Task_lock_t<T>> lock) : lock(lock) {

		}

		bool await_ready() {
			//TODO: Check for threadpool thread
			return lock->completed;
		}

		void await_suspend(std::coroutine_handle<> h) {
			if (!lock->completed) {
				lock->waiting_coroutines.Push([h] ()  {
					BaseTaskScheduler::Schedule(h);
				});
			}
			else {
				BaseTaskScheduler::Schedule(h);
			}
		}

		T await_resume() {
			if (lock->exception.has_value()) {
				std::rethrow_exception(lock->exception.value());
			}

			if (!lock->returnValue.has_value()) {
				throw std::runtime_error("Task_t<T> ended with no result provided");
			}

			return lock->returnValue.value();
		}
	};

	struct AggregateException : public std::runtime_error {
		std::vector<std::exception_ptr> exceptions;

		AggregateException(std::vector<std::exception_ptr> exceptions) : std::runtime_error("Aggregate exceptions from tasks"), exceptions(exceptions) {

		}

		void throw_all() {
			for (auto & ptr : exceptions) {
				std::rethrow_exception(ptr);
			}
		}
	};

	struct MultiTaskAwaiter {
		struct MultiTaskAwaiter_ctrl {
			std::vector<std::shared_ptr<Task_lock>> task_locks;
			size_t tasks_count;
			std::atomic_size_t completed_tasks;
		};

		std::shared_ptr<MultiTaskAwaiter_ctrl> ctrl_block;

		MultiTaskAwaiter(std::vector<std::shared_ptr<Task_lock>> locks) {
			ctrl_block = std::make_shared<MultiTaskAwaiter_ctrl>();
			ctrl_block->tasks_count = locks.size();

			for (auto l : locks) {
				ctrl_block->task_locks.push_back(l);
			}
		}

		bool await_ready() {
			for(auto& t : ctrl_block->task_locks) {
				if (!t->completed) {
					return false;
				}
			}

			return true;
		}

		void increase_and_schedule(std::coroutine_handle<> h) {
			size_t old = ctrl_block->completed_tasks.fetch_add(1);

			if (old == ctrl_block->tasks_count - 1) {
				BaseTaskScheduler::Schedule(h);
			}
		}

		void await_suspend(std::coroutine_handle<> h) {
			for (auto& t : ctrl_block->task_locks) {
				if (t->completed) {
					increase_and_schedule(h);
				} else {
					t->waiting_coroutines.write([this, h] () {
						increase_and_schedule(h);
					});
				}
			}
		}

		void await_resume() {
			std::vector<std::exception_ptr> exceptions;

			for (auto& t : ctrl_block->task_locks) {
				if (t->exception.has_value()) {
					exceptions.push_back(t->exception.value());
				}
			}

			if (exceptions.size() == 1) {
				std::rethrow_exception(exceptions[0]);
			} else if(exceptions.size() > 1) {
				throw AggregateException(std::move(exceptions));
			}
		}
	};

	template<typename T>
	struct GeneratorTask_Awaiter {
		std::shared_ptr<Generator_Lock_t<T>> lock;
		std::optional<T> val = std::nullopt;

		GeneratorTask_Awaiter() = delete;
		explicit GeneratorTask_Awaiter(std::shared_ptr<Generator_Lock_t<T>> lock) : lock(std::move(lock)) {

		}

		bool await_ready() {
			return false;
		}

		void await_suspend(std::coroutine_handle<> h) {
			if (lock->completed) {
				//Generator task is completed, return std::nullopt
				BaseTaskScheduler::Schedule(h);
				return;
			}

			if(lock->waiting_queue.write([h, this](std::optional<T> value) -> void {
				this->val = value;
				BaseTaskScheduler::Schedule(h);
			})) {
				lock->wake();
			} else {
				//Too many items waiting in queue
				throw std::runtime_error("Coroutine queue full");
			}
		}

		std::optional<T> await_resume() {
			return val;
		}
	};

	struct TaskAwaitable {
		bool await_ready() {
			return false;
		}

		void await_suspend(std::coroutine_handle<> h) {
			BaseTaskScheduler::Schedule(h);
		}

		void await_resume() {

		}
	};
}
#endif //COROUTINELIB_CC_AWAITABLES_H
