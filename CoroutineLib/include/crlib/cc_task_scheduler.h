#pragma once
#include <coroutine>
#include <thread>
#include <memory>
#include <optional>
#include "cc_api.h"
#include "cc_thread_pool.h"

#define CRLIB_DEFAULT_THREAD_POOL_THREADS 8U

namespace crlib {
template<typename T>
concept IsTaskScheduler = requires(T scheduler, std::coroutine_handle<> h) {
	T::Schedule(h);
	scheduler.OnTaskSubmitted(h);
};

template<typename T>
concept InlineableTaskScheduler = IsTaskScheduler<T> && requires() {
	T::CanInline();
};



struct BaseTaskScheduler {
    CRLIB_API static std::shared_ptr<BaseTaskScheduler> default_task_scheduler;
	thread_local static  std::shared_ptr<BaseTaskScheduler> current_scheduler;
    CRLIB_API static void Schedule(std::coroutine_handle<> handle);
	CRLIB_API constexpr static bool CanInline() {
		return false;
	}

    CRLIB_API BaseTaskScheduler();
	CRLIB_API virtual ~BaseTaskScheduler() = default;


    CRLIB_API virtual void OnTaskSubmitted(std::coroutine_handle<> handle) = 0;
};

struct ThreadPoolTaskScheduler : public BaseTaskScheduler {
    std::shared_ptr<ThreadPool> thread_pool;

    CRLIB_API ThreadPoolTaskScheduler();
    CRLIB_API ThreadPoolTaskScheduler(size_t thread_amount);

    CRLIB_API virtual void OnTaskSubmitted(std::coroutine_handle<> handle) override;
	CRLIB_API ~ThreadPoolTaskScheduler() override = default;
};

}