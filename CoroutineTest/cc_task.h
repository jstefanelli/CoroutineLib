#pragma once
#include <semaphore>
#include <coroutine>
#include <optional>
#include "cc_queue.h"
#include "cc_task_scheduler.h"

extern void Schedule(std::coroutine_handle<> h);

struct Task_lock {
	bool completed;
	Concurrent_Queue_t<std::coroutine_handle<>> waiting_coroutines;
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
		std::optional<std::coroutine_handle<>> h;
		do {
			h = waiting_coroutines.Pull();
			if (h.has_value()) {
				Schedule(h.value());
			}

		} while (h.has_value());
	}
};

template<typename T>
struct Task_lock_t {
	bool completed;
	std::optional<T> returnValue;
	Concurrent_Queue_t<std::coroutine_handle<>> waiting_coroutines;
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
		std::optional<std::coroutine_handle<>> h;
		do {
			h = waiting_coroutines.Pull();
			if (h.has_value()) {
				Schedule(h.value());
			}

		} while (h.has_value());
	}
};

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
			lock->waiting_coroutines.Push(h);
		}
		else {
			Schedule(h);
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
			lock->waiting_coroutines.Push(h);
		}
		else {
			Schedule(h);
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

struct TaskAwaitable {
	bool await_ready() {
		return false;
	}

	void await_suspend(std::coroutine_handle<> h) {
		Schedule(h);
	}

	void await_resume() {

	}
};


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

template<typename ... Args>
struct std::coroutine_traits<Task, Args...> {
	struct promise_type {
		std::shared_ptr<Task_lock> lock;

		promise_type() : lock(new Task_lock()) {

		}

		Task get_return_object() {
			return Task(lock);
		}

		TaskAwaitable initial_suspend() {
			return {};
		}

		TaskAwaiter await_transform(Task task) {
			return TaskAwaiter(task.lock);
		}

		template<typename TT>
		TaskAwaiter_t<TT> await_transform(Task_t<TT> task) {
			return TaskAwaiter_t<TT>(task.lock);
		}

		std::suspend_never final_suspend() noexcept {
			lock->complete();
			return {};
		}

		void return_void() {}
		void unhandled_exception() {
			lock->exception = std::current_exception();
		}

	};
};

template<typename T, typename ... Args>
struct std::coroutine_traits<Task_t<T>, Args...> {
	struct promise_type {
		std::shared_ptr<Task_lock_t<T>> lock;

		promise_type() : lock(new Task_lock_t<T>()) {

		}

		Task_t<T> get_return_object() {
			return Task_t<T>(lock);
		}

		TaskAwaitable initial_suspend() {
			return {};
		}

		TaskAwaiter await_transform(Task task) {
			return TaskAwaiter(task.lock);
		}

		template<typename TT>
		TaskAwaiter_t<TT> await_transform(Task_t<TT> task) {
			return TaskAwaiter_t<TT>(task.lock);
		}

		std::suspend_never final_suspend() noexcept {
			lock->complete();
			return {};
		}

		void return_value(T val) {
			lock->set_result(val);
		}

		void unhandled_exception() {
			lock->exception = std::current_exception();
		}

	};
};