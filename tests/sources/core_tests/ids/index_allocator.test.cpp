#include <common.test.hpp>
#include <gtest/gtest.h>

#include <tavros/core/ids/l4_bitmap_index_allocator.hpp>
#include <tavros/core/ids/l3_bitmap_index_allocator.hpp>
#include <tavros/core/ids/l2_bitmap_index_allocator.hpp>

#include <random>
#include <vector>

using namespace tavros::core;


template <typename Allocator>
class index_allocator_test
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_tav_assert_was_called = false;
        alc = new Allocator();
    }

    void TearDown() override
    {
        delete alc;
    }

    Allocator* alc = nullptr;
};

using l2_bitmap_index_allocator_test = index_allocator_test<l2_bitmap_index_allocator>;
using l3_bitmap_index_allocator_test = index_allocator_test<l3_bitmap_index_allocator>;
using l4_bitmap_index_allocator_test = index_allocator_test<l4_bitmap_index_allocator>;


TEST_F(l2_bitmap_index_allocator_test, capacity_works)
{
    EXPECT_EQ(alc->max_index(), 64ull * 64ull);
}

TEST_F(l3_bitmap_index_allocator_test, capacity_works)
{
    EXPECT_EQ(alc->max_index(), 64ull * 64ull * 64ull);
}

TEST_F(l4_bitmap_index_allocator_test, capacity_works)
{
    EXPECT_EQ(alc->max_index(), 64ull * 64ull * 64ull * 64ull);
}

using Implementations = ::testing::Types<l4_bitmap_index_allocator, l3_bitmap_index_allocator, l2_bitmap_index_allocator>;
TYPED_TEST_SUITE(index_allocator_test, Implementations);

TYPED_TEST(index_allocator_test, allocate_works)
{
    constexpr size_t capacity = 345;
    std::set<index_type> allocated_indices;

    for (size_t i = 0; i < capacity; ++i) {
        index_type idx = this->alc->allocate();

        ASSERT_NE(idx, invalid_index) << "Allocation returned invalid_index";

        ASSERT_TRUE(allocated_indices.find(idx) == allocated_indices.end()) << "Duplicate index allocated: " << idx;

        allocated_indices.insert(idx);
    }
}

TYPED_TEST(index_allocator_test, deallocate_works)
{
    constexpr size_t capacity = 345;
    std::set<index_type> allocated_indices;

    for (size_t i = 0; i < capacity; ++i) {
        index_type idx = this->alc->allocate();
        allocated_indices.insert(idx);
    }

    for (auto idx : allocated_indices) {
        this->alc->deallocate(idx);
        ASSERT_FALSE(this->alc->allocated(idx)) << "Index should be marked as free after deallocate";
    }

    ASSERT_EQ(this->alc->remaining(), this->alc->max_index());
}

TYPED_TEST(index_allocator_test, try_deallocate_works)
{
    constexpr size_t capacity = 345;
    std::set<index_type> allocated_indices;

    for (size_t i = 0; i < capacity; ++i) {
        index_type idx = this->alc->allocate();
        allocated_indices.insert(idx);
    }

    for (auto idx : allocated_indices) {
        bool deallocated_first = this->alc->try_deallocate(idx);
        ASSERT_TRUE(deallocated_first) << "try_deallocate should return true on first deallocation";
        ASSERT_FALSE(this->alc->allocated(idx)) << "Index should be marked as free after try_deallocate";

        bool deallocated_second = this->alc->try_deallocate(idx);
        ASSERT_FALSE(deallocated_second) << "try_deallocate should return false if index is already free";
    }

    ASSERT_EQ(this->alc->remaining(), this->alc->max_index());
}

TYPED_TEST(index_allocator_test, allocated_works)
{
    constexpr size_t capacity = 345;
    std::set<index_type> allocated_indices;

    for (size_t i = 0; i < capacity; ++i) {
        index_type idx = this->alc->allocate();
        allocated_indices.insert(idx);
    }

    for (auto idx : allocated_indices) {
        ASSERT_TRUE(this->alc->allocated(idx));
        this->alc->deallocate(idx);
        ASSERT_FALSE(this->alc->allocated(idx));
    }

    ASSERT_FALSE(this->alc->allocated(capacity + 1));
    ASSERT_FALSE(this->alc->allocated(this->alc->max_index()));
    ASSERT_FALSE(this->alc->allocated(invalid_index));
    ASSERT_FALSE(this->alc->allocated(0));
}

TYPED_TEST(index_allocator_test, reset_works)
{

    constexpr size_t capacity = 345;
    auto initial_remaining = this->alc->remaining();
    for (uint32 i = 0; i < capacity; ++i) {
        index_type idx = this->alc->allocate();
        ASSERT_NE(idx, invalid_index);
    }

    EXPECT_EQ(this->alc->remaining(), initial_remaining - capacity);

    this->alc->reset();
    EXPECT_EQ(this->alc->remaining(), this->alc->max_index());
    EXPECT_EQ(this->alc->remaining(), initial_remaining);
}

TYPED_TEST(index_allocator_test, stress_reset_works)
{
    if (g_stress_skip) {
        GTEST_SKIP();
    }

    auto max_idx = this->alc->max_index();
    for (uint32 i = 0; i < max_idx; ++i) {
        (void)this->alc->allocate();
    }

    // Last idx should be invalid_index
    auto idx = this->alc->allocate();
    EXPECT_EQ(idx, invalid_index);
    EXPECT_EQ(this->alc->remaining(), 0);
    this->alc->reset();

    EXPECT_EQ(this->alc->remaining(), max_idx);

    for (uint32 i = 0; i < max_idx; ++i) {
        auto idx = this->alc->allocate();
        ASSERT_NE(idx, invalid_index);
        ASSERT_EQ(idx, i);
        ASSERT_EQ(this->alc->remaining(), this->alc->max_index() - i - 1);
    }
}

TYPED_TEST(index_allocator_test, stress_allocate_all_indices_after_dealocate_works)
{
    if (g_stress_skip) {
        GTEST_SKIP();
    }

    auto max_idx = this->alc->max_index();
    for (uint32 i = 0; i < max_idx; ++i) {
        auto idx = this->alc->allocate();
        ASSERT_NE(idx, invalid_index);
        ASSERT_EQ(idx, i);
        ASSERT_EQ(this->alc->remaining(), this->alc->max_index() - i - 1);
    }

    ASSERT_FALSE(assert_was_called());

    for (uint32 i = 0; i < max_idx; ++i) {
        this->alc->deallocate(i);
        ASSERT_FALSE(assert_was_called());
    }

    for (uint32 i = 0; i < max_idx; ++i) {
        auto idx = this->alc->allocate();
        ASSERT_NE(idx, invalid_index);
        ASSERT_EQ(idx, i);
        ASSERT_EQ(this->alc->remaining(), this->alc->max_index() - i - 1);
    }
}

TYPED_TEST(index_allocator_test, stress_random_allocate_deallocate)
{
    if (g_stress_skip) {
        GTEST_SKIP();
    }

    auto max_idx = this->alc->max_index();
    std::vector<bool> allocated(max_idx, false);

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> action_dist(0, 1); // 0 = allocate, 1 = deallocate
    std::uniform_int_distribution<uint32> idx_dist(0, max_idx - 1);

    const size_t iterations = max_idx * 10;

    // Fill the 50% of indeces to alloced state
    for (uint32 i = 0; i < max_idx / 2; ++i) {
        auto idx = this->alc->allocate();
        allocated[idx] = true;
    }

    for (size_t it = 0; it < iterations; ++it) {
        if (action_dist(rng) == 0) {
            // allocate
            auto idx = this->alc->allocate();
            if (idx != invalid_index) {
                EXPECT_LT(idx, max_idx);
                EXPECT_FALSE(allocated[idx]) << "Index was already allocated!";
                allocated[idx] = true;
            }
        } else {
            // deallocate random index
            auto idx = idx_dist(rng);
            if (allocated[idx]) {
                ASSERT_TRUE(this->alc->allocated(idx));
                auto first_deallocate = this->alc->try_deallocate(idx);
                ASSERT_TRUE(first_deallocate);
                
                auto second_deallocate = this->alc->try_deallocate(idx);
                ASSERT_FALSE(second_deallocate);

                ASSERT_FALSE(this->alc->allocated(idx));
                EXPECT_FALSE(assert_was_called());
                allocated[idx] = false;
            }
        }
    }
}
