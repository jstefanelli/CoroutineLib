#include <thread>
#include <iostream>
#include <crlib/cc_queue_2.h>
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

bool test_generic() {
	std::cout << "[QueueTest] Running generic queue test" << std::endl;
	auto queue = std::make_shared<crlib::SingleProducerQueue<int>>(128);
	bool ok = true;
	bool alive = true;

	auto writer = std::thread([&ok, &alive, queue] {
		bool res = true;
		int i = 0;
		do {
			res = queue->write(i);
			if (res) {
				i++;
			}
		} while(i < 1024 && ok && alive);
	});

	std::vector<std::thread> readers;

	for (int t = 0; t < 16; t++) {
		readers.emplace_back([&ok, &alive, queue] {
			int last = -1;
			std::optional<int> res = std::nullopt;
			do {
				res = queue->read();
				if (!res.has_value()) {
					if (!alive) {
						break;
					}
					continue;
				}

				if (res.value() <= last) {
					std::cerr << "[Queue-Test-Generic] Last: " << last << ", got: " << res.value() << std::endl;
					ok = false;
				}
				last = res.value();

				if (last == 1023) {
					alive = false;
				}
			} while (ok);
		});
	}


	writer.join();
	for (auto& t : readers) {
		t.join();
	}

	return ok;
}

bool test_readOnce() {
	std::cout << "[QueueTest] Running non multi-read test" << std::endl;
	auto queue = std::make_shared<crlib::SingleProducerQueue<int>>(128);
	bool ok = true;
	bool alive = true;

	auto writer = std::thread([&ok, &alive, queue] {
		bool res = true;
		int i = 0;
		do {
			res = queue->write(i);
			if (res) {
				i++;
			}
		} while(i < 1024 && ok && alive);
	});

	std::vector<std::thread> readers;

	std::mutex read_values_lock;
	std::vector<std::set<int>> read_values_vector;

	for (int t = 0; t < 16; t++) {
		readers.emplace_back([t, &read_values_vector, &read_values_lock, &ok, &alive, queue] {
			std::set<int> read_values;
			std::optional<int> res = std::nullopt;
			do {
				res = queue->read();
				if (!res.has_value()) {
					if (!alive) {
						break;
					}
					continue;
				}

				int last = res.value();
				if (read_values.contains(last)) {
					ok = false;
				} else {
					read_values.insert(last);
				}

				if (last == 1023) {
					alive = false;
				}
			} while (ok);

			{
				auto g = std::lock_guard(read_values_lock);

				read_values_vector.push_back(read_values);
			}
		});
	}

	writer.join();
	for (auto& t : readers) {
		t.join();
	}

	if (!ok) {
		return false;
	}

	for (int i = 0; i < 1024; i++) {
		bool found = false;
		for(auto& set : read_values_vector) {
			if (set.contains(i)) {
				if (found) {
					std::cerr << "[QueueTest/NoMultiRead] Found duplicate: " << i << std::endl;
					ok = false;
					break;
				}
				found = true;
			}
		}
	}

	return ok;
}

int main(int argc, char** argv) {
	if (argc > 1 && std::string(argv[1]) ==  "--test-no-multi-read") {
		return test_readOnce() ? 0 : 1;
	}

	//return test_generic() ? 0 : 1;
	return test_boundless() ? 0 : 1;
}