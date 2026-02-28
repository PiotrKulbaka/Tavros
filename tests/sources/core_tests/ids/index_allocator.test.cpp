#include <common.test.hpp>
#include <gtest/gtest.h>

#include <tavros/core/ids/l4_bitmap_index_allocator.hpp>
#include <tavros/core/ids/l3_bitmap_index_allocator.hpp>
#include <tavros/core/ids/l2_bitmap_index_allocator.hpp>

#include <random>
#include <vector>

using namespace tavros::core;

using index_type = index_t;

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
    EXPECT_EQ(alc->capacity(), 64ull * 64ull);
}

TEST_F(l3_bitmap_index_allocator_test, capacity_works)
{
    EXPECT_EQ(alc->capacity(), 64ull * 64ull * 64ull);
}

TEST_F(l4_bitmap_index_allocator_test, capacity_works)
{
    EXPECT_EQ(alc->capacity(), 64ull * 64ull * 64ull * 64ull);
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
        bool deallocated_first = this->alc->deallocate(idx);
        ASSERT_TRUE(deallocated_first) << "deallocate should return true on first deallocation";
        ASSERT_FALSE(this->alc->contains(idx)) << "Index should be marked as free after deallocate";

        bool deallocated_second = this->alc->deallocate(idx);
        ASSERT_FALSE(deallocated_second) << "deallocate should return false if index is already free";
    }

    ASSERT_EQ(this->alc->available(), this->alc->capacity());
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
        ASSERT_TRUE(this->alc->contains(idx));
        this->alc->deallocate(idx);
        ASSERT_FALSE(this->alc->contains(idx));
    }

    ASSERT_FALSE(this->alc->contains(capacity + 1));
    ASSERT_FALSE(this->alc->contains(this->alc->capacity()));
    ASSERT_FALSE(this->alc->contains(invalid_index));
    ASSERT_FALSE(this->alc->contains(0));
}

TYPED_TEST(index_allocator_test, reset_works)
{

    constexpr size_t capacity = 345;
    auto initial_remaining = this->alc->available();
    for (uint32 i = 0; i < capacity; ++i) {
        index_type idx = this->alc->allocate();
        ASSERT_NE(idx, invalid_index);
    }

    EXPECT_EQ(this->alc->available(), initial_remaining - capacity);

    this->alc->reset();
    EXPECT_EQ(this->alc->available(), this->alc->capacity());
    EXPECT_EQ(this->alc->available(), initial_remaining);
}

TYPED_TEST(index_allocator_test, iterator_empty)
{
    EXPECT_EQ(this->alc->begin(), this->alc->end());
}

TYPED_TEST(index_allocator_test, iterator_after_deallocate)
{
    for (int i = 0; i < 10; ++i) {
        this->alc->allocate();
    }

    this->alc->deallocate(0);
    this->alc->deallocate(5);
    this->alc->deallocate(9);

    std::set<index_type> actual;
    for (auto idx : *this->alc) {
        actual.insert(idx);
    }

    std::set<index_type> expected = { 1, 2, 3, 4, 6, 7, 8 };
    EXPECT_EQ(actual, expected);
}

TYPED_TEST(index_allocator_test, iterator_across_block_boundaries)
{
    for (int i = 0; i < 200; ++i) {
        this->alc->allocate();
    }

    for (int i = 0; i < 200; ++i) {
        if (i != 63 && i != 64 && i != 128) {
            this->alc->deallocate(i);
        }
    }

    std::vector<index_type> result;
    for (auto idx : *this->alc) {
        result.push_back(idx);
    }

    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], 63);
    EXPECT_EQ(result[1], 64);
    EXPECT_EQ(result[2], 128);
}

TYPED_TEST(index_allocator_test, size_consistency)
{
    EXPECT_EQ(this->alc->size(), 0u);
    EXPECT_EQ(this->alc->available(), this->alc->capacity());
    EXPECT_TRUE(this->alc->empty());
    EXPECT_FALSE(this->alc->full());

    for (size_t i = 0; i < 10; ++i) {
        this->alc->allocate();
        EXPECT_EQ(this->alc->size(), i + 1);
        EXPECT_EQ(this->alc->available(), this->alc->capacity() - i - 1);
        EXPECT_FALSE(this->alc->empty());
    }

    for (size_t i = 0; i < 10; ++i) {
        this->alc->deallocate(static_cast<index_type>(i));
        EXPECT_EQ(this->alc->size(), 9 - i);
    }

    EXPECT_TRUE(this->alc->empty());
}

TYPED_TEST(index_allocator_test, iterator_matches_contains)
{
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1);

    for (int i = 0; i < 200; ++i) {
        this->alc->allocate();
    }
    for (int i = 0; i < 200; ++i) {
        if (dist(rng)) {
            this->alc->deallocate(i);
        }
    }

    std::set<index_type> from_iterator;
    for (auto idx : *this->alc) {
        EXPECT_TRUE(this->alc->contains(idx)) << "idx=" << idx;
        from_iterator.insert(idx);
    }

    EXPECT_EQ(from_iterator.size(), this->alc->size());
}

TYPED_TEST(index_allocator_test, boundary_indices)
{
    constexpr index_type boundaries[] = { 63, 64, 65, 127, 128, 129 };

    for (int i = 0; i < 130; ++i) {
        this->alc->allocate();
    }

    for (auto idx : boundaries) {
        ASSERT_TRUE(this->alc->contains(idx)) << "idx=" << idx;
        ASSERT_TRUE(this->alc->deallocate(idx)) << "idx=" << idx;
        ASSERT_FALSE(this->alc->contains(idx)) << "idx=" << idx;

        auto reallocated = this->alc->allocate();
        EXPECT_EQ(reallocated, idx) << "Expected smallest free index=" << idx;
    }
}

TYPED_TEST(index_allocator_test, stress_reset_works)
{
    if (g_stress_skip) {
        GTEST_SKIP();
    }

    auto max_idx = this->alc->capacity();
    for (uint32 i = 0; i < max_idx; ++i) {
        (void)this->alc->allocate();
    }

    // Last idx should be invalid_index
    auto idx = this->alc->allocate();
    EXPECT_EQ(idx, invalid_index);
    EXPECT_EQ(this->alc->available(), 0);
    this->alc->reset();

    EXPECT_EQ(this->alc->available(), max_idx);

    for (uint32 i = 0; i < max_idx; ++i) {
        auto idx = this->alc->allocate();
        ASSERT_NE(idx, invalid_index);
        ASSERT_EQ(idx, i);
        ASSERT_EQ(this->alc->available(), this->alc->capacity() - i - 1);
    }
}

TYPED_TEST(index_allocator_test, stress_allocate_all_indices_after_dealocate_works)
{
    if (g_stress_skip) {
        GTEST_SKIP();
    }

    auto max_idx = this->alc->capacity();
    for (uint32 i = 0; i < max_idx; ++i) {
        auto idx = this->alc->allocate();
        ASSERT_NE(idx, invalid_index);
        ASSERT_EQ(idx, i);
        ASSERT_EQ(this->alc->available(), this->alc->capacity() - i - 1);
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
        ASSERT_EQ(this->alc->available(), this->alc->capacity() - i - 1);
    }
}

TYPED_TEST(index_allocator_test, stress_random_allocate_deallocate)
{
    if (g_stress_skip) {
        GTEST_SKIP();
    }

    auto max_idx = this->alc->capacity();
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
                ASSERT_TRUE(this->alc->contains(idx));
                auto first_deallocate = this->alc->deallocate(idx);
                ASSERT_TRUE(first_deallocate);
                
                auto second_deallocate = this->alc->deallocate(idx);
                ASSERT_FALSE(second_deallocate);

                ASSERT_FALSE(this->alc->contains(idx));
                EXPECT_FALSE(assert_was_called());
                allocated[idx] = false;
            }
        }
    }
}
