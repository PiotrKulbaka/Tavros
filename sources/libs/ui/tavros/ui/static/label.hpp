#pragma once

#include <tavros/ui/view.hpp>

#include <tavros/core/string.hpp>

namespace tavros::ui
{

    class label : public view
    {
    public:
        label();
        ~label() override;

        void set_text(std::string_view str);

        void update(float dt) override;
        void draw(const render_context& rctx) override;

    private:
        void create_res(rhi::graphics_device* gdevice);

    private:
        core::string m_text;

        bool m_resources_created = false;
    };


} // namespace tavros::ui
