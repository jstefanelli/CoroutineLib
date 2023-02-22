#pragma once

#include <memory>
#include <optional>
#include <atomic>
/**
namespace crlib {
	template<typename T>
	struct BoundlessQueueItem;

	template<typename T>
	struct BoundlessQueueItem {
		using ptr_type = std::shared_ptr<BoundlessQueueItem<T>>;
		using atomic_ptr_t = std::atomic<ptr_type>;
		std::optional<T> value;
		atomic_ptr_t next;

		BoundlessQueueItem() : value(std::nullopt), next(nullptr) {

		}

		BoundlessQueueItem(const BoundlessQueueItem<T>&) = delete;
		BoundlessQueueItem& operator=(const BoundlessQueueItem<T>&) = delete;
	};

	template<typename T>
	struct BoundlessQueue {
		BoundlessQueueItem<T>::atomic_ptr_t head;
		BoundlessQueueItem<T>::atomic_ptr_t tail;

		BoundlessQueue() {
			auto item = std::make_shared<BoundlessQueueItem<T>>();
			item->value = std::nullopt;
			item->next.store(nullptr);

			head.store(item);
			tail.store(item);
		}

		BoundlessQueue(const BoundlessQueue<T>&) = delete;
		BoundlessQueue& operator=(const BoundlessQueue<T>&) = delete;

		bool push(const T& val) {
			auto item = std::make_shared<BoundlessQueueItem<T>>();
			item->value = val;
			item->next.store(nullptr);

			bool ok;
			typename BoundlessQueueItem<T>::ptr_type p;
			typename BoundlessQueueItem<T>::ptr_type c;
			typename BoundlessQueueItem<T>::ptr_type next;
			do {
				c = nullptr;
				p = tail.load();
				do {
					next = p->next.load();
					if (next != nullptr) {
						p = next;
					}
				} while(next != nullptr);
				ok = p->next.compare_exchange_weak(c, item);
			} while(!ok);

			tail.compare_exchange_weak(p, item);
			return true;
		}

		std::optional<T> pull() {
			typename BoundlessQueueItem<T>::ptr_type p = head.load(), next;
			do {
				//p = head.load();
				next = p->next.load();
				if (next == nullptr) {
					return std::nullopt;
				}
			} while(!head.compare_exchange_weak(p, next));

			auto v = next->value;
			next->value = std::nullopt;
			p->next.store(nullptr);

			return v;
		}

		constexpr bool is_full() {
			return false;
		}

		bool is_empty() {
			auto p = head.load();
			auto next = p->next.load();

			return next == nullptr;
		}
	};

}
**/

namespace crlib {
	template<typename T>
	struct BoundlessQueueNode;

	template<typename T>
	struct BoundlessQueueNode {
		using ptr_t = std::shared_ptr<BoundlessQueueNode<T>>;
		using atomic_ptr_t = std::atomic<ptr_t>;

		std::optional<T> value;
		atomic_ptr_t next;

		BoundlessQueueNode(std::optional<T> val, ptr_t next) : value(std::move(val)), next(std::move(next)) {

		}
	};

	template<typename T>
	struct BoundlessQueue {
		using ptr_t = BoundlessQueueNode<T>::ptr_t;
		using atomic_ptr_t = BoundlessQueueNode<T>::atomic_ptr_t;

		atomic_ptr_t head;
		atomic_ptr_t tail;

		BoundlessQueue() {
			ptr_t node = std::make_shared<BoundlessQueueNode<T>>(std::nullopt, nullptr);
			head.store(node);
			tail.store(node);
		}

		bool push(T val) {
			ptr_t node = std::make_shared<BoundlessQueueNode<T>>(std::move(val), nullptr);
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
							return val;
						}
					}
				}
			}
		}
	};
}