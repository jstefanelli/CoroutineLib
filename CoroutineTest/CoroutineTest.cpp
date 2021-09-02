// CoroutineTest.cpp : Defines the entry point for the application.
//

#include "CoroutineTest.h"
#include <semaphore>
#include <coroutine>
#include <thread>
#include <condition_variable>
#include "cc_queue.h"
#include "cc_task.h"
#include "cc_thread_pool.h"

Concurrent_Queue_t<std::coroutine_handle<>> tasks;
bool alive = true;
std::mutex task_mutex;
std::condition_variable task_semaphore;

void Schedule(std::coroutine_handle<> h) {
	tasks.Push(h);
	task_semaphore.notify_all();
}

void handler_run() {
	do {
		auto h = tasks.Pull();
		if (!h.has_value()) {
			{
				auto lock = std::unique_lock<std::mutex>(task_mutex);
				task_semaphore.wait_until(lock, std::chrono::system_clock::now() + std::chrono::milliseconds(500));
			}
		}
		else {
			h.value().resume();
		}

	} while (alive);
}


Task_t<int> My_Other_Coroutine() {
	std::cout << "Other coroutine" << std::endl;
	co_return 5;
}

Task_t<int> My_Coroutine() {
	std::cout << "Coroutine start." << std::endl;

	int x = co_await My_Other_Coroutine();

	std::cout << "Resumed" << std::endl;
	co_return 49 + x;
}

int main()
{
	std::thread th(handler_run);

	auto task = My_Coroutine();

	try {
		int x = task.wait();
		std::cout << "Return value: " << x << std::endl;
	}
	catch (int x) {
		std::cout << "Nested coroutine exception: " << x << std::endl;
	}

	std::cout << "Continuing." << std::endl;

	alive = false;
	th.join();
	return 0;
}
