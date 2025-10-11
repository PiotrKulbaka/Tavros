#include <tavros/core/utils/to_string.hpp>

namespace tavros::core
{

    string_view uint64_to_base64(uint64 u)
    {
        static const char        base64_alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        thread_local static char s[] = "000000000000";
        for (auto i = 0; i < 12; ++i) {
            s[12 - i - 1] = base64_alpha[u & 0x3f];
            u >>= 6;
        }
        return string_view(s, 12);
    }

} // namespace tavros::core
