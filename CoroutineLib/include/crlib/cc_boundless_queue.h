#pragma once

#include <memory>
#include <optional>
#include <atomic>

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