#include <concepts>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>
#include "entry.hpp"

template<typename T>
concept Hashable = requires(T a) {
  { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};

template<typename T>
concept Keyable = Hashable<T> && std::equality_comparable<T>;

template<Keyable K, typename V>
class hashmap {
private:
    std::vector<std::optional<std::pair<K, V>>> buckets_;
    std::vector<bool> deleted_;

    float load_factor_limit_{0.5}; // a value that will trigger a reallocation. calculated as 'size_ / buckets_.size()'
    std::size_t size_{0}; // how many elements are actually in buckets
    std::size_t last_probe_count_{0}; // buckets examined by the most recent lookup

    constexpr static inline auto k = 2;

    std::size_t next_size(std::size_t base) const {
        while (std::gcd(base, k) != 1) {
            ++base;
        }
        return base;
    }


    void rebalance() {
        const std::size_t new_size = next_size(buckets_.size() * 2);
        auto old = std::move(buckets_);

        buckets_.assign(new_size, std::nullopt);
        deleted_.assign(new_size, false);
        size_ = 0;

        for (auto& slot : old) {
            if (slot) {
                insert(slot->first, slot->second);
            }
        }
    }
    bool need_rebalance() const {
        return static_cast<float>(size_) / buckets_.size() >= load_factor_limit_;
    }

    void insert_at(const std::size_t slot_idx, const K& key, const V& val) {
        buckets_[slot_idx].emplace(key, val);
        deleted_[slot_idx] = false;
    }
    void delete_at(const std::size_t slot_idx) {
        buckets_[slot_idx].reset();
        deleted_[slot_idx] = true;
    }

    std::optional<std::size_t> find_free_slot(const K& key) {
        const std::size_t hash_v = std::hash<K>{}(key);

        std::size_t i = 0;
        std::optional<std::size_t> first_deleted;

        while (i < buckets_.size()) {
            const std::size_t idx = (hash_v + i * k) % buckets_.size();

            if (!buckets_[idx].has_value()) {
                if (deleted_[idx]) {
                    if (!first_deleted) first_deleted = idx;
                } else {
                    return first_deleted.value_or(idx);
                }
            } else {
                if (buckets_[idx].value().first == key) {
                    return std::nullopt; // already exists, not unique -> return
                }
            }

            if (deleted_[idx] && !first_deleted) first_deleted = idx;

            ++i;
        }

        return first_deleted;
    }

    std::optional<std::size_t> find_entry(const K& key) {
        const std::size_t hash_v = std::hash<K>{}(key);

        std::size_t i = 0;
        last_probe_count_ = 0;
        while (i < buckets_.size()) {
            const std::size_t idx = (hash_v + i * k) % buckets_.size();
            ++last_probe_count_;

            if (!buckets_[idx].has_value() && !deleted_[idx]) return std::nullopt;
            if (buckets_[idx].has_value() && buckets_[idx].value().first == key) {
                return idx;
            }

            ++i;
        }

        return std::nullopt;
    }
public:
    explicit hashmap(const std::size_t init_size) {
        if (init_size == 0) throw std::invalid_argument("hashmap size must be positive");
        buckets_.resize(next_size(init_size));
        deleted_.resize(buckets_.size(), false);
    }
    ~hashmap() = default;


    bool insert(const K& key, const V& val) {
        const auto slot_idx = find_free_slot(key);
        if (!slot_idx) {
            return false;
        }

        insert_at(slot_idx.value(), key, val);
        ++size_;

        if (need_rebalance()) {
            rebalance();
        }

        return true;
    }

    bool remove(const K& key) {
        const auto pos_opt = find_entry(key);
        if (!pos_opt.has_value()) {
            return false;
        }

        delete_at(pos_opt.value());
        --size_;

        return true;
    }

    V* get(const K& key) {
        const auto pos_opt = find_entry(key);
        if (!pos_opt.has_value()) {
            return nullptr;
        }

        return &buckets_[pos_opt.value()].value().second;
    }

    std::size_t size() const {
        return size_;
    }

    std::size_t last_probe_count() const {
        return last_probe_count_;
    }

    float load_factor() const {
        return static_cast<float>(size_) / buckets_.size();
    }

    void print(std::ostream& os = std::cout) const {
        os << "size=" << size_ << " buckets=" << buckets_.size()
           << " load=" << load_factor() << '\n';

        for (std::size_t i = 0; i < buckets_.size(); ++i) {
            os << '[' << i << "] ";
            if (buckets_[i].has_value()) {
                os << buckets_[i]->first << " -> " << buckets_[i]->second;
            } else {
                os << "<empty>";
            }
            os << '\n';
        }
    }
};
// (ФИО, номер заявки) - ключ
