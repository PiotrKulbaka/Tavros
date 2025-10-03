#include <tavros/core/utils/to_string.hpp>

namespace tavros::core
{

    string_view uint32_to_base64(uint32 u)
    {
        static const char        base64_alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        thread_local static char s[] = "000000";
        for (auto i = 0; i < 6; ++i) {
            s[5 - i] = base64_alpha[u & 0x3f];
            u >>= 6;
        }
        return string_view(s, 6);
    }

} // namespace tavros::core
