#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <coroutine>
#include <vector>
#include "cc_api.h"
#include "cc_queue.h"

namespace crlib {

struct ThreadPool_Thread;

CRLIB_API struct ThreadPool {
public:
	static thread_local std::shared_ptr<ThreadPool_Thread> local_thread;
private:
	std::mutex task_added_mutex;
	std::condition_variable task_added_variable;
	Concurrent_Queue_t<std::coroutine_handle<>> global_tasks;
	std::vector<std::shared_ptr<ThreadPool_Thread>> threads;
	bool running;
	std::weak_ptr<ThreadPool> self_ptr;

	CRLIB_API ThreadPool();
public:
	CRLIB_API static std::shared_ptr<ThreadPool> build(size_t thread_count);
	CRLIB_API ~ThreadPool();

	CRLIB_API void submit(std::coroutine_handle<> h);

	bool is_running() {
		return this->running;
	}

	CRLIB_API std::optional<std::coroutine_handle<>> get_work();

	CRLIB_API void stop();
};

struct ThreadPool_Thread {
	friend ThreadPool;
private:
	std::unique_ptr<std::thread> self;
	std::shared_ptr<ThreadPool> thread_pool;
	Concurrent_Queue_t<std::coroutine_handle<>> local_tasks;

	void run(std::shared_ptr<ThreadPool_Thread> self_ptr) {
		ThreadPool::local_thread = self_ptr;
		
		do {
			auto h = local_tasks.Pull();
			if (!h.has_value()) {
				h = thread_pool->get_work();
			}

			if (h.has_value()) {
				h.value().resume();
			}
		} while (thread_pool->is_running());

		ThreadPool::local_thread = nullptr;
	}

	explicit ThreadPool_Thread(std::shared_ptr<ThreadPool> thread_pool) : thread_pool(thread_pool), self(nullptr) {
		
	}

	void start(std::shared_ptr<ThreadPool_Thread> self_ptr) {
		self = std::move(std::unique_ptr<std::thread>(new std::thread([this, self_ptr] () { run(self_ptr); })));
	}

	void join() {
		self->join();
	}
};

}