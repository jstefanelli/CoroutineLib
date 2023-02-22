#ifndef COROUTINELIB_CC_TASK_LOCKS_H
#define COROUTINELIB_CC_TASK_LOCKS_H

#include <semaphore>
#include <memory>
#include <functional>
#include <optional>
#include "cc_queue_config.h"
#include <concepts>
#include <stdexcept>
#include "cc_logger.h"

namespace crlib {
	template<typename T>
	concept NotVoid = !std::same_as<T, void>;

	template<typename T>
	concept Lockable = requires(T a, std::function<void()> func) {
		typename T::ValueType;
		a.append_coroutine(func);
	};

	template<typename T>
	concept EarlyLockable = Lockable<T> && requires(T a) {
		{ a.completed.load() } -> std::convertible_to<bool>;
	};


	template<typename T>
	concept ValueHolder = requires(T a) {
		a.returnValue;
	};

	template<typename T>
	concept ExceptionHolder = requires(T a) {
		a.exception;
	};

	struct BaseLock {
		std::atomic_bool completed;
		default_queue<std::function<void()>> waiting_coroutines;
		std::binary_semaphore wait_semaphore;
		std::optional<std::exception_ptr> exception;

		BaseLock() : wait_semaphore(0), completed(false) {

		}

		void complete() {
			completed.store(true);
			wait_semaphore.release();
			std::optional<std::function<void()>> h;
			do {
				h = waiting_coroutines.pull();
				if (h.has_value()) {
					h.value()();
				}

			} while (h.has_value());
		}

		void append_coroutine(std::function<void()> f) {
			if (completed.load()) {
				f();
				return;
			}
			waiting_coroutines.push(f);
		}
	};

	template<NotVoid T>
	struct Single_Awaitable_Task_lock {
		using ValueType = T;
		T value;
		std::atomic_bool has_value;
		bool completed;
		std::optional<std::function<void()>> waiting_coroutine;
		std::atomic_bool has_awaiter;
		std::optional<std::exception_ptr> exception;

		Single_Awaitable_Task_lock() : has_value(false), has_awaiter(false), completed(false) {

		}

		bool add_awaiter(std::function<void()> awaiter) {
			bool v = false;
			if (has_awaiter.compare_exchange_weak(v, true)) {
				waiting_coroutine = awaiter;
				return true;
			}

			return false;
		}

		void set_result(T result) {
			bool v = false;
			if (has_value.compare_exchange_weak(v, true)) {
				value = std::move(result);
			}
		}

		void complete() {
			completed = true;
			if (waiting_coroutine.has_value()) {
				auto c = waiting_coroutine.value();
				c();
			}
		}
	};

	template<typename T>
	struct Task_lock : public BaseLock {
		using ValueType = T;
		std::optional<T> returnValue;

		Task_lock() : BaseLock(), returnValue(std::nullopt) {

		}

		T wait() requires NotVoid<T> {
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


		void set_result(T result) requires NotVoid<T> {
			returnValue = std::move(result);
		}
	};

	template<std::same_as<void> T>
	struct Task_lock<T> : public BaseLock {
		using ValueType = T;

		Task_lock() = default;

		void wait() {
			if (!completed) {
				wait_semaphore.acquire();
			}

			if (exception.has_value()) {
				std::rethrow_exception(exception.value());
			}
		}
	};

	template<typename T>
	struct Generator_Lock_t {
		using ValueType = T;
		default_queue<std::function<void(std::optional<T>)>> waiting_queue;

		std::atomic<std::shared_ptr<std::function<void()>>> generator_waiter;
		std::optional<std::exception_ptr> exception;
		std::atomic_bool completed = std::atomic_bool(false);

		std::optional<T> wait() {
			auto waiting_val = std::make_shared<std::optional<T>>();
			auto weak_val = std::weak_ptr<std::optional<T>>(waiting_val);
			auto wait_semaphore = std::make_shared<std::binary_semaphore>(0);
			auto weak_semaphore = std::weak_ptr<std::binary_semaphore>(wait_semaphore);
			if (waiting_queue.push([weak_semaphore, weak_val](std::optional<T> value) -> void {
				auto strong_val = weak_val.lock();
				if (strong_val != nullptr) {
					strong_val.swap(value);
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
			auto should_resume = generator_waiter.load();
			if (should_resume != nullptr &&
			generator_waiter.compare_exchange_strong(should_resume, nullptr)) {
				should_resume->operator()();
			}

			if (completed.load()) {
				auto w = waiting_queue.pull();
				while (w.has_value()) {
					w.value()(std::nullopt);
					w = waiting_queue.pull();
				}
			}
		}

		void complete() {

			completed.store(true);
			auto awaiter = waiting_queue.pull();
			while (awaiter.has_value()) {
				awaiter.value()(std::nullopt);

				awaiter = waiting_queue.pull();
			}
		}
	};
}


#endif //COROUTINELIB_CC_TASK_LOCKS_H
