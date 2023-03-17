#include <thread>
#include <iostream>
#include <vector>
#include <crlib/cc_boundless_queue.h>
#include <memory>
#include <string>
#include <set>
#include <mutex>

#define writers_amount 10

bool test_boundless() {
	std::cout << "[QueueTest] Running boundless queue test" << std::endl;

	auto q = std::make_shared<crlib::BoundlessQueue<int>>();
	std::vector<std::shared_ptr<std::thread>> writers;


	for(int i = 0; i < writers_amount; i++) {
		writers.emplace_back(new std::thread([i, q]() {
			for (int j = 0; j < 1000; j++) {
				int v = (j * writers_amount) + i;
				q->push(v);
			}
		}));
	}

	for(auto& t : writers) {
		t->join();
	}


	std::vector<std::shared_ptr<std::thread>> readers;

	std::mutex lock;
	std::shared_ptr<std::vector<std::set<int>>> results = std::make_shared<std::vector<std::set<int>>>();
	for(int i = 0; i < 10; i++) {
		readers.emplace_back(new std::thread([i, q, &lock, &results]() {
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

				results->push_back(rs);
			}
		}));
	}


	for(auto& t : readers) {
		t->join();
	}

	bool anyError = false;
	for (int i = 0; i < 10000; i++) {
		bool found = false;
		for(auto& set : *results) {
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