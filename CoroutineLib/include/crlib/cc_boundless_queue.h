#pragma once

#include <memory>
#include <optional>
#include <atomic>
namespace crlib {
	template<typename T>
	struct BoundlessQueueNode;

	template<typename T>
	struct BoundlessQueueNode {
		using ptr_t = BoundlessQueueNode<T>*;
		using atomic_ptr_t = std::atomic<ptr_t>;

		std::optional<T> value;
		atomic_ptr_t next;

		BoundlessQueueNode(std::optional<T> val, ptr_t next) : value(std::move(val)), next(std::move(next)) {

		}

		~BoundlessQueueNode() {
			auto v = next.load();
			if (v != nullptr) {
				delete v;
			}
		}
	};

	template<typename T>
	struct BoundlessQueue {
		using ptr_t = BoundlessQueueNode<T>::ptr_t;
		using atomic_ptr_t = BoundlessQueueNode<T>::atomic_ptr_t;

		atomic_ptr_t head;
		atomic_ptr_t tail;

		BoundlessQueue() {
			ptr_t node = new BoundlessQueueNode<T>(std::nullopt, nullptr);
			head.store(node);
			tail.store(node);
		}

		bool push(T val) {
			ptr_t node = new BoundlessQueueNode<T>(std::move(val), nullptr);
			ptr_t t, next;
			while(true) {
				t = tail.load();
				next = t->next.load();
				if (t == tail.load()) {
					if (next == nullptr) {
						if (t->next.compare_exchange_weak(next, node)) {
							break;
						}
					} else {
						tail.compare_exchange_weak(t, next);
					}
				}
			}
			tail.compare_exchange_strong(t, node);
			return true;
		}

		std::optional<T> pull() {
			while(true) {
				ptr_t h = head.load();
				ptr_t t = tail.load();
				ptr_t next = h->next.load();
				if (h == head.load()) {
					if (h == t) {
						if (next == nullptr) {
							return std::nullopt;
						}
						tail.compare_exchange_weak(t, next);
					} else {
						auto val = next->value;
						if (head.compare_exchange_weak(h, next)) {
							h->next.store(nullptr);
							next->value = std::nullopt;
							delete h;
							return val;
						}
					}
				}
			}
		}

		~BoundlessQueue() {
			auto h = head.load();
			while (h != nullptr) {
				auto next = h->next.load();
				h->next.store(nullptr);
				delete h;
				h = next;
			}
		}
	};
}