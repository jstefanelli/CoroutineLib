#ifndef COROUTINELIB_CC_TASK_TYPES_H
#define COROUTINELIB_CC_TASK_TYPES_H

#include "cc_task_locks.h"
#include "cc_awaitables.h"

namespace crlib {

	struct Task {
		std::shared_ptr<Task_lock> lock;

		Task() = delete;
		Task(std::shared_ptr<Task_lock> lock) : lock(lock) {

		}

		Task(const Task& other) : lock(other.lock) {

		}

		void wait() {
			lock->wait();
		}
	};

	template<typename T>
	struct Task_t {
		std::shared_ptr<Task_lock_t<T>> lock;

		Task_t() = delete;
		Task_t(std::shared_ptr<Task_lock_t<T>> lock) : lock(lock) {

		}

		Task_t(const Task_t& other) : lock(other.lock) {

		}

		T wait() {
			return lock->wait();
		}
	};

	template<typename ... TS>
	MultiTaskAwaiter WhenAll(const TS&... tasks) {
		std::vector<Task> task_vector {{ tasks... }};

		std::vector<std::shared_ptr<Task_lock>> locks;

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
}

#endif //COROUTINELIB_CC_TASK_TYPES_H
