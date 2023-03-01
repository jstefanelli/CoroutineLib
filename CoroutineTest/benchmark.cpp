#define PICOBENCH_IMPLEMENT_WITH_MAIN
#include <picobench/picobench.hpp>
#include <crlib/cc_task.h>
#include <thread>
#include <mutex>
#include <condition_variable>

PICOBENCH_SUITE("Queue Benchmarks");

static void queue_fill_test(picobench::state& s, int n_threads) {
	std::vector<std::thread> threads;
	crlib::default_queue<int> queue;
	std::condition_variable cv;
	std::mutex mtx;

	int max = s.iterations();
	for(int i = 0; i < n_threads; i++) {
		threads.emplace_back([&queue, &mtx, &cv, max, n_threads]() -> void {

			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock);
			lock.unlock();

			for(int x = 0; x < max / n_threads; x++) {
				queue.push(x);
			}
		});
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	s.start_timer();
	cv.notify_all();

	for(auto& t : threads) {
		t.join();
	}
	s.stop_timer();
}

#define QUEUE_FILL_TEST(t) \
void queue_fill_test_##t (picobench::state& s) { \
	queue_fill_test(s, t);\
} \
PICOBENCH(queue_fill_test_##t).iterations({10000, 100000, 1000000, 10000000}).samples(2);

QUEUE_FILL_TEST(2)
QUEUE_FILL_TEST(4)
QUEUE_FILL_TEST(8)
QUEUE_FILL_TEST(16)
QUEUE_FILL_TEST(32)