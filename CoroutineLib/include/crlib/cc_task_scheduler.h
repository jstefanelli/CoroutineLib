#pragma once
#include <coroutine>
#include <thread>
#include <memory>
#include <optional>
#include "cc_api.h"
#include "cc_queue.h"
#include "cc_thread_pool.h"

#define CRLIB_DEFAULT_THREAD_POOL_THREADS 8U

namespace crlib {

CRLIB_API struct BaseTaskScheduler {
    static std::shared_ptr<BaseTaskScheduler> default_task_scheduler;
    static thread_local std::shared_ptr<BaseTaskScheduler> current_scheduler;
	static void Schedule(std::coroutine_handle<> handle);

    BaseTaskScheduler();

    virtual void OnTaskSubmitted(std::coroutine_handle<> handle) = 0;
};

CRLIB_API struct ThreadPoolTaskScheduler : public BaseTaskScheduler {
    std::shared_ptr<ThreadPool> thread_pool;

    ThreadPoolTaskScheduler();
    ThreadPoolTaskScheduler(size_t thread_amount);

    virtual void OnTaskSubmitted(std::coroutine_handle<> handle) override;
};

}