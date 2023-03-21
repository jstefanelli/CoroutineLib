#ifndef COROUTINELIB_CC_TASK_TYPES_H
#define COROUTINELIB_CC_TASK_TYPES_H

#include "cc_task_locks.h"
#include "cc_awaitables.h"
#include "cc_task_scheduler.h"

namespace crlib {

	template<typename T>
	concept IsSchedulable = requires() {
		typename T::Scheduler;

	} && IsTaskScheduler<typename T::Scheduler>;

    template<typename T = void, IsTaskScheduler SchedulerType = ThreadPoolTaskScheduler>
    struct Task {
		using Scheduler = SchedulerType;
        using Lock = Task_lock<T>;
        std::shared_ptr<Task_lock<T>> lock;

        Task() = delete;
        Task(std::shared_ptr<Task_lock<T>> lock) : lock(lock) {

        }

        Task(const Task& other) : lock(other.lock) {

        }

        T wait() requires NotVoid<T> {
            return lock->wait();
        }

        void wait() {
            lock->wait();
        }
    };

	template<typename T = crlib::Task<>, typename ... TS>
	MultiTaskAwaiter<typename T::Lock> WhenAll(const TS&... tasks) {
		std::vector<T> task_vector {{ tasks... }};

		std::vector<std::shared_ptr<typename T::Lock>> locks;

		for (auto& t : task_vector) {
			locks.push_back(t.lock);
		}

		return MultiTaskAwaiter(std::move(locks));
	}


	template<typename T, IsTaskScheduler SchedulerType = ThreadPoolTaskScheduler>
	struct GeneratorTask {
		using Scheduler = SchedulerType;
		std::shared_ptr<Generator_Lock_t<T>> lock;

		GeneratorTask() = delete;
		explicit GeneratorTask(std::shared_ptr<Generator_Lock_t<T>> lock) : lock(std::move(lock)) {

		}

		GeneratorTask(const GeneratorTask& other) : lock(other.lock) {

		}

		std::optional<T> wait() {
			return lock->wait();
		}
	};

	template<NotVoid T, IsTaskScheduler SchedulerType = ThreadPoolTaskScheduler>
	struct ValueTask {
		using Scheduler = SchedulerType;
		std::shared_ptr<Single_Awaitable_Task_lock<T>> lock;

		ValueTask() = delete;
		explicit ValueTask(std::shared_ptr<Single_Awaitable_Task_lock<T>> lock) : lock(std::move(lock)) {

		}

		ValueTask(const ValueTask& other) : lock(other.lock) {

		}

		T wait() {
			return lock->wait();
		}
	};
}

#endif //COROUTINELIB_CC_TASK_TYPES_H
