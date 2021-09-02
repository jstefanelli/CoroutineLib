#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <coroutine>
#include <vector>
#include "cc_queue.h"

struct ThreadPool_Thread;

struct ThreadPool {
public:
	static thread_local std::shared_ptr<ThreadPool_Thread> local_thread;
private:
	std::mutex task_added_mutex;
	std::condition_variable task_added_variable;
	Concurrent_Queue_t<std::coroutine_handle<>> global_tasks;
	std::vector<std::shared_ptr<ThreadPool_Thread>> threads;
	bool running;
	std::weak_ptr<ThreadPool> self_ptr;

	ThreadPool();
public:
	static std::shared_ptr<ThreadPool> build(size_t thread_count);

	void submit(std::coroutine_handle<> h);

	bool is_running() {
		return this->running;
	}

	std::optional<std::coroutine_handle<>> get_work();

	void stop();
};

struct ThreadPool_Thread {
	friend ThreadPool;
private:
	std::thread self;
	std::shared_ptr<ThreadPool> thread_pool;
	Concurrent_Queue_t<std::coroutine_handle<>> local_tasks;

	void run() {
		do {
			auto h = local_tasks.Pull();
			if (!h.has_value()) {
				h = thread_pool->get_work();
			}

			if (h.has_value()) {
				h.value().resume();
			}
		} while (thread_pool->is_running());
	}

	ThreadPool_Thread(std::shared_ptr<ThreadPool> thread_pool) : thread_pool(thread_pool) {
		
	}

	void start() {
		self = std::thread([this] () { run(); });
	}

	void join() {
		self.join();
	}
};