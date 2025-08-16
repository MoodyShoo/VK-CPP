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

class KVStoragePerfTest : public testing::Test {
protected:
    void SetUp() override {
        storage = make_unique<Storage>(span<tuple<string, string, uint32_t>>{}, clock);
    }

    MockClock clock;
    using Storage = KVStorage<MockClock>;
    unique_ptr<Storage> storage;
};

TEST_F(KVStoragePerfTest, InsertMillionEntries) {
    const int N = 1'000'000;
    auto start = steady_clock::now();

    for (int i = 0; i < N; ++i) {
        storage->set("key" + ::to_string(i), "val" + ::to_string(i), 3600);
    }

    auto end = steady_clock::now();

    auto diff = end - start;
    cout << "[InsertMillionEntries] Duration: " << duration_cast<milliseconds>(diff) << '\n';
}

TEST_F(KVStoragePerfTest, ReadRandomEntries) {
    const int N = 1'000'000;
    for (int i = 0; i < N; ++i) {
        storage->set("key" + ::to_string(i), "val" + ::to_string(i), 3600);
    }

    auto start = steady_clock::now();

    for (int i = 0; i < 10'000; ++i) {
        int idx = rand() % N;
        auto val = storage->get("key" + ::to_string(idx));
        ASSERT_TRUE(val.has_value());
    }

    auto end = steady_clock::now();
    auto diff = end - start;
    cout << "[ReadRandomEntries] Duration: " << duration_cast<milliseconds>(diff) << '\n';
}