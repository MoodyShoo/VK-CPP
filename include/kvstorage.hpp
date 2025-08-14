#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

template <typename Clock>
class KVStorage {
public:
    using TimePoint = typename Clock::time_point;

    // Инициализирует хранилище переданными множеством записей. Размер span может быть очень большим.
    // Также принимает абстракцию часов (Clock) для возможности управления временем в тестах.
    explicit KVStorage(
        std::span<std::tuple<std::string /* key */, std::string /* value */, uint32_t /* ttl */>> entries, Clock& clock = Clock())
            : clock_(clock) {
        // O(n log n) - где n - кол-во записей в span entries
        for (const auto& [key, value, ttl] : entries) {
            set(key, value, ttl);
        }
    }

    ~KVStorage() = default;

    // Присваивает по ключу key значение value.
    // Если ttl == 0, то время жизни - бесконечность, иначе запись должна перестать быть доступной через ttl секунд.
    // Вставка в map происходит за O(log n), плюс O(1) амортизированное для хеш-таблицы
    void set(std::string key, std::string value, uint32_t ttl) {
        TimePoint expire_time = ttl == 0 ? TimePoint::max() : clock_.now() + std::chrono::seconds(ttl);

        auto [map_it, inserted] = storage_.insert_or_assign(std::move(key), Entry{std::move(value), expire_time});
        key_to_storage_iter_[map_it->first] = map_it;
    }

    // Удаляет запись по ключу key.
    // Возвращает true если запись была удалена. Если ключа не было до удаления, то вернет false.
    // O(1)* благодаря хеш-таблице для поиска + O(log n) для удаления из map (но в std::map erase по итератору - O(1)* амортизированно)
    bool remove(const std::string_view key) {
        auto it = key_to_storage_iter_.find(key);
        if (it == key_to_storage_iter_.end()) {
            return false;
        }

        storage_.erase(it->second);
        key_to_storage_iter_.erase(it);
        return true;
    }

    // Получает значение по ключу key. Если данного ключа нет, то вернёт std::nullopt.
    // Получение происходит за O(1)* - за счёт доп хеш-таблицы
    std::optional<std::string> get(const std::string_view key) const {
        auto it = key_to_storage_iter_.find(key);

        if (it != key_to_storage_iter_.end()) {
            const auto& [key, entry] = *it->second;

            if (entry.expire_time > clock_.now()) {
                return entry.value;
            }
        }

        return std::nullopt;
    }

    // Возвращает следующие count записей начиная с key в порядке лексикографической сортировки ключей.
    // Пример: ("a", "val11"), ("b", "val12"), ("d, "val13"), ("e", "val14")
    // getManySorted("c", 2) -> ("d", "val13"), ("e", "val14")
    // O(log n + k) - log n на поиск первого элемента (lower_bound), k — кол-во возвращаемых записей(count)
    std::vector<std::pair<std::string, std::string>> getManySorted(const std::string_view key,
                                                                   const uint32_t count) const {
        auto now = clock_.now();
        std::vector<std::pair<std::string, std::string>> result;

        if (count < 1) {
            return result;
        }

        result.reserve(count);

        for (auto it = storage_.lower_bound(key); it != storage_.end() && result.size() < count; ++it) {
            if (it->second.expire_time > now) {
                result.push_back({it->first, it->second.value});
            }
        }

        return result;
    }

    // Удаляет протухшую запись из структуры и возвращает ее. Если удалять нечего, то вернёт std::nullopt.
    // Если на момент вызова метода протухло несколько записей, то можно удалить любую.
    // O(n) - где n - кол-во записей в storage_

    // Была идея сделать хеш таблицу для хранения ttl, но это доп память
    std::optional<std::pair<std::string, std::string>> removeOneExpiredEntry() {
        auto now = clock_.now();

        for (const auto& [key, entry] : storage_) {
            if (entry.expire_time <= now) {
                auto expired = std::make_pair(key, entry.value);
                storage_.erase(key);

                return expired;
            }
        }

        return std::nullopt;
    }

private:
    struct Entry {
        std::string value; // sizeof(value);
        TimePoint expire_time; // ~8 байт;
    };

    // Компаратор для сравнения string_view и string, чтобы не создавать временные строки в методах мапы
    struct TransparentLess {
        using is_transparent = void;

        bool operator()(const std::string& lhs, const std::string& rhs) const noexcept { 
            return lhs < rhs; 
        }

        bool operator()(const std::string& lhs, std::string_view rhs) const noexcept {
            return std::string_view(lhs) < rhs;
        }

        bool operator()(std::string_view lhs, const std::string& rhs) const noexcept {
            return lhs < std::string_view(rhs);
        }
    };

    using StorageIterator = std::map<std::string, Entry>::iterator;

    Clock& clock_;

    // Использую map, так как читающих запросов 95% и сортировать каждый раз при вызове getManySorted повышает время
    // отклика системы
    std::map<std::string /* key */, Entry /* value, ttl*/, TransparentLess> storage_;
    // Дополнительная хеш-таблица для доступа к элементам storage_ за O(1)*
    // Требуется примерно 24 байта на запись: 16 байт под string_view (указатель + длина)
    // и 8 байт под итератор std::map (указатель на узел)
    std::unordered_map<std::string_view, StorageIterator> key_to_storage_iter_;
};