#ifndef COROUTINELIB_CC_DICTIONARY_H
#define COROUTINELIB_CC_DICTIONARY_H

#include <memory>
#include <optional>
#include <vector>
#include <mutex>
#include <algorithm>

#undef min
#undef max

namespace crlib {
	template<typename K, typename V>
	struct ConcurrentDictionaryEntry {
		using entry_t = ConcurrentDictionaryEntry<K, V>;

		K key;
		V value;
		entry_t* next = nullptr;

		ConcurrentDictionaryEntry() = delete;
		ConcurrentDictionaryEntry(const K& key, const V& value) : key(key), value(value) {

		}

		~ConcurrentDictionaryEntry() {
			if (next != nullptr) {
				delete next;
			}
		}
	};

	template<typename K, typename V>
	class ConcurrentDictionary {
	public:
		using entry_t = ConcurrentDictionaryEntry<K, V>;
	protected:
		using buckets_t = std::shared_ptr<std::vector<entry_t*>>;

		buckets_t buckets;
		std::vector<std::mutex> locks;

		size_t get_bucket_idx(const K& key, size_t buckets_n) {
			return static_cast<size_t>(std::hash<K>()(key)) % buckets_n;
		}

		static size_t append_entry(buckets_t buckets, size_t bucket_idx, entry_t* new_entry) {
			size_t i = 0;

			if ((*buckets)[bucket_idx] == nullptr) {
				(*buckets)[bucket_idx] = new_entry;
				return 1;
			}

			auto tail = (*buckets)[bucket_idx];

			while (tail != nullptr && tail->next != nullptr) {
				i++;
				tail = tail->next;
			}

			tail->next = std::move(new_entry);
			i++;

			return i;
		}

		void expand() noexcept {
			auto bks = buckets;

			if (bks->size() >= locks.size()) {
				return;
			}

			locks[0].lock();

			if (bks != buckets) {
				locks[0].unlock();
				return;
			}


			auto next_size = std::min(bks->size() * 2U, locks.size());

			auto next_buckets = std::make_shared<std::vector<entry_t*>>(next_size);

			for(size_t i = 1; i < locks.size(); i++) {
				locks[i].lock();
			}

			for(auto& head : (*bks)) {
				auto head_val = head;
				if (head_val == nullptr) {
					continue;
				}

				auto val = head_val;

				while(val != nullptr) {
					auto idx = get_bucket_idx(val->key, next_size);
					auto nval = new entry_t(*val);
					nval->next = nullptr;
					append_entry(next_buckets, idx, nval);
					val = val->next;
				}
			}

			buckets = std::move(next_buckets); //This should be atomic, if it isn't, big trouble

			for(auto& l : locks) {
				l.unlock();
			}
		}

		struct iterator_point {
			buckets_t buckets;
			size_t bucket_idx;
			entry_t* entry;
		};
	public:

		struct Iterator {
		private:
			iterator_point pt;
		public:
			using iterator_category = std::input_iterator_tag;
			using value_type = std::pair<K, V>;

			Iterator() = default;
			explicit Iterator(iterator_point p) {
				pt = p;
			}

			value_type operator*() const {
				return {pt.entry->key, pt.entry->value};
			}

			Iterator& operator++() {
				if (pt.entry->next != nullptr) {
					pt.entry = pt.entry->next;
					return *this;
				}

				size_t i = pt.bucket_idx + 1;
				while(i < pt.buckets->size()) {
					if ((*pt.buckets)[i] != nullptr) {
						pt.bucket_idx = i;
						pt.entry = (*pt.buckets)[i];
						return *this;
					}
					i++;
				}

				pt.entry = nullptr;
				return *this;
			}

			friend bool operator!= (const Iterator& a, const Iterator& b) {
				return a.pt.entry != b.pt.entry;
			}

		};


		explicit ConcurrentDictionary(size_t initial_buckets_n = 64, size_t max_buckets_n = 1024) :
				buckets(std::make_shared<std::vector<entry_t*>>(std::max<size_t>(initial_buckets_n, 1))),
		locks(std::max(max_buckets_n, std::max<size_t>(initial_buckets_n, 64))) {
		}

		Iterator begin() {
			for (size_t i = 0; i < buckets->size(); i++) {
				if ((*buckets)[i] != nullptr) {
					return  Iterator(iterator_point{buckets, i, (*buckets)[i]});
				}
			}

			return end();
		}

		Iterator end() {
			return Iterator(iterator_point{buckets, 0, nullptr});
		}

		void set(const K& key, const V& val) {
			auto entry = new entry_t(key, val);
			entry->next = nullptr;

			while (true) {
				auto bks = buckets;
				auto idx = get_bucket_idx(key, bks->size());
				auto lock_idx = idx % locks.size();

				size_t i = 0;
				{
					std::lock_guard lock(locks[lock_idx]);

					if (bks != buckets) {
						continue;
					}

					auto current_entry = (*bks)[idx];

					while(current_entry != nullptr) {
						if (current_entry->key == key) {
							current_entry->value = val;
							return;
						}
						current_entry = current_entry->next;
					}

					i = append_entry(buckets, idx, entry);
				}

				if (i >= bks->size()) {
					expand();
				}
				break;
			}
		}

		bool erase(const K& key) {
			while(true) {
				auto bks = buckets;
				auto idx = get_bucket_idx(key, bks->size());
				auto lock_idx = idx % locks.size();

				std::lock_guard lock(locks[lock_idx]);

				if (bks != buckets) {
					continue;
				}

				auto prev = (*bks)[idx];

				if (prev == nullptr) {
					return false;
				}

				if (prev->key == key) {
					(*bks)[idx] = prev->next;
					prev->next == nullptr;
					delete prev;
					return true;
				}

				while(prev->next != nullptr) {
					if (prev->next->key == key) {
						auto vl = prev->next;
						prev->next = prev->next->next;
						vl->next = nullptr;
						delete vl;
						return true;
					}
					prev = prev->next;
				}

				return false;
			}
		}

		std::optional<V> get(const K& key) {
			auto bks = buckets;
			auto idx = get_bucket_idx(key, bks->size());

			auto e = (*bks)[idx];

			while(e != nullptr) {
				if (key == e->key) {
					return e->value;
				}
				e = e->next;
			}

			return std::nullopt;
		}
	};
}

#endif //COROUTINELIB_CC_DICTIONARY_H
