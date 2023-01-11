//
// Created by John Stefanelli on 11/01/23.
//

#ifndef COROUTINELIB_CC_SYNC_UTILS_H
#define COROUTINELIB_CC_SYNC_UTILS_H

#include "cc_generic_queue.h"
#include "cc_awaitables.h"
#include <functional>

namespace crlib {
    struct AsyncMutexLock {
        GenericQueue<std::function<void()>> waiting_queue;
    };

    struct AsyncMutex {
        std::shared_ptr<AsyncMutexLock> lock;

        TaskAwaiter<AsyncMutexLock> await();

        void release();
    };
}

#endif //COROUTINELIB_CC_SYNC_UTILS_H
