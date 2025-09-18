#pragma once

#include <tavros/renderer/interfaces/gl_context.hpp>

#import <Cocoa/Cocoa.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace tavros::renderer
{
    class gl_context : public interfaces::gl_context, core::noncopyable, core::nonmovable
    {
    public:
        gl_context(NSView* ns_view);

        virtual ~gl_context() override;

        virtual void make_current() override;

        virtual void make_inactive() override;

        virtual void swap_buffers() override;

        virtual bool is_current() override;

    private:
        NSView*          m_ns_view;
        NSOpenGLContext* m_context;
    };
} // namespace tavros::renderer

#pragma clang diagnostic pop
