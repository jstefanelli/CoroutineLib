#pragma once
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <coroutine>
#include <vector>
#include <queue>
#include <iostream>
#include "cc_api.h"
#include "cc_dictionary.h"
#include "cc_queue_config.h"

namespace crlib {

struct ThreadPool_Thread;

#ifndef CRLIB_LOCAL_QUEUE_SIZE
#define CRLIB_LOCAL_QUEUE_SIZE 1024
#endif

struct ThreadPool {
public:
	static thread_local std::shared_ptr<ThreadPool_Thread> local_thread;
	using Queue_t = default_queue<std::coroutine_handle<>>;
	using QueuePtr = std::shared_ptr<default_queue<std::coroutine_handle<>>>;
private:

	std::mutex task_added_mutex;
	std::condition_variable task_added_variable;
	std::vector<std::shared_ptr<ThreadPool_Thread>> threads;
	ConcurrentDictionary<std::thread::id, QueuePtr> queues;
	default_queue<std::coroutine_handle<>> global_tasks_queue;
	bool running;
	std::weak_ptr<ThreadPool> self_ptr;

	CRLIB_API ThreadPool();
public:
	CRLIB_API static std::shared_ptr<ThreadPool> build(size_t thread_count);
	CRLIB_API ~ThreadPool();

	CRLIB_API void submit(std::coroutine_handle<> h);
	CRLIB_API void register_queue(std::thread::id id, QueuePtr queue);

	CRLIB_API bool is_running() {
		return this->running;
	}

	CRLIB_API std::optional<std::coroutine_handle<>> get_work();

	CRLIB_API void stop();
};

struct ThreadPool_Thread {
	friend ThreadPool;
private:

	std::shared_ptr<ThreadPool> thread_pool;
	std::unique_ptr<std::thread> self;
	ThreadPool::QueuePtr local_tasks;

	void run(std::shared_ptr<ThreadPool_Thread> self_ptr) {
		ThreadPool::local_thread = self_ptr;
		thread_pool->register_queue(std::this_thread::get_id(), local_tasks);
		
		do {
			auto h = local_tasks->pull();
			if (!h.has_value()) {
				h = thread_pool->get_work();
			}

			if (h.has_value()) {
				h.value().resume();
			}
		} while (thread_pool->is_running());

		ThreadPool::local_thread = nullptr;
	}

	explicit ThreadPool_Thread(std::shared_ptr<ThreadPool> thread_pool) : thread_pool(std::move(thread_pool)), self(nullptr) {
		local_tasks = std::make_shared<ThreadPool::Queue_t>();
	}

	void start(std::shared_ptr<ThreadPool_Thread> self_ptr) {
		self = std::move(std::make_unique<std::thread>([this, self_ptr] () { run(self_ptr); }));
	}

	void join() {
		self->join();
	}
};

}