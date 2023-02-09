#ifndef COROUTINELIB_CC_TASK_TYPES_H
#define COROUTINELIB_CC_TASK_TYPES_H

#include "cc_task_locks.h"
#include "cc_awaitables.h"

namespace crlib {

    template<typename T = void>
    struct Task {
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

	template<typename ... TS>
	MultiTaskAwaiter WhenAll(const TS&... tasks) {
		std::vector<Task<>> task_vector {{ tasks... }};

		std::vector<std::shared_ptr<Task<>::Lock>> locks;

		for (auto& t : task_vector) {
			locks.push_back(t.lock);
		}

		return MultiTaskAwaiter(locks);
	}


	template<typename T>
	struct GeneratorTask_t {
		std::shared_ptr<Generator_Lock_t<T>> lock;

		GeneratorTask_t() = delete;
		explicit GeneratorTask_t(std::shared_ptr<Generator_Lock_t<T>> lock) : lock(std::move(lock)) {

		}

		GeneratorTask_t(const GeneratorTask_t& other) : lock(other.lock) {

		}

		std::optional<T> wait() {
			return lock->wait();
		}
	};

	template<NotVoid T>
	struct ValueTask {
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
