#include <crlib/cc_task.h>
#include <chrono>

struct MyCustomScheduler;

struct MyCustomScheduler {
	static std::shared_ptr<MyCustomScheduler> da_scheduler;
	crlib::default_queue<std::coroutine_handle<>> queue;

	bool alive;

	MyCustomScheduler() : alive(true) {

	}

	static void Schedule(std::coroutine_handle<> h) {
		if (da_scheduler == nullptr) {
			da_scheduler = std::make_shared<MyCustomScheduler>();
		}

		da_scheduler->OnTaskSubmitted(h);
	}

	void OnTaskSubmitted(std::coroutine_handle<> h) {
		queue.push(h);
	}

	void run() {
		while(alive) {
			auto t = queue.pull();

			if (t.has_value()) {
				t.value()();
			}
		}
	}
};

std::shared_ptr<MyCustomScheduler> MyCustomScheduler::da_scheduler;

int main(int argc, char** argv) {
	auto s = std::make_shared<MyCustomScheduler>();
	MyCustomScheduler::da_scheduler = s;

	bool ok = true;
	auto id = std::this_thread::get_id();

	([&ok, &id]() -> crlib::Task<void, MyCustomScheduler> {
		auto task_id = std::this_thread::get_id();

		if (task_id != id) {
			ok = false;
			std::cerr << "[Task] Main task running on the wrong thread" << std::endl;
			co_return;
		}

		co_await ([task_id, &ok]() -> crlib::Task<> {
			auto second_task_id = std::this_thread::get_id();
			if(second_task_id == task_id) {
				std::cout << "[Task2] Running on the same thread!" << std::endl;
				ok = false;
			}

			std::this_thread::sleep_for(std::chrono::seconds(1));
			co_return;
		})();

		task_id = std::this_thread::get_id();

		if (task_id != id) {
			ok = false;
			std::cerr << "[Task] Main task resumed on the wrong thread" << std::endl;
			co_return;
		}

		MyCustomScheduler::da_scheduler->alive = false;

		co_return;
	})();

	s->run();

	return ok ? 0 : 1;
}