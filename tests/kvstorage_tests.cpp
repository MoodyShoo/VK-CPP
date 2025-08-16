#include <vector>

#include "gtest/gtest.h"
#include "kvstorage.hpp"

using namespace std;
using namespace chrono;

class MockClock {
public:
    using time_point = steady_clock::time_point;

    time_point now() const { return current_time_; }

    void advance(seconds sec) { current_time_ += sec; }

private:
    time_point current_time_ = steady_clock::now();
};

class KVStorageTest : public testing::Test {
protected:
    void SetUp() override { storage = make_unique<Storage>(span<tuple<string, string, uint32_t>>{}, clock); }

    MockClock clock;
    using Storage = KVStorage<MockClock>;
    unique_ptr<Storage> storage;
};

TEST_F(KVStorageTest, Construct) {
    MockClock clock;

    vector<tuple<string, string, uint32_t>> entries;

    for (char c = 'a'; c <= 'z'; ++c) {
        string key1(1, c);
        string key2 = key1 + key1;

        entries.emplace_back(key1, "val_" + key1, 0);
        entries.emplace_back(key2, "val_" + key2, 0);
    }

    span span_entries(entries);

    KVStorage<MockClock> storage(span_entries, clock);

    auto result = storage.getManySorted("", 1000);

    vector<pair<string, string>> expected;
    for (char c = 'a'; c <= 'z'; ++c) {
        string key1(1, c);
        string key2 = key1 + key1;
        expected.emplace_back(key1, "val_" + key1);
        expected.emplace_back(key2, "val_" + key2);
    }

    EXPECT_EQ(result, expected);
}

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
    clock.advance(2s);

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

TEST_F(KVStorageTest, GetManySortedWithExpiredSkip) {
    storage->set("a", "val_a", 0);

    for (char c = 'b'; c <= 'f'; ++c) {
        storage->set(string(1, c), "val_" + string(1, c), 5);
    }

    for (char c = 'g'; c <= 'j'; ++c) {
        storage->set(string(1, c), "val_" + string(1, c), 0);
    }

    clock.advance(10s);

    auto result = storage->getManySorted("a", 10);

    vector<pair<string, string>> expected = {
        {"a", "val_a"}, 
        {"g", "val_g"}, 
        {"h", "val_h"}, 
        {"i", "val_i"}, 
        {"j", "val_j"},
    };

    EXPECT_EQ(result, expected);
}

TEST_F(KVStorageTest, RemoveOneExpiredEntry) {
    storage->set("key1", "value1", 1);
    storage->set("key2", "value2", 10);

    clock.advance(seconds(2));

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
        storage->set("key" + to_string(i), "val" + to_string(i), 3600);
    }

    for (int i = 0; i < N; i += 1000) {
        auto val = storage->get("key" + to_string(i));
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(val.value(), "val" + to_string(i));
    }
}

TEST_F(KVStorageTest, BulkInsertInfiniteTTL) {
    const int N = 50'000;
    for (int i = 0; i < N; ++i) {
        storage->set("key_inf_" + to_string(i), "val" + to_string(i), 0);
    }

    clock.advance(100h);

    for (int i = 0; i < N; i += 500) {
        auto val = storage->get("key_inf_" + to_string(i));
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(val.value(), "val" + to_string(i));
    }
}

TEST_F(KVStorageTest, ExpireAllEntries) {
    const int N = 10'000;
    for (int i = 0; i < N; ++i) {
        storage->set("key_exp_" + to_string(i), "value", 1);
    }

    clock.advance(2s);

    for (int i = 0; i < N; ++i) {
        auto val = storage->get("key_exp_" + to_string(i));
        EXPECT_FALSE(val.has_value());
    }
}

TEST_F(KVStorageTest, RemoveManyExpiredEntries) {
    const int N = 5000;

    for (int i = 0; i < N; ++i) {
        storage->set("k" + to_string(i), "v" + to_string(i), 1);
    }

    clock.advance(2s);

    int removed_count = 0;
    while (auto entry = storage->removeOneExpiredEntry()) {
        ++removed_count;
    }

    EXPECT_EQ(removed_count, N);
}

TEST_F(KVStorageTest, SortedRangeQuery) {
    for (char c = 'a'; c <= 'z'; ++c) {
        string key(1, c);
        storage->set(key, "val_" + key, 100);
    }

    auto result = storage->getManySorted("m", 5);
    ASSERT_EQ(result.size(), 5);

    EXPECT_EQ(result[0].first, "m");
    EXPECT_EQ(result[4].first, "q");
}
