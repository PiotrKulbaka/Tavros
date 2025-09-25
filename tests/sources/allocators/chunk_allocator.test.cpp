#include <common.test.hpp>

#include <tavros/core/memory/chunk_allocator.hpp>

#include <map>
#include <set>
#include <cstdlib>

#include <random>

using namespace tavros::core;

class chunk_allocator_test : public unittest_scope
{
};

struct alignas(32) alc_t
{
    uint64 m[30];
};

template <size_t BlocksNumber>
class chunk_allocator_param_test
    : public ::testing::TestWithParam<size_t>
{
protected:
    void SetUp() override {
        g_tav_assert_was_called = false;
        chunks_number = GetParam();
        storage = reinterpret_cast<uint8_t*>(malloc(sizeof(alc_t) * (chunks_number + 1)));
        
        uint8_t* aligned_storage = reinterpret_cast<uint8*>((reinterpret_cast<size_t>(storage) + alignof(alc_t) - 1) & ~(alignof(alc_t) - 1));
        alloc = new chunk_allocator<alc_t, BlocksNumber>(aligned_storage, chunks_number);
    }

    void TearDown() override {
        delete alloc;
        free(storage);
    }

    size_t chunks_number;
    uint8_t* storage;
    chunk_allocator<alc_t, BlocksNumber>* alloc;
};

using chunk_allocator_param_test_512 = chunk_allocator_param_test<512>;

using chunk_allocator_param_test_4096 = chunk_allocator_param_test<4096>;


TEST_P(chunk_allocator_param_test_512, allocate_all_blocks_works)
{
    std::set<alc_t*> pointers;
    for (auto i = 0; i < chunks_number; ++i) {
        alc_t* p = alloc->allocate_block();
        ASSERT_NE(p, nullptr);

        auto it = pointers.find(p);
        EXPECT_EQ(it, pointers.end());

        pointers.insert(p);
    }

    EXPECT_EQ(alloc->allocate_block(), nullptr);
}

TEST_P(chunk_allocator_param_test_512, allocate_alignement_works)
{
    for (auto i = 0; i < chunks_number; ++i) {
        alc_t* p = alloc->allocate_block();
        auto mem_i = reinterpret_cast<size_t>(p);
        EXPECT_TRUE(mem_i % alignof(alc_t) == 0);
    }
}

TEST_P(chunk_allocator_param_test_512, filled_works)
{
    for (auto i = 0; i < chunks_number; ++i) {
        EXPECT_FALSE(alloc->filled());
        alc_t* p = alloc->allocate_block();
        (void)p;
    }

    EXPECT_TRUE(alloc->filled());
}

TEST_P(chunk_allocator_param_test_512, deallocate_works)
{
    std::set<alc_t*> pointers;
    for (auto i = 0; i < chunks_number; ++i) {
        alc_t* p = alloc->allocate_block();
        pointers.insert(p);
    }

    EXPECT_TRUE(alloc->filled());

    for (auto* p : pointers) {
        alloc->deallocate_block(p);
        EXPECT_FALSE(alloc->filled());
    }

    EXPECT_FALSE(alloc->filled());
}


TEST_P(chunk_allocator_param_test_512, allocate_after_deallocate_works)
{
    std::set<alc_t*> pointers;
    for (auto i = 0; i < chunks_number; ++i) {
        alc_t* p = alloc->allocate_block();
        pointers.insert(p);
    }

    EXPECT_TRUE(alloc->filled());

    for (auto* p : pointers) {
        alloc->deallocate_block(p);
    }

    EXPECT_FALSE(alloc->filled());

    // All pointers should be allocated again
    for (auto i = 0; i < chunks_number; ++i) {
        alc_t* p = alloc->allocate_block();
        auto it = pointers.find(p);
        EXPECT_TRUE(it != pointers.end());
    }

    EXPECT_TRUE(alloc->filled());
}

TEST_P(chunk_allocator_param_test_512, random_allocate_deallocate_stress) {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> coin(0, 2);

    std::vector<alc_t*> live_blocks; //

    constexpr size_t operations = 20000; // Number stress operations
    for (size_t i = 0; i < operations; ++i) {
        if (live_blocks.empty() || coin(rng) != 0) { // about 66 percent to allocate a new block
            // Attempt to allocate
            alc_t* p = alloc->allocate_block();
            if (p) {
                live_blocks.push_back(p);
                // Check pool stage
                EXPECT_LE(live_blocks.size(), chunks_number);
                EXPECT_EQ(alloc->filled(), live_blocks.size() == chunks_number);
            }
            else {
                // Allocation failed -> pool shold be filled
                EXPECT_TRUE(alloc->filled());
            }
        }
        else {
            // Random deallocation
            std::uniform_int_distribution<size_t> idx_dist(0, live_blocks.size() - 1);
            size_t idx = idx_dist(rng);
            alloc->deallocate_block(live_blocks[idx]);
            std::swap(live_blocks[idx], live_blocks.back());
            live_blocks.pop_back();

            // Check filled()
            EXPECT_FALSE(alloc->filled());
        }
    }

    // Deallocate all blocks
    for (auto* p : live_blocks) {
        alloc->deallocate_block(p);
    }
    live_blocks.clear();

    // Check for not filled
    EXPECT_FALSE(alloc->filled());

    // And ckeck for allocate all blocks
    for (size_t i = 0; i < chunks_number; ++i) {
        alc_t* p = alloc->allocate_block();
        EXPECT_NE(p, nullptr);
    }
    EXPECT_TRUE(alloc->filled());
}


INSTANTIATE_TEST_SUITE_P(
    chunk_sizes,
    chunk_allocator_param_test_512,
    ::testing::Values(512, 1, 64, 11, 255, 256, 257, 7, 3, 507, 128)
);

TEST_P(chunk_allocator_param_test_4096, allocate_after_deallocate_works)
{
    std::set<alc_t*> pointers;
    for (auto i = 0; i < chunks_number; ++i) {
        alc_t* p = alloc->allocate_block();
        ASSERT_NE(p, nullptr);

        auto it = pointers.find(p);
        EXPECT_EQ(it, pointers.end());

        pointers.insert(p);

        EXPECT_EQ(chunks_number - i - 1, alloc->available_blocks());
    }

    EXPECT_TRUE(alloc->filled());

    // check deallocate wrong ptr
    EXPECT_FALSE(assert_was_called());

    alc_t* wrong_ptr = reinterpret_cast<alc_t*>(alignof(alc_t) * 11);
    alloc->deallocate_block(wrong_ptr);

    EXPECT_TRUE(assert_was_called());

    EXPECT_TRUE(alloc->filled());


    for (auto* p : pointers) {
        auto avail_before_dealloc = alloc->available_blocks();

        alloc->deallocate_block(p);
        auto avail_after_dealloc = alloc->available_blocks();

        EXPECT_FALSE(assert_was_called());
        alloc->deallocate_block(p);
        EXPECT_TRUE(assert_was_called());
        
        auto avail_now = alloc->available_blocks();

        EXPECT_EQ(avail_before_dealloc + 1, avail_after_dealloc);
        EXPECT_EQ(avail_after_dealloc, avail_now);
    }

    EXPECT_FALSE(alloc->filled());

    // All pointers should be allocated again
    for (auto i = 0; i < chunks_number; ++i) {
        alc_t* p = alloc->allocate_block();
        auto it = pointers.find(p);
        EXPECT_TRUE(it != pointers.end());
    }

    EXPECT_TRUE(alloc->filled());
}

INSTANTIATE_TEST_SUITE_P(
    chunk_sizes,
    chunk_allocator_param_test_4096,
    ::testing::Values(4096, 1, 4095, 1234, 2222)
);
