#ifndef COROUTINELIB_CC_GENERATOR_TASK_H
#define COROUTINELIB_CC_GENERATOR_TASK_H

#include "cc_task_types.h"
#include "cc_awaitables.h"
#include "cc_base_promise.h"
#include <exception>
#include "cc_logger.h"

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

		template<typename PromiseType>
		void await_suspend(std::coroutine_handle<PromiseType> h) {
			while(true) {
				auto awaiter = lock->waiting_queue.pull();
				if (awaiter.has_value()) {
					auto func = awaiter.value();
					func(val);
					h();
					return;
				} else {
					auto waiter = new std::function<void()>(std::move([this, h]() -> void {
						auto func = lock->waiting_queue.pull();
						while(!func.has_value()) {
							func = lock->waiting_queue.pull();
						}
						func.value()(val);
						PromiseType::Scheduler::Schedule(h);
					}));
					std::function<void()>* f = nullptr;
					if (lock->generator_waiter.compare_exchange_strong(f, waiter)) {
						return;
					}

					delete waiter;
				}
			}
		}

		void await_resume() {

		}
	};
}


template<typename T, typename ... Args>
struct std::coroutine_traits<crlib::GeneratorTask<T>, Args...> {
struct promise_type : public crlib::BasePromise<crlib::GeneratorTask<T>, crlib::Generator_Lock_t<T>> {
		crlib::GeneratorTask_Yielder<T> yield_value(T val) {
			return {this->lock, val};
		}

		void return_void() {

		}
	};
};


#endif //COROUTINELIB_CC_GENERATOR_TASK_H
