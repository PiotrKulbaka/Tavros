#include <gtest/gtest.h>

#include <tavros/core/math.hpp>

TEST(VectorTest, LengthTest)
{
    tavros::math::vec3 v(3.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(tavros::math::length(v), 5.0f);
}

TEST(VectorTest, NormalizeTest)
{
    tavros::math::vec3 v(3.0f, 4.0f, 0.0f);
    auto normalized = normalize(v);
    EXPECT_FLOAT_EQ(tavros::math::length(normalized), 1.0f);
}

TEST(QuaternionTest, ConjugateTest)
{
    tavros::math::quat q(1.0f, 2.0f, 3.0f, 4.0f);
    tavros::math::quat conj = tavros::math::conjugate(q);
    EXPECT_FLOAT_EQ(conj.x, -1.0f);
    EXPECT_FLOAT_EQ(conj.y, -2.0f);
    EXPECT_FLOAT_EQ(conj.z, -3.0f);
    EXPECT_FLOAT_EQ(conj.w, 4.0f);
}
