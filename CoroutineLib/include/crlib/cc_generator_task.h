#ifndef COROUTINELIB_CC_GENERATOR_TASK_H
#define COROUTINELIB_CC_GENERATOR_TASK_H

#include "cc_task_types.h"
#include "cc_awaitables.h"
#include "cc_base_promise.h"
#include <exception>

namespace crlib {
	template<typename T>
	struct GeneratorTask_Yielder {
		std::shared_ptr<Generator_Lock_t<T>> lock;
		T val;

		GeneratorTask_Yielder() = delete;
		GeneratorTask_Yielder(std::shared_ptr<Generator_Lock_t<T>> lock, T val) : lock(std::move(lock)), val(std::move(val)) {
			
		}

		bool await_ready() {
			return false;
		}

		void await_suspend(std::coroutine_handle<> h) {
			auto awaiter = lock->waiting_queue.read();
			if (awaiter.has_value()) {
				auto func = awaiter.value();
				func(val);
				h();
			} else {
				lock->generator_waiter = [this, h]() -> void {
					auto func = lock->waiting_queue.read();
					if (func.has_value()) {
						func.value()(val);
					}
					BaseTaskScheduler::Schedule(h);
				};
				lock->resume_generator.store(true);
			}
		}

		void await_resume() {

		}
	};
}


template<typename T, typename ... Args>
struct std::coroutine_traits<crlib::GeneratorTask_t<T>, Args...> {
struct promise_type : public crlib::BasePromise<crlib::GeneratorTask_t<T>, crlib::Generator_Lock_t<T>> {
		crlib::GeneratorTask_Yielder<T> yield_value(T val) {
			return {this->lock, val};
		}

		void return_void() {

		}
	};
};


#endif //COROUTINELIB_CC_GENERATOR_TASK_H
