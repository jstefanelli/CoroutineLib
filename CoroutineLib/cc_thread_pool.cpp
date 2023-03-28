#include "cc_thread_pool.h"


namespace crlib {

thread_local std::shared_ptr<ThreadPool_Thread> ThreadPool::local_thread = nullptr;

CRLIB_API ThreadPool::ThreadPool() : running(true) {

}

CRLIB_API ThreadPool::~ThreadPool() {
	stop();
}

CRLIB_API std::shared_ptr<ThreadPool> ThreadPool::build(size_t thread_count) {
	std::shared_ptr<ThreadPool> self_ptr = std::shared_ptr<ThreadPool>(new ThreadPool());
	self_ptr->self_ptr = self_ptr;

	for (size_t i = 0; i < thread_count; ++i) {
		self_ptr->threads.push_back(std::shared_ptr<ThreadPool_Thread>(new ThreadPool_Thread(self_ptr)));
	}

	for (auto& t : self_ptr->threads) {

		t->start(t);
	}

	return self_ptr;
}

CRLIB_API void ThreadPool::submit(std::coroutine_handle<> h) {
	if (local_thread != nullptr && local_thread->thread_pool.get() == this) {
		if (local_thread->local_tasks->push(h)) {
			return;
		}
	}

	global_tasks_queue.push(h);

	{
		std::lock_guard lock(task_added_mutex);
		task_added_variable.notify_all();
	}
}

CRLIB_API std::optional<std::coroutine_handle<>> ThreadPool::get_work() {
	std::optional<std::coroutine_handle<>> h;
	do {
		h = global_tasks_queue.pull();

		if (h.has_value()) {
			return h.value();
		}

		for (auto stuff : queues) {
			h = stuff.second->pull();
			if (h.has_value()) {
				return h;
			}
		}

		if (!h.has_value())
		{
			auto lock = std::unique_lock<std::mutex>(task_added_mutex);
			task_added_variable.wait_for(lock, std::chrono::milliseconds(500));
		}
		else {
			return h;
		}

	} while (!h.has_value() && running);

	return std::nullopt;
}

CRLIB_API void ThreadPool::stop() {
	running = false;

	for (auto& t : threads) {
		t->join();
	}

	threads.clear();
}

CRLIB_API void ThreadPool::register_queue(std::thread::id id, QueuePtr queue) {
	queues.set(id, queue);
}

}