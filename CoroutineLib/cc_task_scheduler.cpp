#include "cc_task_scheduler.h"

namespace crlib {

std::shared_ptr<BaseTaskScheduler> BaseTaskScheduler::default_task_scheduler = nullptr;
thread_local std::shared_ptr<BaseTaskScheduler> BaseTaskScheduler::current_scheduler = nullptr;

BaseTaskScheduler::BaseTaskScheduler() {

}

void BaseTaskScheduler::Schedule(std::coroutine_handle<> handle) {
    if (current_scheduler != nullptr) {
        current_scheduler->OnTaskSubmitted(handle);
    } else {
        if (default_task_scheduler == nullptr) {
            default_task_scheduler = std::make_shared<ThreadPoolTaskScheduler>();
        }
        default_task_scheduler->OnTaskSubmitted(handle);
    }
}

ThreadPoolTaskScheduler::ThreadPoolTaskScheduler() : ThreadPoolTaskScheduler(CRLIB_DEFAULT_THREAD_POOL_THREADS) {

}

ThreadPoolTaskScheduler::ThreadPoolTaskScheduler(size_t thread_amount) {
    thread_pool = ThreadPool::build(thread_amount);
}

void ThreadPoolTaskScheduler::OnTaskSubmitted(std::coroutine_handle<> handle) {
    thread_pool->submit(handle);
}

}