// CoroutineTest.cpp : Defines the entry point for the application.
//

#include "CoroutineTest.h"
#include <semaphore>
#include <coroutine>
#include <thread>
#include <condition_variable>
#include "crlib/cc_queue.h"
#include "crlib/cc_task.h"
#include "crlib/cc_thread_pool.h"

using namespace crlib;

std::thread::id CurrentThreadId() {
	return std::this_thread::get_id();
}

Task My_Other_Coroutine() {
	std::cout << "[" << CurrentThreadId() << "] Other coroutine" << std::endl;

	std::this_thread::sleep_for(std::chrono::seconds(5));

	std::cout << "[" << CurrentThreadId() << "] Other coroutine slept" << std::endl;

	co_return;
}

Task_t<int> My_Coroutine() {
	std::cout << "[" << CurrentThreadId() << "] Coroutine start." << std::endl;

	Task t1 = My_Other_Coroutine();
	Task t2 = My_Other_Coroutine();
	Task t3 = My_Other_Coroutine();

	co_await WhenAll(t1, t2, t3, [] () -> Task {
		std::cout << "["  << CurrentThreadId() << "] Coroutine Lambda!" << std::endl;
		co_return;
	}());

	std::cout << "[" << CurrentThreadId() << "] Resumed" << std::endl;
	co_return 49;
}

int main()
{
	std::cout << "[" << CurrentThreadId() << "] Main Thread" << std::endl;
	auto task = My_Coroutine();

	try {
		int x = task.wait();
		std::cout << "["  << CurrentThreadId() << "] Return value: " << x << std::endl;
	}
	catch (int x) {
		std::cout << "[" << CurrentThreadId() << "] Nested coroutine exception: " << x << std::endl;
	}

	std::cout << "[" << CurrentThreadId() << "] Continuing." << std::endl;
	return 0;
}
