// CoroutineTest.cpp : Defines the entry point for the application.
//
#include <thread>
#include <iostream>
#include "crlib/cc_generic_queue.h"
#include "crlib/cc_task.h"
#include <sstream>

using namespace crlib;

bool ok = true;


std::thread::id CurrentThreadId() {
	return std::this_thread::get_id();
}

Task Reader_Coroutine(int i, GeneratorTask_t<int> gen) {
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

int main() {
	auto t = Writer_coroutine();

	std::vector<Task> readers;
	for (int i = 0; i < 4; i++) {
		readers.push_back(Reader_Coroutine(i, t));
	}

	([&readers]() -> Task {
		co_await WhenAll(readers);
	} )().wait();

	return 0;
}
