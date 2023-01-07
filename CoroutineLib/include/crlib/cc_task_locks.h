#ifndef COROUTINELIB_CC_TASK_LOCKS_H
#define COROUTINELIB_CC_TASK_LOCKS_H

#include <semaphore>
#include <memory>
#include <functional>
#include <optional>
#include "cc_generic_queue.h"

namespace crlib {

	struct Task_lock {
		bool completed;
		GenericQueue<std::function<void()>> waiting_coroutines;
		std::binary_semaphore wait_semaphore;
		std::optional<std::exception_ptr> exception;

		Task_lock() : wait_semaphore(0), completed(false) {

		}

		void wait() {
			if (!completed) {
				wait_semaphore.acquire();
			}

			if (exception.has_value()) {
				std::rethrow_exception(exception.value());
			}
		}

		void complete() {
			completed = true;
			wait_semaphore.release();
			std::optional<std::function<void()>> h;
			do {
				h = waiting_coroutines.read();
				if (h.has_value()) {
					h.value()();
				}

			} while (h.has_value());
		}
	};

	template<typename T>
	struct Task_lock_t {
		bool completed;
		std::optional<T> returnValue;
		GenericQueue<std::function<void()>> waiting_coroutines;
		std::binary_semaphore wait_semaphore;
		std::optional<std::exception_ptr> exception;

		Task_lock_t() : wait_semaphore(0), completed(false), returnValue() {

		}

		T wait() {
			if (!completed) {
				wait_semaphore.acquire();
			}

			if (exception.has_value()) {
				std::rethrow_exception(exception.value());
			}

			if (!returnValue.has_value()) {
				throw std::runtime_error("Task_t<T> ended with no result provided");
			}
			return returnValue.value();
		}

		void set_result(T& result) {
			returnValue = result;
		}

		void complete() {
			completed = true;
			wait_semaphore.release();
			std::optional<std::function<void()>> h;
			do {
				h = waiting_coroutines.read();
				if (h.has_value()) {
					h.value()();
				}

			} while (h.has_value());
		}
	};

	template<typename T>
	struct Generator_Lock_t {
		GenericQueue<std::function<void(std::optional<T>)>> waiting_queue;
		std::atomic_bool resume_generator = std::atomic_bool(false);
		std::optional<std::function<void()>> generator_waiter;
		std::optional<std::exception_ptr> exception;
		bool completed = false;

		std::optional<T> wait() {
			auto waiting_val = std::make_shared<std::optional<T>>();
			auto weak_val = std::weak_ptr<std::optional<T>>(waiting_val);
			auto wait_semaphore = std::make_shared<std::binary_semaphore>(0);
			auto weak_semaphore = std::weak_ptr<std::binary_semaphore>(wait_semaphore);
			if (waiting_queue.write([weak_semaphore, weak_val](std::optional<T> value) -> void {
				auto strong_val = weak_val.lock();
				if (strong_val != nullptr) {
					(*strong_val) = value;
				}

				auto strong_semaphore = weak_semaphore.lock();
				if (strong_semaphore != nullptr) {
					strong_semaphore->release();
				}
			})) {
				wait_semaphore->acquire();
				return *waiting_val;
			}

			return std::nullopt;
		}

		void wake() {
			auto should_resume = resume_generator.load();
			if (should_resume && resume_generator.compare_exchange_strong(should_resume, false, std::memory_order_seq_cst, std::memory_order_seq_cst)) {
				auto h = generator_waiter.value();
				generator_waiter = std::nullopt;
				h();
			}
		}

		void complete() {
			completed = true;
			std::optional<std::function<void(std::optional<T>)>> awaiter;
			do {
				awaiter = waiting_queue.read();
				if (awaiter.has_value()) {
					auto v = awaiter.value();
					v(std::nullopt);
				}
			} while(awaiter.has_value());
		}
	};
}

#endif //COROUTINELIB_CC_TASK_LOCKS_H
