#include <thread>
#include <iostream>
#include <vector>
#include <crlib/cc_boundless_queue.h>
#include <memory>
#include <string>
#include <set>
#include <mutex>

bool test_boundless() {
	std::cout << "[QueueTest] Running boundless queue test" << std::endl;

	auto q = std::make_shared<crlib::BoundlessQueue<int>>();
	std::vector<std::thread> writers;

	for(int i = 0; i < 10; i++) {
		writers.emplace_back([i, q]() {
			for (int j = 0; j < 1000; j++) {
				int v = (j * 10) + i;
				q->push(v);
			}
		});
	}

	for(auto& t : writers) {
		t.join();
	}


	std::vector<std::thread> readers;

	std::mutex lock;
	std::vector<std::set<int>> results;
	for(int i = 0; i < 10; i++) {
		readers.emplace_back([i, q, &lock, &results]() {
			std::set<int> rs;

			std::optional<int> val;
			do {
				val = q->pull();

				if (val.has_value()) {
					rs.insert(val.value());
				}
			} while(val.has_value());

			{
				auto l = std::lock_guard(lock);

				results.push_back(rs);
			}
		});
	}


	for(auto& t : readers) {
		t.join();
	}

	bool anyError = false;
	for (int i = 0; i < 10000; i++) {
		bool found = false;
		for(auto& set : results) {
			if (set.contains(i)) {
				found = true;
				break;
			}
		}

		if (!found) {
			std::cout << "[QueueTest] Missing value: " << i << std::endl;
			anyError = true;
		}
	}

	return !anyError;
}

int main(int argc, char** argv) {

	//return test_generic() ? 0 : 1;
	return test_boundless() ? 0 : 1;
}