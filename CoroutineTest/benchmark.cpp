#define PICOBENCH_IMPLEMENT_WITH_MAIN
#include <picobench/picobench.hpp>
#include <crlib/cc_task.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

static void queue_fill_atomic(picobench::state& s, int n_threads) {
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

static void queue_fill_sync(picobench::state& s, int n_threads) {
	std::vector<std::thread> threads;
	std::queue<int> queue;
	std::condition_variable cv;
	std::mutex mtx, queue_mtx;

	int max = s.iterations();
	for(int i = 0; i < n_threads; i++) {
		threads.emplace_back([&queue, &mtx, &cv, &queue_mtx, max, n_threads]() -> void {

			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock);
			lock.unlock();

			for(int x = 0; x < max / n_threads; x++) {
				std::lock_guard g(queue_mtx);
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

#define ITERATIONS {1000, 10000, 1000000, 10000000 }
#define SAMPLES 2

#define QUEUE_FILL_ATOMIC(t) \
void queue_fill_atomic_##t (picobench::state& s) { \
	queue_fill_atomic(s, t);\
} \
PICOBENCH(queue_fill_atomic_##t).iterations(ITERATIONS).samples(SAMPLES)

#define QUEUE_FILL_SYNC(t) \
void queue_fill_sync_##t (picobench::state& s) { \
	queue_fill_sync(s, t);\
} \
PICOBENCH(queue_fill_sync_##t).iterations(ITERATIONS).samples(SAMPLES)

#define QUEUE_FILL(t) \
QUEUE_FILL_SYNC(t);   \
QUEUE_FILL_ATOMIC(t);

PICOBENCH_SUITE("Fill: 1 Thread");
QUEUE_FILL_SYNC(1).baseline();
QUEUE_FILL_ATOMIC(1);

PICOBENCH_SUITE("Fill: 2 Threads");
QUEUE_FILL_SYNC(2).baseline();
QUEUE_FILL_ATOMIC(2);

PICOBENCH_SUITE("Fill: 4 Threads");
QUEUE_FILL_SYNC(4).baseline();
QUEUE_FILL_ATOMIC(4);

PICOBENCH_SUITE("Fill: 8 Threads");
QUEUE_FILL_SYNC(8).baseline();
QUEUE_FILL_ATOMIC(8);

PICOBENCH_SUITE("Fill: 16 Threads");
QUEUE_FILL_SYNC(16).baseline();
QUEUE_FILL_ATOMIC(16);

PICOBENCH_SUITE("Fill: 32 Threads");
QUEUE_FILL_SYNC(32).baseline();
QUEUE_FILL_ATOMIC(32);