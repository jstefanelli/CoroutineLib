#include <thread>
#include <iostream>
#include <crlib/cc_queue_2.h>
#include <memory>
#include <string>
#include <set>
#include <mutex>

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

	return test_generic() ? 0 : 1;
}