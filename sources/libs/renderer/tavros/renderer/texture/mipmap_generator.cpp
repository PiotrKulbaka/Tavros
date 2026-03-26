#include <tavros/renderer/texture/mipmap_generator.hpp>

#include <tavros/core/math/functions/basic_math.hpp>

namespace tavros::renderer
{

    assets::image mipmap_generator::generate_level(assets::image_view base, uint32 level, bool srgb)
    {
        uint32 mw = std::max(1u, base.width() >> level);
        uint32 mh = std::max(1u, base.height() >> level);
        return base.resize(mw, mh, srgb);
    }

    mipmap_generator::mipmap_levels mipmap_generator::generate(assets::image_view base, uint32 levels, bool srgb)
    {
        TAV_ASSERT(base.valid());
        auto levels_size = math::mip_levels(base.width(), base.height());

        if (levels == 0) {
            levels = levels_size;
        }
        TAV_ASSERT(levels <= levels_size);

        mipmap_levels mips;
        for (uint32 i = 1; i < levels; ++i) {
            mips.push_back(generate_level(base, i, srgb));
        }

        return mips;
    }

} // namespace tavros::renderer
