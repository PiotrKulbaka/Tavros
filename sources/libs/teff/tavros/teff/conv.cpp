#include <tavros/teff/conv.hpp>

#include <tavros/teff/node.hpp>

namespace
{
    bool valid_n(const tavros::teff::node* n) noexcept
    {
        return n && n->is_number();
    }

    bool valid_cont(const tavros::teff::node* n) noexcept
    {
        return n && n->is_number() && !n->has_key();
    }

    const tavros::teff::node* next_cont(const tavros::teff::node* n) noexcept
    {
        if (!n) {
            return nullptr;
        }
        auto* nx = n->next();
        return (nx && !nx->has_key()) ? nx : nullptr;
    }
} // namespace

namespace tavros::teff
{

    std::optional<math::euler3> conv<math::euler3>::operator()(const node* n) const noexcept
    {
        const auto* roll = n;
        const auto* pitch = roll ? roll->next() : nullptr;
        const auto* yaw = pitch ? pitch->next() : nullptr;

        if (valid_n(roll) && valid_cont(pitch) && valid_cont(yaw)) {
            return math::euler3(
                roll->value_or(0.0f),
                pitch->value_or(0.0f),
                yaw->value_or(0.0f)
            );
        }
        return std::nullopt;
    }

    std::optional<math::vec2> conv<math::vec2>::operator()(const node* n) const noexcept
    {
        const auto* x = n;
        const auto* y = x ? x->next() : nullptr;

        if (valid_n(x) && valid_cont(y)) {
            return math::vec2(x->value_or(0.0f), y->value_or(0.0f));
        }
        return std::nullopt;
    }

    std::optional<math::vec3> conv<math::vec3>::operator()(const node* n) const noexcept
    {
        const auto* x = n;
        const auto* y = x ? x->next() : nullptr;
        const auto* z = y ? y->next() : nullptr;

        if (valid_n(x) && valid_cont(y) && valid_cont(z)) {
            return math::vec3(
                x->value_or(0.0f),
                y->value_or(0.0f),
                z->value_or(0.0f)
            );
        }
        return std::nullopt;
    }

    std::optional<math::vec4> conv<math::vec4>::operator()(const node* n) const noexcept
    {
        const auto* x = n;
        const auto* y = x ? x->next() : nullptr;
        const auto* z = y ? y->next() : nullptr;
        const auto* w = z ? z->next() : nullptr;

        if (valid_n(x) && valid_cont(y) && valid_cont(z) && valid_cont(w)) {
            return math::vec4(
                x->value_or(0.0f),
                y->value_or(0.0f),
                z->value_or(0.0f),
                w->value_or(0.0f)
            );
        }
        return std::nullopt;
    }

    std::optional<math::quat> conv<math::quat>::operator()(const node* n) const noexcept
    {
        const auto* x = n;
        const auto* y = x ? x->next() : nullptr;
        const auto* z = y ? y->next() : nullptr;
        const auto* w = z ? z->next() : nullptr;

        if (valid_n(x) && valid_cont(y) && valid_cont(z) && valid_cont(w)) {
            return math::quat(
                x->value_or(0.0f),
                y->value_or(0.0f),
                z->value_or(0.0f),
                w->value_or(0.0f)
            );
        }
        return std::nullopt;
    }

    std::optional<math::ivec2> conv<math::ivec2>::operator()(const node* n) const noexcept
    {
        const auto* x = n;
        const auto* y = x ? x->next() : nullptr;

        if (valid_n(x) && valid_cont(y)) {
            return math::ivec2(
                x->value_or<int32>(0),
                y->value_or<int32>(0)
            );
        }
        return std::nullopt;
    }

    std::optional<math::rgba8> conv<math::rgba8>::operator()(const node* n) const noexcept
    {
        if (valid_n(n)) {
            auto v = n->value_or<uint64>(0);
            if (v <= 0xffffffffull) {
                return math::rgba8(static_cast<uint32>(v));
            }
        }
        return std::nullopt;
    }

    // mat3 - 9 floats, row-major: 3 rows x 3 cols
    std::optional<math::mat3> conv<math::mat3>::operator()(const node* n) const noexcept
    {
        const node* c[9];
        c[0] = n;
        for (int i = 1; i < 9; ++i) {
            c[i] = c[i - 1] ? c[i - 1]->next() : nullptr;
        }

        if (!valid_n(c[0])) {
            return std::nullopt;
        }
        for (int i = 1; i < 9; ++i) {
            if (!valid_cont(c[i])) {
                return std::nullopt;
            }
        }

        return math::mat3(
            c[0]->value_or(0.0f), c[1]->value_or(0.0f), c[2]->value_or(0.0f),
            c[3]->value_or(0.0f), c[4]->value_or(0.0f), c[5]->value_or(0.0f),
            c[6]->value_or(0.0f), c[7]->value_or(0.0f), c[8]->value_or(0.0f)
        );
    }

    // mat4 - 16 floats, row-major: 4 rows x 4 cols
    std::optional<math::mat4> conv<math::mat4>::operator()(const node* n) const noexcept
    {
        const node* c[16];
        c[0] = n;
        for (int i = 1; i < 16; ++i) {
            c[i] = c[i - 1] ? c[i - 1]->next() : nullptr;
        }

        if (!valid_n(c[0])) {
            return std::nullopt;
        }
        for (int i = 1; i < 16; ++i) {
            if (!valid_cont(c[i])) {
                return std::nullopt;
            }
        }

        return math::mat4(
            c[0]->value_or(0.0f), c[1]->value_or(0.0f), c[2]->value_or(0.0f), c[3]->value_or(0.0f),
            c[4]->value_or(0.0f), c[5]->value_or(0.0f), c[6]->value_or(0.0f), c[7]->value_or(0.0f),
            c[8]->value_or(0.0f), c[9]->value_or(0.0f), c[10]->value_or(0.0f), c[11]->value_or(0.0f),
            c[12]->value_or(0.0f), c[13]->value_or(0.0f), c[14]->value_or(0.0f), c[15]->value_or(0.0f)
        );
    }

} // namespace tavros::teff
