#include <tavros/tef/conv.hpp>

#include <tavros/tef/node.hpp>

namespace
{
    bool valid_n(const tavros::tef::node* n) noexcept
    {
        return n && n->is_number();
    }

    bool valid_cont(const tavros::tef::node* n) noexcept
    {
        return n && n->is_number() && !n->has_key();
    }

    const tavros::tef::node* next_cont(const tavros::tef::node* n) noexcept
    {
        if (!n) {
            return nullptr;
        }
        auto* nx = n->next();
        return (nx && !nx->has_key()) ? nx : nullptr;
    }

    template<class T>
    std::optional<T> read_integer(const tavros::tef::node* n) noexcept
    {
        static_assert(std::is_integral_v<T>);

        if (!n) {
            return std::nullopt;
        }

        using base_t = std::conditional_t<std::is_signed_v<T>, int64, uint64>;

        auto opt_v = n->value<base_t>();
        if (!opt_v) {
            return std::nullopt;
        }

        const auto v = *opt_v;

        if constexpr (std::is_signed_v<base_t> == std::is_signed_v<T>) {
            if (v < static_cast<base_t>(std::numeric_limits<T>::min()) || v > static_cast<base_t>(std::numeric_limits<T>::max())) {
                return std::nullopt;
            }
        } else if constexpr (std::is_signed_v<base_t>) {
            if (v < 0 || static_cast<std::make_unsigned_t<base_t>>(v) > std::numeric_limits<T>::max()) {
                return std::nullopt;
            }
        } else {
            if (v > static_cast<std::make_unsigned_t<T>>(std::numeric_limits<T>::max())) {
                return std::nullopt;
            }
        }

        return static_cast<T>(v);
    }

    template<typename T>
    void write_value(tavros::tef::node* parent, tavros::core::string_view key, const T& val)
    {
        TAV_ASSERT(parent);

        if (parent) {
            parent->append(key, val);
        }
    }
} // namespace

namespace tavros::tef
{

    // ============================================================
    // bool
    // ============================================================

    std::optional<bool> conv<bool>::read(const node* n) const noexcept
    {
        return n ? n->value<bool>() : std::nullopt;
    }

    void conv<bool>::write(node* parent, core::string_view key, const bool& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<bool>::description() const noexcept
    {
        return "bool";
    }


    // ============================================================
    // uint8
    // ============================================================

    std::optional<uint8> conv<uint8>::read(const node* n) const noexcept
    {
        return read_integer<uint8>(n);
    }

    void conv<uint8>::write(node* parent, core::string_view key, const uint8& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<uint8>::description() const noexcept
    {
        return "uint8 [0..255]";
    }


    // ============================================================
    // int8
    // ============================================================

    std::optional<int8> conv<int8>::read(const node* n) const noexcept
    {
        return read_integer<int8>(n);
    }

    void conv<int8>::write(node* parent, core::string_view key, const int8& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<int8>::description() const noexcept
    {
        return "int8 [-128..127]";
    }


    // ============================================================
    // uint16
    // ============================================================

    std::optional<uint16> conv<uint16>::read(const node* n) const noexcept
    {
        return read_integer<uint16>(n);
    }

    void conv<uint16>::write(node* parent, core::string_view key, const uint16& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<uint16>::description() const noexcept
    {
        return "uint16 [0..65535]";
    }


    // ============================================================
    // int16
    // ============================================================

    std::optional<int16> conv<int16>::read(const node* n) const noexcept
    {
        return read_integer<int16>(n);
    }

    void conv<int16>::write(node* parent, core::string_view key, const int16& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<int16>::description() const noexcept
    {
        return "int16 [-32768..32767]";
    }


    // ============================================================
    // uint32
    // ============================================================

    std::optional<uint32> conv<uint32>::read(const node* n) const noexcept
    {
        return read_integer<uint32>(n);
    }

    void conv<uint32>::write(node* parent, core::string_view key, const uint32& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<uint32>::description() const noexcept
    {
        return "uint32 [0..4294967295]";
    }


    // ============================================================
    // int32
    // ============================================================

    std::optional<int32> conv<int32>::read(const node* n) const noexcept
    {
        return read_integer<int32>(n);
    }

    void conv<int32>::write(node* parent, core::string_view key, const int32& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<int32>::description() const noexcept
    {
        return "int32 [-2147483648..2147483647]";
    }


    // ============================================================
    // uint64
    // ============================================================

    std::optional<uint64> conv<uint64>::read(const node* n) const noexcept
    {
        return n ? n->value<uint64>() : std::nullopt;
    }

    void conv<uint64>::write(node* parent, core::string_view key, const uint64& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<uint64>::description() const noexcept
    {
        return "uint64 [0..18446744073709551615]";
    }


    // ============================================================
    // int64
    // ============================================================

    std::optional<int64> conv<int64>::read(const node* n) const noexcept
    {
        return n ? n->value<int64>() : std::nullopt;
    }

    void conv<int64>::write(node* parent, core::string_view key, const int64& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<int64>::description() const noexcept
    {
        return "int64 [-9223372036854775808..9223372036854775807]";
    }


    // ============================================================
    // float
    // ============================================================

    std::optional<float> conv<float>::read(const node* n) const noexcept
    {
        return n ? n->value<float>() : std::nullopt;
    }

    void conv<float>::write(node* parent, core::string_view key, const float& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<float>::description() const noexcept
    {
        return "float";
    }


    // ============================================================
    // double
    // ============================================================

    std::optional<double> conv<double>::read(const node* n) const noexcept
    {
        return n ? n->value<double>() : std::nullopt;
    }

    void conv<double>::write(node* parent, core::string_view key, const double& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<double>::description() const noexcept
    {
        return "double";
    }


    // ============================================================
    // string_view
    // ============================================================

    std::optional<core::string_view> conv<core::string_view>::read(const node* n) const noexcept
    {
        return n ? n->value<core::string_view>() : std::nullopt;
    }

    void conv<core::string_view>::write(node* parent, core::string_view key, const core::string_view& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<core::string_view>::description() const noexcept
    {
        return "string_view";
    }


    // ============================================================
    // string
    // ============================================================

    std::optional<core::string> conv<core::string>::read(const node* n) const noexcept
    {
        return n ? n->value<core::string>() : std::nullopt;
    }

    void conv<core::string>::write(node* parent, core::string_view key, const core::string& val) const
    {
        write_value(parent, key, val);
    }

    core::string_view conv<core::string>::description() const noexcept
    {
        return "string";
    }


    // ============================================================
    // euler3
    // ============================================================

    std::optional<math::euler3> conv<math::euler3>::read(const node* n) const noexcept
    {
        const auto* roll = n;
        const auto* pitch = next_cont(roll);
        const auto* yaw = next_cont(pitch);

        if (!valid_n(roll) || !valid_cont(pitch) || !valid_cont(yaw)) {
            return std::nullopt;
        }
        return math::euler3(
            roll->value_or(0.0f),
            pitch->value_or(0.0f),
            yaw->value_or(0.0f)
        );
    }

    void conv<math::euler3>::write(node* parent, core::string_view key, const math::euler3& val) const
    {
        TAV_ASSERT(parent);

        if (parent) {
            parent->append(key, val.roll);
            parent->append("", val.pitch);
            parent->append("", val.yaw);
        }
    }

    core::string_view conv<math::euler3>::description() const noexcept
    {
        return "euler3 (roll pitch yaw)";
    }


    // ============================================================
    // vec2
    // ============================================================

    std::optional<math::vec2> conv<math::vec2>::read(const node* n) const noexcept
    {
        const auto* x = n;
        const auto* y = next_cont(x);

        if (!valid_n(x) || !valid_cont(y)) {
            return std::nullopt;
        }
        return math::vec2(x->value_or(0.0f), y->value_or(0.0f));
    }

    void conv<math::vec2>::write(node* parent, core::string_view key, const math::vec2& val) const
    {
        TAV_ASSERT(parent);

        if (parent) {
            parent->append(key, val.x);
            parent->append("", val.y);
        }
    }

    core::string_view conv<math::vec2>::description() const noexcept
    {
        return "vec2 (x y)";
    }


    // ============================================================
    // vec3
    // ============================================================

    std::optional<math::vec3> conv<math::vec3>::read(const node* n) const noexcept
    {
        const auto* x = n;
        const auto* y = next_cont(x);
        const auto* z = next_cont(y);

        if (!valid_n(x) || !valid_cont(y) || !valid_cont(z)) {
            return std::nullopt;
        }
        return math::vec3(
            x->value_or(0.0f),
            y->value_or(0.0f),
            z->value_or(0.0f)
        );
    }

    void conv<math::vec3>::write(node* parent, core::string_view key, const math::vec3& val) const
    {
        TAV_ASSERT(parent);

        if (parent) {
            parent->append(key, val.x);
            parent->append("", val.y);
            parent->append("", val.z);
        }
    }

    core::string_view conv<math::vec3>::description() const noexcept
    {
        return "vec3 (x y z)";
    }


    // ============================================================
    // vec4
    // ============================================================

    std::optional<math::vec4> conv<math::vec4>::read(const node* n) const noexcept
    {
        const auto* x = n;
        const auto* y = next_cont(x);
        const auto* z = next_cont(y);
        const auto* w = next_cont(z);

        if (!valid_n(x) || !valid_cont(y) || !valid_cont(z) || !valid_cont(w)) {
            return std::nullopt;
        }
        return math::vec4(
            x->value_or(0.0f),
            y->value_or(0.0f),
            z->value_or(0.0f),
            w->value_or(0.0f)
        );
    }

    void conv<math::vec4>::write(node* parent, core::string_view key, const math::vec4& val) const
    {
        TAV_ASSERT(parent);

        if (parent) {
            parent->append(key, val.x);
            parent->append("", val.y);
            parent->append("", val.z);
            parent->append("", val.w);
        }
    }

    core::string_view conv<math::vec4>::description() const noexcept
    {
        return "vec4 (x y z w)";
    }


    // ============================================================
    // quat
    // ============================================================

    std::optional<math::quat> conv<math::quat>::read(const node* n) const noexcept
    {
        const auto* x = n;
        const auto* y = next_cont(x);
        const auto* z = next_cont(y);
        const auto* w = next_cont(z);

        if (!valid_n(x) || !valid_cont(y) || !valid_cont(z) || !valid_cont(w)) {
            return std::nullopt;
        }
        return math::quat(
            x->value_or(0.0f),
            y->value_or(0.0f),
            z->value_or(0.0f),
            w->value_or(0.0f)
        );
    }

    void conv<math::quat>::write(node* parent, core::string_view key, const math::quat& val) const
    {
        TAV_ASSERT(parent);

        if (parent) {
            parent->append(key, val.x);
            parent->append("", val.y);
            parent->append("", val.z);
            parent->append("", val.w);
        }
    }

    core::string_view conv<math::quat>::description() const noexcept
    {
        return "quat (x y z w)";
    }


    // ============================================================
    // ivec2
    // ============================================================

    std::optional<math::ivec2> conv<math::ivec2>::read(const node* n) const noexcept
    {
        const auto* x = n;
        const auto* y = next_cont(x);

        if (!valid_n(x) || !valid_cont(y)) {
            return std::nullopt;
        }
        auto opt_x = read_integer<int32>(x);
        auto opt_y = read_integer<int32>(y);
        if (opt_x.has_value() && opt_y.has_value()) {
            return math::ivec2(*opt_x, *opt_y);
        }
        return std::nullopt;
    }

    void conv<math::ivec2>::write(node* parent, core::string_view key, const math::ivec2& val) const
    {
        TAV_ASSERT(parent);

        if (parent) {
            parent->append(key, val.x);
            parent->append("", val.y);
        }
    }

    core::string_view conv<math::ivec2>::description() const noexcept
    {
        return "ivec2 (int32 x int32 y)";
    }


    // ============================================================
    // rgba8
    // ============================================================

    std::optional<math::rgba8> conv<math::rgba8>::read(const node* n) const noexcept
    {
        if (!valid_n(n)) {
            return std::nullopt;
        }
        auto v = n->value_or<uint64>(0);
        if (v > 0xffffffffull) {
            return std::nullopt;
        }
        return math::rgba8(static_cast<uint32>(v));
    }

    void conv<math::rgba8>::write(node* parent, core::string_view key, const math::rgba8& val) const
    {
        TAV_ASSERT(parent);

        if (parent) {
            parent->append(key, static_cast<uint64>(val.color));
        }
    }

    core::string_view conv<math::rgba8>::description() const noexcept
    {
        return "rgba8 (0xRRGGBBAA)";
    }


    // ============================================================
    // mat3
    // ============================================================

    std::optional<math::mat3> conv<math::mat3>::read(const node* n) const noexcept
    {
        const node* c[9];
        c[0] = n;
        for (int i = 1; i < 9; ++i) {
            c[i] = next_cont(c[i - 1]);
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

    void conv<math::mat3>::write(node* parent, core::string_view key, const math::mat3& val) const
    {
        TAV_ASSERT(parent);

        if (parent) {
            parent->append(key, val[0][0]);
            parent->append("", val[0][1]);
            parent->append("", val[0][2]);
            parent->append("", val[1][0]);
            parent->append("", val[1][1]);
            parent->append("", val[1][2]);
            parent->append("", val[2][0]);
            parent->append("", val[2][1]);
            parent->append("", val[2][2]);
        }
    }

    core::string_view conv<math::mat3>::description() const noexcept
    {
        return "mat3 (row-major, 9 floats)";
    }


    // ============================================================
    // mat4
    // ============================================================

    std::optional<math::mat4> conv<math::mat4>::read(const node* n) const noexcept
    {
        const node* c[16];
        c[0] = n;
        for (int i = 1; i < 16; ++i) {
            c[i] = next_cont(c[i - 1]);
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

    void conv<math::mat4>::write(node* parent, core::string_view key, const math::mat4& val) const
    {
        TAV_ASSERT(parent);

        if (parent) {
            parent->append(key, val[0][0]);
            parent->append("", val[0][1]);
            parent->append("", val[0][2]);
            parent->append("", val[0][3]);
            parent->append("", val[1][0]);
            parent->append("", val[1][1]);
            parent->append("", val[1][2]);
            parent->append("", val[1][3]);
            parent->append("", val[2][0]);
            parent->append("", val[2][1]);
            parent->append("", val[2][2]);
            parent->append("", val[2][3]);
            parent->append("", val[3][0]);
            parent->append("", val[3][1]);
            parent->append("", val[3][2]);
            parent->append("", val[3][3]);
        }
    }

    core::string_view conv<math::mat4>::description() const noexcept
    {
        return "mat4 (row-major, 16 floats)";
    }
} // namespace tavros::tef
