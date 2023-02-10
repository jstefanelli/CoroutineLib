// CoroutineTest.cpp : Defines the entry point for the application.
//
#include <thread>
#include <iostream>
#include "crlib/cc_generic_queue.h"
#include "crlib/cc_task.h"
#include <sstream>
#include <crlib/cc_sync_utils.h>

using namespace crlib;

bool ok = true;

std::thread::id CurrentThreadId() {
	return std::this_thread::get_id();
}

Task<> Reader_Coroutine(int i, GeneratorTask_t<int> gen) {
	int last;
	std::stringstream ss;
	do {
		auto last_opt = co_await gen;
		if (last_opt.has_value()) {
			last = last_opt.value();
			ss << "[" << i << "] Received: " << last << std::endl;
			std::cout << ss.str();
			std::cout.flush();
			ss.str("");
		} else {
			ok = false;
		}
	} while (ok && last < 511);
	ok = false;
	co_return;
}

GeneratorTask_t<int> Writer_coroutine() {
	for (int i = 0; i < 512; i++) {
		co_yield i;
	}
}

bool test_yield() {
	auto t = Writer_coroutine();

	std::vector<Task<>> readers;
	for (int i = 0; i < 4; i++) {
		readers.push_back(Reader_Coroutine(i, t));
	}

	([&readers]() -> Task<> {
		co_await WhenAll(readers);
	} )().wait();

	return true;
}

bool test_async_mutex() {
	AsyncMutex mutex;

	std::atomic_bool flag(false);
	std::atomic_bool ok(true);
	std::vector<crlib::Task<>> tasks;

	for(int i = 0; i < 16; i++) {
		int ix = i + 1;
		tasks.push_back(([&mutex, &flag, &ok](int ix) -> Task<> {
			std::stringstream ss;

			ss << ix << " Coroutine Started" << std::endl;
			std::cout << ss.str();
			ss.str("");
			{
				auto guard = co_await mutex.await();
				bool f = false;
				if (!flag.compare_exchange_strong(f, true)) {
					ok.store(false);
					co_return;
				}

				ss << ix << " Coroutine In" << std::endl;
				std::cout << ss.str();
				ss.str("");
				f = true;
				if(!flag.compare_exchange_strong(f, false)) {
					ok.store(false);
					co_return;
				}
				guard->release();
			}
			ss << ix << " Coroutine Out" << std::endl;
			std::cout << ss.str();
			ss.str("");
		})(ix));
	}


	([&tasks]() -> Task<void> {
		co_await WhenAll(tasks);
	})().wait();

	return ok.load();
}

int main(int argc, char** argv) {
	if (argc > 1 && std::string(argv[1]) == "--test-async-mutex") {
		return test_async_mutex() ? 0 : 1;
	}

	return test_yield() ? 0 : 1;
}
