#include <common.test.hpp>

#include <tavros/core/ids/l4_index_allocator.hpp>

#include <random>
#include <vector>

using namespace tavros::core;



class l4_index_allocator_param_test
    : public ::testing::TestWithParam<size_t>
{
protected:
    void SetUp() override
    {
        g_tav_assert_was_called = false;
        index_count = GetParam();
        alc = new l4_index_allocator();
    }

    void TearDown() override
    {
        delete alc;
    }

    size_t index_count;
    l4_index_allocator* alc;
};

class l4_index_allocator_test
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_tav_assert_was_called = false;
        alc = new l4_index_allocator();
    }

    void TearDown() override
    {
        delete alc;
    }

    l4_index_allocator* alc;
};

TEST_F(l4_index_allocator_test, capacity_works)
{
    EXPECT_EQ(alc->capacity(), 64ull * 64ull * 64ull * 64ull);
}

TEST_F(l4_index_allocator_test, allocate_all_indices_works)
{
    auto max_idx = alc->capacity();
    for (uint32 i = 0; i < max_idx; ++i) {
        auto idx = alc->allocate();
        EXPECT_NE(idx, l4_index_allocator::invalid_index);
        EXPECT_EQ(idx, i);
        EXPECT_EQ(alc->remaining(), alc->capacity() - i - 1);
    }

    EXPECT_EQ(alc->remaining(), 0);
    // Last idx should be invalid_index
    auto idx = alc->allocate();
    EXPECT_EQ(idx, l4_index_allocator::invalid_index);
    EXPECT_EQ(alc->remaining(), 0);
}

TEST_F(l4_index_allocator_test, reset_works)
{
    auto max_idx = alc->capacity();
    for (uint32 i = 0; i < max_idx; ++i) {
        (void)alc->allocate();
    }

    // Last idx should be invalid_index
    auto idx = alc->allocate();
    EXPECT_EQ(idx, l4_index_allocator::invalid_index);
    EXPECT_EQ(alc->remaining(), 0);
    alc->reset();

    EXPECT_EQ(alc->remaining(), max_idx);

    for (uint32 i = 0; i < max_idx; ++i) {
        auto idx = alc->allocate();
        ASSERT_NE(idx, l4_index_allocator::invalid_index);
        ASSERT_EQ(idx, i);
        ASSERT_EQ(alc->remaining(), alc->capacity() - i - 1);
    }
}

TEST_F(l4_index_allocator_test, deallocate_all_indices_works)
{
    auto max_idx = alc->capacity();
    for (uint32 i = 0; i < max_idx; ++i) {
        (void)alc->allocate();
    }

    ASSERT_FALSE(assert_was_called());

    for (uint32 i = 0; i < max_idx; ++i) {
        alc->deallocate(i);
        ASSERT_FALSE(assert_was_called());
    }

}

TEST_F(l4_index_allocator_test, allocate_all_indices_after_dealocate_works)
{
    auto max_idx = alc->capacity();
    for (uint32 i = 0; i < max_idx; ++i) {
        (void)alc->allocate();
    }

    for (uint32 i = 0; i < max_idx; ++i) {
        alc->deallocate(i);
    }

    for (uint32 i = 0; i < max_idx; ++i) {
        auto idx = alc->allocate();
        EXPECT_NE(idx, l4_index_allocator::invalid_index);
        EXPECT_EQ(idx, i);
        EXPECT_EQ(alc->remaining(), alc->capacity() - i - 1);
    }

}


TEST_F(l4_index_allocator_test, stress_random_allocate_deallocate)
{
    auto max_idx = alc->capacity();
    std::vector<bool> allocated(max_idx, false);

    std::mt19937 rng(12345); // фиксированный seed для воспроизводимости
    std::uniform_int_distribution<int> action_dist(0, 1); // 0 = allocate, 1 = deallocate
    std::uniform_int_distribution<uint32> idx_dist(0, max_idx - 1);

    const size_t iterations = max_idx * 10; // многократно больше, чем размер

    // Fill the 50% of indeces to alloced state
    for (uint32 i = 0; i < max_idx / 2; ++i) {
        auto idx = alc->allocate();
        allocated[idx] = true;
    }

    for (size_t it = 0; it < iterations; ++it) {
        if (action_dist(rng) == 0) {
            // allocate
            auto idx = alc->allocate();
            if (idx != l4_index_allocator::invalid_index) {
                EXPECT_LT(idx, max_idx);
                EXPECT_FALSE(allocated[idx]) << "Index was already allocated!";
                allocated[idx] = true;
            }
        }
        else {
            // deallocate random index
            auto idx = idx_dist(rng);
            if (allocated[idx]) {
                alc->deallocate(idx);
                EXPECT_FALSE(assert_was_called());
                allocated[idx] = false;
            }
        }
    }

    // В конце проверим, что можно снова аллоцировать все индексы
    alc->reset();
    for (uint32 i = 0; i < max_idx; ++i) {
        auto idx = alc->allocate();
        ASSERT_NE(idx, l4_index_allocator::invalid_index);
        ASSERT_LT(idx, max_idx);
    }
    EXPECT_EQ(alc->allocate(), l4_index_allocator::invalid_index);
}

//INSTANTIATE_TEST_SUITE_P(
//    sized_indices,
//    l4_index_allocator_param_test,
//    ::testing::Values(64ull*64ull*64ull*64ull)
//);
