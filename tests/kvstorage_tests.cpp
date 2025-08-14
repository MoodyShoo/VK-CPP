#include <iostream>
#include <vector>

#include "gtest/gtest.h"
#include "kvstorage.hpp"

class MockClock {
public:
    using time_point = std::chrono::steady_clock::time_point;

    time_point now() const { return current_time_; }

    void advance(std::chrono::seconds sec) { current_time_ += sec; }

private:
    time_point current_time_ = std::chrono::steady_clock::now();
};

class KVStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        storage = std::make_unique<Storage>(std::span<std::tuple<std::string, std::string, uint32_t>>{}, clock);
    }

    MockClock clock;
    using Storage = KVStorage<MockClock>;
    std::unique_ptr<Storage> storage;
};

TEST_F(KVStorageTest, SetAndGet) {
    storage->set("key1", "value1", 10);
    auto val = storage->get("key1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "value1");
}

TEST_F(KVStorageTest, SetOverwrite) {
    storage->set("key1", "value1", 10);
    auto val = storage->get("key1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "value1");

    storage->set("key1", "value222", 0);
    val = storage->get("key1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "value222");
}

TEST_F(KVStorageTest, GetExpiredKey) {
    storage->set("key1", "value1", 1);
    clock.advance(std::chrono::seconds(2));

    auto val = storage->get("key1");
    EXPECT_FALSE(val.has_value());
}

TEST_F(KVStorageTest, RemoveKey) {
    storage->set("key1", "value1", 10);
    bool removed = storage->remove("key1");
    EXPECT_TRUE(removed);

    auto val = storage->get("key1");
    EXPECT_FALSE(val.has_value());
    EXPECT_FALSE(storage->remove("key1"));
}

TEST_F(KVStorageTest, GetManySorted) {
    storage->set("a", "val11", 10);
    storage->set("b", "val12", 10);
    storage->set("d", "val13", 10);
    storage->set("e", "val14", 10);

    auto result = storage->getManySorted("c", 2);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].first, "d");
    EXPECT_EQ(result[0].second, "val13");
    EXPECT_EQ(result[1].first, "e");
    EXPECT_EQ(result[1].second, "val14");
}

TEST_F(KVStorageTest, RemoveOneExpiredEntry) {
    storage->set("key1", "value1", 1);
    storage->set("key2", "value2", 10);

    clock.advance(std::chrono::seconds(2));

    auto expired = storage->removeOneExpiredEntry();
    ASSERT_TRUE(expired.has_value());
    EXPECT_EQ(expired->first, "key1");
    EXPECT_EQ(expired->second, "value1");

    auto val = storage->get("key1");
    EXPECT_FALSE(val.has_value());

    val = storage->get("key2");
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "value2");
}

TEST_F(KVStorageTest, BulkInsertAndRead) {
    const int N = 100'000;
    for (int i = 0; i < N; ++i) {
        storage->set("key" + std::to_string(i), "val" + std::to_string(i), 3600);
    }

    for (int i = 0; i < N; i += 1000) {
        auto val = storage->get("key" + std::to_string(i));
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(val.value(), "val" + std::to_string(i));
    }
}

TEST_F(KVStorageTest, BulkInsertInfiniteTTL) {
    const int N = 50'000;
    for (int i = 0; i < N; ++i) {
        storage->set("key_inf_" + std::to_string(i), "val" + std::to_string(i), 0);
    }

    clock.advance(std::chrono::hours(100));

    for (int i = 0; i < N; i += 500) {
        auto val = storage->get("key_inf_" + std::to_string(i));
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(val.value(), "val" + std::to_string(i));
    }
}

TEST_F(KVStorageTest, ExpireAllEntries) {
    const int N = 10'000;
    for (int i = 0; i < N; ++i) {
        storage->set("key_exp_" + std::to_string(i), "value", 1);
    }

    clock.advance(std::chrono::seconds(2));

    for (int i = 0; i < N; ++i) {
        auto val = storage->get("key_exp_" + std::to_string(i));
        EXPECT_FALSE(val.has_value());
    }
}

TEST_F(KVStorageTest, RemoveManyExpiredEntries) {
    const int N = 5000;

    for (int i = 0; i < N; ++i) {
        storage->set("k" + std::to_string(i), "v" + std::to_string(i), 1);
    }

    clock.advance(std::chrono::seconds(2));

    int removed_count = 0;
    while (auto entry = storage->removeOneExpiredEntry()) {
        ++removed_count;
    }

    EXPECT_EQ(removed_count, N);
}

TEST_F(KVStorageTest, SortedRangeQuery) {
    for (char c = 'a'; c <= 'z'; ++c) {
        std::string key(1, c);
        storage->set(key, "val_" + key, 100);
    }

    auto result = storage->getManySorted("m", 5);
    ASSERT_EQ(result.size(), 5);

    EXPECT_EQ(result[0].first, "m");
    EXPECT_EQ(result[4].first, "q");
}
