#include <tavros/system/platform/macos/application.hpp>

using namespace tavros::system;

application_uptr interfaces::application::create()
{
    return tavros::core::make_unique<tavros::system::application>();
}

application::application()
    : m_is_running(false)
{
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
}

application::~application()
{
}

void application::run()
{
    m_is_running = true;
    [NSApp finishLaunching];
    [NSApp activateIgnoringOtherApps:YES];
}

void application::exit()
{
    m_is_running = false;
    [NSApp stop:nil];
}

bool application::is_runing()
{
    return m_is_running;
}

void application::poll_events()
{
    @autoreleasepool {
        NSEvent* event;
        do {
            event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                       untilDate:[NSDate distantPast]
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES];
            if (event != nil) {
                [NSApp sendEvent:event];
                [NSApp updateWindows];
            }
        } while (event != nil);
    }
}
