#include <tavros/renderer/internal/platform/macos/gl_context.hpp>

#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

using namespace tavros::renderer;


gl_context_uptr interfaces::gl_context::create(handle ns_view)
{
    return tavros::core::make_unique<tavros::renderer::gl_context>(static_cast<NSView*>(ns_view));
}

gl_context::gl_context(NSView* ns_view)
    : m_ns_view(ns_view)
    , m_context(nullptr)
{
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAStencilSize, 8,
        NSOpenGLPFADoubleBuffer,
        0
    };

    NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (!format) {
        throw std::runtime_error("Unable to create NSOpenGLPixelFormat");
    }

    NSOpenGLContext* context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
    [format release];

    if (!context) {
        throw std::runtime_error("Unable to create NSOpenGLContext");
    }

    m_context = context;
    [m_context retain]; // Чтобы сохранить владение
    [m_context setView:m_ns_view];
}

gl_context::~gl_context()
{
    if (m_context) {
        [m_context clearDrawable];
        [m_context release];
        m_context = nullptr;
    }
    m_ns_view = nullptr;
}

void gl_context::make_current()
{
    [m_context makeCurrentContext];
}

void gl_context::make_inactive()
{
    [NSOpenGLContext clearCurrentContext];
}

void gl_context::swap_buffers()
{
    [m_context flushBuffer];
}

bool gl_context::is_current()
{
    return [NSOpenGLContext currentContext] == m_context;
}

#pragma clang diagnostic pop
