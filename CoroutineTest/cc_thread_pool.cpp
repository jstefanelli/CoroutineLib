#include "cc_thread_pool.h"

ThreadPool::ThreadPool() : running(true) {

}

std::shared_ptr<ThreadPool> ThreadPool::build(size_t thread_count) {
	std::shared_ptr<ThreadPool> self_ptr = std::shared_ptr<ThreadPool>(new ThreadPool());
	self_ptr->self_ptr = self_ptr;

	for (int i = 0; i < thread_count; ++i) {
		self_ptr->threads.push_back(std::shared_ptr<ThreadPool_Thread>(new ThreadPool_Thread(self_ptr)));
	}

	for (auto& t : self_ptr->threads) {
		t->start();
	}

	return self_ptr;
}

void ThreadPool::submit(std::coroutine_handle<> h) {
	global_tasks.Push(h);
	std::lock_guard lock(task_added_mutex);
	task_added_variable.notify_all();
}

std::optional<std::coroutine_handle<>> ThreadPool::get_work() {
	std::optional<std::coroutine_handle<>> h;
	do {
		h = global_tasks.Pull();

		size_t i = 0;
		while (i < threads.size() && !h.has_value()) {
			h = threads[i]->local_tasks.Pull();
			++i;
		}

		if (!h.has_value())
		{
			auto lock = std::unique_lock<std::mutex>(task_added_mutex);
			task_added_variable.wait_until(lock, std::chrono::system_clock::now() + std::chrono::milliseconds(500));
		}
		else {
			return h;
		}

	} while (!h.has_value() && running);

	return h;
}

void ThreadPool::stop() {
	running = false;

	for (auto& t : threads) {
		t->join();
	}
}