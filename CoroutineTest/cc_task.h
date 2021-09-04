#pragma once
#include <semaphore>
#include <coroutine>
#include <optional>
#include <functional>
#include "cc_queue.h"
#include "cc_task_scheduler.h"

struct Task_lock {
	bool completed;
	Concurrent_Queue_t<std::function<void()>> waiting_coroutines;
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
			h = waiting_coroutines.Pull();
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
	Concurrent_Queue_t<std::function<void()>> waiting_coroutines;
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
			h = waiting_coroutines.Pull();
			if (h.has_value()) {
				h.value()();
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
			lock->waiting_coroutines.Push([h] ()  {
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
		ctrl_block = std::shared_ptr<MultiTaskAwaiter_ctrl>(new MultiTaskAwaiter_ctrl());
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
				t->waiting_coroutines.Push([this, h] () {
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

		MultiTaskAwaiter await_transform(MultiTaskAwaiter awaiter) {
			return awaiter;
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

		MultiTaskAwaiter await_transform(MultiTaskAwaiter awaiter) {
			return awaiter;
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