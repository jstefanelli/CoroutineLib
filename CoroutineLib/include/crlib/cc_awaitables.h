#ifndef COROUTINELIB_CC_AWAITABLES_H
#define COROUTINELIB_CC_AWAITABLES_H

#include "cc_task_locks.h"
#include "cc_task_scheduler.h"

namespace crlib {

    template<typename T>
    concept Awaitable = requires (T a, std::coroutine_handle<> handle) {
        {a.await_ready()} -> std::same_as<bool>;
        a.await_suspend(handle);
        a.await_resume();
    };

    template<typename T>
    concept HasLock = requires (T a) {
        T::Lock -> Lockable;
        {a.lock} -> Lockable<>;
    };

    template<Lockable LockType>
	struct TaskAwaiter {
		std::shared_ptr<LockType> lock;

		TaskAwaiter(std::shared_ptr<LockType> lock) : lock(lock) {

		}

		bool await_ready() requires EarlyLockable<LockType> {
			return lock->completed;
		}

        bool await_ready() {
            return false;
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

        inline void rethrow_exception() requires ExceptionHolder<LockType> {
            if (lock->exception.has_value()) {
                std::rethrow_exception(lock->exception.value());
            }
        }

        inline void rethrow_exception() {

        }

        template<typename V>
        V await_resume() requires ValueHolder<LockType> {
            rethrow_exception();

            if(!lock->returnValue.has_value()) {
                throw std::runtime_error("ValueLock unlocked with no value");
            }

            return lock->returnValue.value();
        }

		void await_resume() {
			rethrow_exception();
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
			std::vector<std::shared_ptr<Task_lock<void>>> task_locks;
			size_t tasks_count;
			std::atomic_size_t completed_tasks;
		};

		std::shared_ptr<MultiTaskAwaiter_ctrl> ctrl_block;

		MultiTaskAwaiter(std::vector<std::shared_ptr<Task_lock<void>>> locks) {
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
