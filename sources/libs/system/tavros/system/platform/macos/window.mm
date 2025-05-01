#include <tavros/system/platform/macos/window.hpp>

#include <tavros/system/platform/macos/utils.hpp>

using namespace tavros::system;

@interface TavrosWindowDelegate : NSObject <NSWindowDelegate> {
    tavros::system::window* m_owner;
}

- (instancetype)initWithOwner:(tavros::system::window*)owner;
@end

@implementation TavrosWindowDelegate

- (instancetype)initWithOwner:(tavros::system::window*)owner
{
    self = [super init];
    if (self) {
        m_owner = owner;
    }
    return self;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)windowShouldClose:(NSWindow*)sender
{
    tavros::system::close_event_args args{};
    m_owner->on_close(args);
    if (args.cancel) {
        return NO;
    }
    return YES;
}

- (void)windowDidMove:(NSNotification*)notification
{
    auto                            loc = m_owner->get_location();
    tavros::system::move_event_args args{loc};
    m_owner->on_move(args);
}

- (void)windowDidResize:(NSNotification*)notification
{
    auto                            sz = m_owner->get_window_size();
    tavros::system::size_event_args args{sz};
    m_owner->on_resize(args);
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
    m_owner->on_activate();
}

- (void)windowDidResignKey:(NSNotification*)notification
{
    m_owner->on_deactivate();
}

@end


@interface WindowView : NSView {
    tavros::system::window* m_owner;
    NSTrackingArea*         m_tracking_area;
    tavros::math::point2    m_prev_cursor_pos;
}

- (instancetype)initWithOwner:(tavros::system::window*)owner;

- (tavros::math::point2)getCursorPos:(NSEvent*)event;
- (mouse_event_args)makeMouseEventArgsFromEvent:(NSEvent*)fromEvent withButton:(mouse_button)button;
- (mouse_button)mapMouseButton:(NSInteger)buttonId;

@end


@implementation WindowView

- (instancetype)initWithOwner:(tavros::system::window*)owner
{
    self = [super initWithFrame:NSZeroRect];
    if (self != nil) {
        m_owner = owner;
        m_tracking_area = nil;
        m_prev_cursor_pos = tavros::math::point2(0.0f, 0.0f);

        [self updateTrackingAreas];
        [self setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
    }
    return self;
}

- (void)dealloc
{
    [m_tracking_area release];
    m_tracking_area = nil;
    [super dealloc];
}

- (void)updateTrackingAreas
{
    if (m_tracking_area != nil) {
        [self removeTrackingArea:m_tracking_area];
        [m_tracking_area release];
    }

    const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow | NSTrackingEnabledDuringMouseDrag | NSTrackingCursorUpdate | NSTrackingInVisibleRect;

    m_tracking_area = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                   options:options
                                                     owner:self
                                                  userInfo:nil];

    [self addTrackingArea:m_tracking_area];
    [super updateTrackingAreas];
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (tavros::math::point2)getCursorPos:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    NSRect  frame = [self frame];
    return tavros::math::point2{
        static_cast<float>(location.x),
        static_cast<float>(frame.size.height - location.y)
    };
}

- (mouse_event_args)makeMouseEventArgsFromEvent:(NSEvent*)event withButton:(mouse_button)button
{
    mouse_event_args args{button, ([event clickCount] == 2), false, {0.0f, 0.0f}, [self getCursorPos:event]};
    return args;
}

- (mouse_button)mapMouseButton:(NSInteger)buttonId
{
    switch (buttonId) {
    case 2:
        return mouse_button::middle;
    case 3:
        return mouse_button::x_button1;
    case 4:
        return mouse_button::x_button2;
    default:
        return mouse_button::none;
    }
}

- (void)mouseDown:(NSEvent*)event
{
    mouse_event_args args = [self makeMouseEventArgsFromEvent:event withButton:mouse_button::left];
    m_owner->on_mouse_down(args);
}

- (void)mouseUp:(NSEvent*)event
{
    mouse_event_args args = [self makeMouseEventArgsFromEvent:event withButton:mouse_button::left];
    m_owner->on_mouse_up(args);
}

- (void)mouseDragged:(NSEvent*)event
{
    [self mouseMoved:event];
}

- (void)rightMouseDown:(NSEvent*)event
{
    mouse_event_args args = [self makeMouseEventArgsFromEvent:event withButton:mouse_button::right];
    m_owner->on_mouse_down(args);
}

- (void)rightMouseUp:(NSEvent*)event
{
    mouse_event_args args = [self makeMouseEventArgsFromEvent:event withButton:mouse_button::right];
    m_owner->on_mouse_up(args);
}

- (void)rightMouseDragged:(NSEvent*)event
{
    [self mouseMoved:event];
}

- (void)otherMouseDown:(NSEvent*)event
{
    mouse_button btn = [self mapMouseButton:[event buttonNumber]];
    if (btn != mouse_button::none) {
        mouse_event_args args = [self makeMouseEventArgsFromEvent:event withButton:btn];
        m_owner->on_mouse_down(args);
    }
}

- (void)otherMouseUp:(NSEvent*)event
{
    mouse_button btn = [self mapMouseButton:[event buttonNumber]];
    if (btn != mouse_button::none) {
        mouse_event_args args = [self makeMouseEventArgsFromEvent:event withButton:btn];
        m_owner->on_mouse_up(args);
    }
}

- (void)otherMouseDragged:(NSEvent*)event
{
    [self mouseMoved:event];
}

- (void)mouseMoved:(NSEvent*)event
{
    tavros::math::point2 current_cursor_pos = [self getCursorPos:event];
    tavros::math::point2 delta = current_cursor_pos - m_prev_cursor_pos;
    m_prev_cursor_pos = current_cursor_pos;
    mouse_event_args args = [self makeMouseEventArgsFromEvent:event withButton:mouse_button::none];
    m_owner->on_mouse_move(args);
    args.is_relative_move = true;
    args.pos = delta;
    m_owner->on_mouse_move(args);
}

- (void)scrollWheel:(NSEvent*)event
{
    auto             wheel = tavros::math::point2(static_cast<float>([event scrollingDeltaX]), static_cast<float>([event scrollingDeltaY]));
    mouse_event_args args{mouse_button::none, false, false, wheel, [self getCursorPos:event]};
    m_owner->on_mouse_wheel(args);
}

- (void)keyDown:(NSEvent*)event
{
    tavros::system::key_event_args args;
    args.key = tavros::system::map_key(static_cast<uint16>([event keyCode]));
    args.is_prev_pressed = [event isARepeat];
    args.repeats = 1;
    args.key_char = 0;
    if (args.key != keys::none) {
        m_owner->on_key_down(args);
    }
    int32 c = [[event charactersIgnoringModifiers] length] > 0 ? [[event charactersIgnoringModifiers] characterAtIndex:0] : 0;
    if (c != 0) {
        args.key_char = c;
        m_owner->on_key_press(args);
    }
}

- (void)keyUp:(NSEvent*)event
{
    tavros::system::key_event_args args;
    args.key = tavros::system::map_key(static_cast<uint16>([event keyCode]));
    args.is_prev_pressed = true;
    args.repeats = 0;
    args.key_char = 0;
    if (args.key != keys::none) {
        m_owner->on_key_up(args);
    }
}

@end


tavros::core::unique_ptr<tavros::system::interfaces::window> tavros::system::interfaces::window::create(tavros::core::string_view name)
{
    return tavros::core::make_unique<tavros::system::window>(name);
}

window::window(tavros::core::string_view name)
{
    // Создаем объект окна
    m_window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 600)
                                           styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
    m_delegate = [[TavrosWindowDelegate alloc] initWithOwner:this];
    [m_window setDelegate:m_delegate];

    [m_window setAcceptsMouseMovedEvents:YES];

    WindowView* view = [[WindowView alloc] initWithOwner:this];
    [m_window setContentView:view];
    [view release];


    // Устанавливаем заголовок окна
    [m_window setTitle:[NSString stringWithUTF8String:name.data()]];
}

window::~window()
{
    [m_window setDelegate:nil];
    [m_delegate release];
    [m_window release];
}

void window::set_text(tavros::core::string_view text)
{
    [m_window setTitle:[NSString stringWithUTF8String:text.data()]];
}

tavros::system::window_state window::get_window_state()
{
    if ([m_window isMiniaturized]) {
        return window_state::minimized;
    }
    if ([m_window styleMask] & NSWindowStyleMaskFullScreen) {
        return window_state::maximized;
    }
    return window_state::normal;
}

void window::set_window_state(tavros::system::window_state ws)
{
    switch (ws) {
    case window_state::minimized:
        [m_window miniaturize:nil];
        break;
    case window_state::maximized:
        [m_window toggleFullScreen:nil];
        break;
    case window_state::normal:
        if ([m_window isMiniaturized]) {
            [m_window deminiaturize:nil];
        }
        break;
    }
}

tavros::math::isize2 window::get_window_size()
{
    NSRect frame = [m_window frame];
    return math::isize2{static_cast<int32>(frame.size.width), static_cast<int32>(frame.size.height)};
}

void window::set_window_size(int32 width, int32 height)
{
    NSRect frame = [m_window frame];
    frame.size = NSMakeSize(width, height);
    [m_window setFrame:frame display:YES];
}

tavros::math::ipoint2 window::get_location()
{
    NSScreen* screen = [m_window screen];
    if (!screen) {
        screen = [NSScreen mainScreen];
    }

    NSRect screen_frame = [screen frame];
    NSRect window_frame = [m_window frame];

    int32 left = static_cast<int32>(window_frame.origin.x);
    int32 top = static_cast<int32>(screen_frame.origin.y + screen_frame.size.height - window_frame.origin.y - window_frame.size.height);

    return tavros::math::ipoint2{left, top};
}

void window::set_location(int32 left, int32 top)
{
    NSScreen* screen = [m_window screen];
    if (!screen) {
        screen = [NSScreen mainScreen];
    }

    NSRect screen_frame = [screen frame];
    NSRect window_frame = [m_window frame];

    CGFloat new_origin_y = screen_frame.origin.y + screen_frame.size.height - top - window_frame.size.height;

    [m_window setFrameOrigin:NSMakePoint(left, new_origin_y)];
}

tavros::math::isize2 window::get_client_size()
{
    NSView* content_view = [m_window contentView];
    NSRect  frame = [content_view frame];
    return {static_cast<int32>(frame.size.width), static_cast<int32>(frame.size.height)};
}

void window::set_client_size(int32 width, int32 height)
{
    set_window_size(width, height);
}

bool window::is_enabled()
{
    return [m_window isVisible] && [m_window isKeyWindow] && [m_window isMainWindow];
}

void window::set_enabled(bool enable)
{
    [m_window setIgnoresMouseEvents:!enable];
}

void window::activate()
{
    [NSApp activateIgnoringOtherApps:YES];
    [m_window makeKeyAndOrderFront:nil];
}

void window::show()
{
    [m_window orderFront:nil];
}

void window::hide()
{
    [m_window orderOut:nil];
}

void window::close()
{
    [m_window close];
}

void window::set_on_close_listener(close_callback cb)
{
    m_on_close_cb = std::move(cb);
}

void window::set_on_activate_listener(event_callback cb)
{
    m_on_activate_cb = std::move(cb);
}

void window::set_on_deactivate_listener(event_callback cb)
{
    m_on_deactivate_cb = std::move(cb);
}

void window::set_on_drop_listener(drop_callback cb)
{
    TAV_ASSERT(false && "Not implemented yet.");
    m_on_drop_cb = std::move(cb);
}

void window::set_on_move_listener(move_callback cb)
{
    m_on_move_cb = std::move(cb);
}

void window::set_on_resize_listener(size_callback cb)
{
    m_on_resize_cb = std::move(cb);
}

void window::set_on_mouse_down_listener(mouse_callback cb)
{
    m_on_mouse_down_cb = std::move(cb);
}

void window::set_on_mouse_move_listener(mouse_callback cb)
{
    m_on_mouse_move_cb = std::move(cb);
}

void window::set_on_mouse_up_listener(mouse_callback cb)
{
    m_on_mouse_up_cb = std::move(cb);
}

void window::set_on_mouse_wheel_listener(mouse_callback cb)
{
    m_on_mouse_wheel_cb = std::move(cb);
}

void window::set_on_key_down_listener(key_callback cb)
{
    m_on_key_down_cb = std::move(cb);
}

void window::set_on_key_up_listener(key_callback cb)
{
    m_on_key_up_cb = std::move(cb);
}

void window::set_on_key_press_listener(key_callback cb)
{
    m_on_key_press_cb = std::move(cb);
}

void window::on_close(close_event_args& e)
{
    if (m_on_close_cb) {
        m_on_close_cb(this, e);
    }
}

void window::on_activate()
{
    if (m_on_activate_cb) {
        m_on_activate_cb(this);
    }
}

void window::on_deactivate()
{
    if (m_on_deactivate_cb) {
        m_on_deactivate_cb(this);
    }
}

void window::on_drop(drop_event_args& e)
{
    if (m_on_drop_cb) {
        m_on_drop_cb(this, e);
    }
}

void window::on_move(move_event_args& e)
{
    if (m_on_move_cb) {
        m_on_move_cb(this, e);
    }
}

void window::on_resize(size_event_args& e)
{
    if (m_on_resize_cb) {
        m_on_resize_cb(this, e);
    }
}

void window::on_mouse_down(mouse_event_args& e)
{
    if (m_on_mouse_down_cb) {
        m_on_mouse_down_cb(this, e);
    }
}

void window::on_mouse_move(mouse_event_args& e)
{
    if (m_on_mouse_move_cb) {
        m_on_mouse_move_cb(this, e);
    }
}

void window::on_mouse_up(mouse_event_args& e)
{
    if (m_on_mouse_up_cb) {
        m_on_mouse_up_cb(this, e);
    }
}

void window::on_mouse_wheel(mouse_event_args& e)
{
    if (m_on_mouse_wheel_cb) {
        m_on_mouse_wheel_cb(this, e);
    }
}

void window::on_key_down(key_event_args& e)
{
    if (m_on_key_down_cb) {
        m_on_key_down_cb(this, e);
    }
}

void window::on_key_up(key_event_args& e)
{
    if (m_on_key_up_cb) {
        m_on_key_up_cb(this, e);
    }
}

void window::on_key_press(key_event_args& e)
{
    if (m_on_key_press_cb) {
        m_on_key_press_cb(this, e);
    }
}

handle window::get_handle() const
{
    return reinterpret_cast<handle>(m_window);
}
