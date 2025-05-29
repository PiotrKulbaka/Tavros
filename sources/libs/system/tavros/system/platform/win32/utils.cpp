#include <tavros/system/platform/win32/utils.hpp>

#include <Windowsx.h>

#include <cstring>

using namespace tavros::system;

const char* tavros::system::wm_message_to_str(UINT msg)
{
    switch (msg) {
    case WM_NULL:
        return "WM_NULL";
    case WM_CREATE:
        return "WM_CREATE";
    case WM_DESTROY:
        return "WM_DESTROY";
    case WM_MOVE:
        return "WM_MOVE";
    case WM_SIZE:
        return "WM_SIZE";
    case WM_ACTIVATE:
        return "WM_ACTIVATE";
    case WM_SETFOCUS:
        return "WM_SETFOCUS";
    case WM_KILLFOCUS:
        return "WM_KILLFOCUS";
    case WM_ENABLE:
        return "WM_ENABLE";
    case WM_SETREDRAW:
        return "WM_SETREDRAW";
    case WM_SETTEXT:
        return "WM_SETTEXT";
    case WM_GETTEXT:
        return "WM_GETTEXT";
    case WM_GETTEXTLENGTH:
        return "WM_GETTEXTLENGTH";
    case WM_PAINT:
        return "WM_PAINT";
    case WM_CLOSE:
        return "WM_CLOSE";
    case WM_QUERYENDSESSION:
        return "WM_QUERYENDSESSION";
    case WM_QUERYOPEN:
        return "WM_QUERYOPEN";
    case WM_ENDSESSION:
        return "WM_ENDSESSION";
    case WM_QUIT:
        return "WM_QUIT";
    case WM_ERASEBKGND:
        return "WM_ERASEBKGND";
    case WM_SYSCOLORCHANGE:
        return "WM_SYSCOLORCHANGE";
    case WM_SHOWWINDOW:
        return "WM_SHOWWINDOW";
    case WM_WININICHANGE:
        return "WM_WININICHANGE";
    case WM_DEVMODECHANGE:
        return "WM_DEVMODECHANGE";
    case WM_ACTIVATEAPP:
        return "WM_ACTIVATEAPP";
    case WM_FONTCHANGE:
        return "WM_FONTCHANGE";
    case WM_TIMECHANGE:
        return "WM_TIMECHANGE";
    case WM_CANCELMODE:
        return "WM_CANCELMODE";
    case WM_SETCURSOR:
        return "WM_SETCURSOR";
    case WM_MOUSEACTIVATE:
        return "WM_MOUSEACTIVATE";
    case WM_CHILDACTIVATE:
        return "WM_CHILDACTIVATE";
    case WM_QUEUESYNC:
        return "WM_QUEUESYNC";
    case WM_GETMINMAXINFO:
        return "WM_GETMINMAXINFO";
    case WM_PAINTICON:
        return "WM_PAINTICON";
    case WM_ICONERASEBKGND:
        return "WM_ICONERASEBKGND";
    case WM_NEXTDLGCTL:
        return "WM_NEXTDLGCTL";
    case WM_SPOOLERSTATUS:
        return "WM_SPOOLERSTATUS";
    case WM_DRAWITEM:
        return "WM_DRAWITEM";
    case WM_MEASUREITEM:
        return "WM_MEASUREITEM";
    case WM_DELETEITEM:
        return "WM_DELETEITEM";
    case WM_VKEYTOITEM:
        return "WM_VKEYTOITEM";
    case WM_CHARTOITEM:
        return "WM_CHARTOITEM";
    case WM_SETFONT:
        return "WM_SETFONT";
    case WM_GETFONT:
        return "WM_GETFONT";
    case WM_SETHOTKEY:
        return "WM_SETHOTKEY";
    case WM_GETHOTKEY:
        return "WM_GETHOTKEY";
    case WM_QUERYDRAGICON:
        return "WM_QUERYDRAGICON";
    case WM_COMPAREITEM:
        return "WM_COMPAREITEM";
    case WM_GETOBJECT:
        return "WM_GETOBJECT";
    case WM_COMPACTING:
        return "WM_COMPACTING";
    case WM_COMMNOTIFY:
        return "WM_COMMNOTIFY";
    case WM_WINDOWPOSCHANGING:
        return "WM_WINDOWPOSCHANGING";
    case WM_WINDOWPOSCHANGED:
        return "WM_WINDOWPOSCHANGED";
    case WM_POWER:
        return "WM_POWER";
    case WM_COPYDATA:
        return "WM_COPYDATA";
    case WM_CANCELJOURNAL:
        return "WM_CANCELJOURNAL";
    case WM_NOTIFY:
        return "WM_NOTIFY";
    case WM_INPUTLANGCHANGEREQUEST:
        return "WM_INPUTLANGCHANGEREQUEST";
    case WM_INPUTLANGCHANGE:
        return "WM_INPUTLANGCHANGE";
    case WM_TCARD:
        return "WM_TCARD";
    case WM_HELP:
        return "WM_HELP";
    case WM_USERCHANGED:
        return "WM_USERCHANGED";
    case WM_NOTIFYFORMAT:
        return "WM_NOTIFYFORMAT";
    case WM_CONTEXTMENU:
        return "WM_CONTEXTMENU";
    case WM_STYLECHANGING:
        return "WM_STYLECHANGING";
    case WM_STYLECHANGED:
        return "WM_STYLECHANGED";
    case WM_DISPLAYCHANGE:
        return "WM_DISPLAYCHANGE";
    case WM_GETICON:
        return "WM_GETICON";
    case WM_SETICON:
        return "WM_SETICON";
    case WM_NCCREATE:
        return "WM_NCCREATE";
    case WM_NCDESTROY:
        return "WM_NCDESTROY";
    case WM_NCCALCSIZE:
        return "WM_NCCALCSIZE";
    case WM_NCHITTEST:
        return "WM_NCHITTEST";
    case WM_NCPAINT:
        return "WM_NCPAINT";
    case WM_NCACTIVATE:
        return "WM_NCACTIVATE";
    case WM_GETDLGCODE:
        return "WM_GETDLGCODE";
    case WM_SYNCPAINT:
        return "WM_SYNCPAINT";
    case WM_NCMOUSEMOVE:
        return "WM_NCMOUSEMOVE";
    case WM_NCLBUTTONDOWN:
        return "WM_NCLBUTTONDOWN";
    case WM_NCLBUTTONUP:
        return "WM_NCLBUTTONUP";
    case WM_NCLBUTTONDBLCLK:
        return "WM_NCLBUTTONDBLCLK";
    case WM_NCRBUTTONDOWN:
        return "WM_NCRBUTTONDOWN";
    case WM_NCRBUTTONUP:
        return "WM_NCRBUTTONUP";
    case WM_NCRBUTTONDBLCLK:
        return "WM_NCRBUTTONDBLCLK";
    case WM_NCMBUTTONDOWN:
        return "WM_NCMBUTTONDOWN";
    case WM_NCMBUTTONUP:
        return "WM_NCMBUTTONUP";
    case WM_NCMBUTTONDBLCLK:
        return "WM_NCMBUTTONDBLCLK";
    case WM_NCXBUTTONDOWN:
        return "WM_NCXBUTTONDOWN";
    case WM_NCXBUTTONUP:
        return "WM_NCXBUTTONUP";
    case WM_NCXBUTTONDBLCLK:
        return "WM_NCXBUTTONDBLCLK";
    case WM_INPUT_DEVICE_CHANGE:
        return "WM_INPUT_DEVICE_CHANGE";
    case WM_INPUT:
        return "WM_INPUT";
    case WM_KEYDOWN:
        return "WM_KEYDOWN";
    case WM_KEYUP:
        return "WM_KEYUP";
    case WM_CHAR:
        return "WM_CHAR";
    case WM_DEADCHAR:
        return "WM_DEADCHAR";
    case WM_SYSKEYDOWN:
        return "WM_SYSKEYDOWN";
    case WM_SYSKEYUP:
        return "WM_SYSKEYUP";
    case WM_SYSCHAR:
        return "WM_SYSCHAR";
    case WM_SYSDEADCHAR:
        return "WM_SYSDEADCHAR";
    case WM_UNICHAR:
        return "WM_UNICHAR";
    case WM_IME_STARTCOMPOSITION:
        return "WM_IME_STARTCOMPOSITION";
    case WM_IME_ENDCOMPOSITION:
        return "WM_IME_ENDCOMPOSITION";
    case WM_IME_COMPOSITION:
        return "WM_IME_COMPOSITION";
    case WM_INITDIALOG:
        return "WM_INITDIALOG";
    case WM_COMMAND:
        return "WM_COMMAND";
    case WM_SYSCOMMAND:
        return "WM_SYSCOMMAND";
    case WM_TIMER:
        return "WM_TIMER";
    case WM_HSCROLL:
        return "WM_HSCROLL";
    case WM_VSCROLL:
        return "WM_VSCROLL";
    case WM_INITMENU:
        return "WM_INITMENU";
    case WM_INITMENUPOPUP:
        return "WM_INITMENUPOPUP";
    case WM_GESTURE:
        return "WM_GESTURE";
    case WM_GESTURENOTIFY:
        return "WM_GESTURENOTIFY";
    case WM_MENUSELECT:
        return "WM_MENUSELECT";
    case WM_MENUCHAR:
        return "WM_MENUCHAR";
    case WM_ENTERIDLE:
        return "WM_ENTERIDLE";
    case WM_MENURBUTTONUP:
        return "WM_MENURBUTTONUP";
    case WM_MENUDRAG:
        return "WM_MENUDRAG";
    case WM_MENUGETOBJECT:
        return "WM_MENUGETOBJECT";
    case WM_UNINITMENUPOPUP:
        return "WM_UNINITMENUPOPUP";
    case WM_MENUCOMMAND:
        return "WM_MENUCOMMAND";
    case WM_CHANGEUISTATE:
        return "WM_CHANGEUISTATE";
    case WM_UPDATEUISTATE:
        return "WM_UPDATEUISTATE";
    case WM_QUERYUISTATE:
        return "WM_QUERYUISTATE";
    case WM_CTLCOLORMSGBOX:
        return "WM_CTLCOLORMSGBOX";
    case WM_CTLCOLOREDIT:
        return "WM_CTLCOLOREDIT";
    case WM_CTLCOLORLISTBOX:
        return "WM_CTLCOLORLISTBOX";
    case WM_CTLCOLORBTN:
        return "WM_CTLCOLORBTN";
    case WM_CTLCOLORDLG:
        return "WM_CTLCOLORDLG";
    case WM_CTLCOLORSCROLLBAR:
        return "WM_CTLCOLORSCROLLBAR";
    case WM_CTLCOLORSTATIC:
        return "WM_CTLCOLORSTATIC";
    case WM_MOUSEMOVE:
        return "WM_MOUSEMOVE";
    case WM_LBUTTONDOWN:
        return "WM_LBUTTONDOWN";
    case WM_LBUTTONUP:
        return "WM_LBUTTONUP";
    case WM_LBUTTONDBLCLK:
        return "WM_LBUTTONDBLCLK";
    case WM_RBUTTONDOWN:
        return "WM_RBUTTONDOWN";
    case WM_RBUTTONUP:
        return "WM_RBUTTONUP";
    case WM_RBUTTONDBLCLK:
        return "WM_RBUTTONDBLCLK";
    case WM_MBUTTONDOWN:
        return "WM_MBUTTONDOWN";
    case WM_MBUTTONUP:
        return "WM_MBUTTONUP";
    case WM_MBUTTONDBLCLK:
        return "WM_MBUTTONDBLCLK";
    case WM_MOUSEWHEEL:
        return "WM_MOUSEWHEEL";
    case WM_XBUTTONDOWN:
        return "WM_XBUTTONDOWN";
    case WM_XBUTTONUP:
        return "WM_XBUTTONUP";
    case WM_XBUTTONDBLCLK:
        return "WM_XBUTTONDBLCLK";
    case WM_MOUSEHWHEEL:
        return "WM_MOUSEHWHEEL";
    case WM_PARENTNOTIFY:
        return "WM_PARENTNOTIFY";
    case WM_ENTERMENULOOP:
        return "WM_ENTERMENULOOP";
    case WM_EXITMENULOOP:
        return "WM_EXITMENULOOP";
    case WM_NEXTMENU:
        return "WM_NEXTMENU";
    case WM_SIZING:
        return "WM_SIZING";
    case WM_CAPTURECHANGED:
        return "WM_CAPTURECHANGED";
    case WM_MOVING:
        return "WM_MOVING";
    case WM_POWERBROADCAST:
        return "WM_POWERBROADCAST";
    case WM_DEVICECHANGE:
        return "WM_DEVICECHANGE";
    case WM_MDICREATE:
        return "WM_MDICREATE";
    case WM_MDIDESTROY:
        return "WM_MDIDESTROY";
    case WM_MDIACTIVATE:
        return "WM_MDIACTIVATE";
    case WM_MDIRESTORE:
        return "WM_MDIRESTORE";
    case WM_MDINEXT:
        return "WM_MDINEXT";
    case WM_MDIMAXIMIZE:
        return "WM_MDIMAXIMIZE";
    case WM_MDITILE:
        return "WM_MDITILE";
    case WM_MDICASCADE:
        return "WM_MDICASCADE";
    case WM_MDIICONARRANGE:
        return "WM_MDIICONARRANGE";
    case WM_MDIGETACTIVE:
        return "WM_MDIGETACTIVE";
    case WM_MDISETMENU:
        return "WM_MDISETMENU";
    case WM_ENTERSIZEMOVE:
        return "WM_ENTERSIZEMOVE";
    case WM_EXITSIZEMOVE:
        return "WM_EXITSIZEMOVE";
    case WM_DROPFILES:
        return "WM_DROPFILES";
    case WM_MDIREFRESHMENU:
        return "WM_MDIREFRESHMENU";
    case WM_POINTERDEVICECHANGE:
        return "WM_POINTERDEVICECHANGE";
    case WM_POINTERDEVICEINRANGE:
        return "WM_POINTERDEVICEINRANGE";
    case WM_POINTERDEVICEOUTOFRANGE:
        return "WM_POINTERDEVICEOUTOFRANGE";
    case WM_TOUCH:
        return "WM_TOUCH";
    case WM_NCPOINTERUPDATE:
        return "WM_NCPOINTERUPDATE";
    case WM_NCPOINTERDOWN:
        return "WM_NCPOINTERDOWN";
    case WM_NCPOINTERUP:
        return "WM_NCPOINTERUP";
    case WM_POINTERUPDATE:
        return "WM_POINTERUPDATE";
    case WM_POINTERDOWN:
        return "WM_POINTERDOWN";
    case WM_POINTERUP:
        return "WM_POINTERUP";
    case WM_POINTERENTER:
        return "WM_POINTERENTER";
    case WM_POINTERLEAVE:
        return "WM_POINTERLEAVE";
    case WM_POINTERACTIVATE:
        return "WM_POINTERACTIVATE";
    case WM_POINTERCAPTURECHANGED:
        return "WM_POINTERCAPTURECHANGED";
    case WM_TOUCHHITTESTING:
        return "WM_TOUCHHITTESTING";
    case WM_POINTERWHEEL:
        return "WM_POINTERWHEEL";
    case WM_POINTERHWHEEL:
        return "WM_POINTERHWHEEL";
    case WM_POINTERROUTEDTO:
        return "WM_POINTERROUTEDTO";
    case WM_POINTERROUTEDAWAY:
        return "WM_POINTERROUTEDAWAY";
    case WM_POINTERROUTEDRELEASED:
        return "WM_POINTERROUTEDRELEASED";
    case WM_IME_SETCONTEXT:
        return "WM_IME_SETCONTEXT";
    case WM_IME_NOTIFY:
        return "WM_IME_NOTIFY";
    case WM_IME_CONTROL:
        return "WM_IME_CONTROL";
    case WM_IME_COMPOSITIONFULL:
        return "WM_IME_COMPOSITIONFULL";
    case WM_IME_SELECT:
        return "WM_IME_SELECT";
    case WM_IME_CHAR:
        return "WM_IME_CHAR";
    case WM_IME_REQUEST:
        return "WM_IME_REQUEST";
    case WM_IME_KEYDOWN:
        return "WM_IME_KEYDOWN";
    case WM_IME_KEYUP:
        return "WM_IME_KEYUP";
    case WM_MOUSEHOVER:
        return "WM_MOUSEHOVER";
    case WM_MOUSELEAVE:
        return "WM_MOUSELEAVE";
    case WM_NCMOUSEHOVER:
        return "WM_NCMOUSEHOVER";
    case WM_NCMOUSELEAVE:
        return "WM_NCMOUSELEAVE";
    case WM_WTSSESSION_CHANGE:
        return "WM_WTSSESSION_CHANGE";
    case WM_TABLET_FIRST:
        return "WM_TABLET_FIRST";
    case WM_TABLET_LAST:
        return "WM_TABLET_LAST";
    case WM_DPICHANGED:
        return "WM_DPICHANGED";
    case WM_DPICHANGED_BEFOREPARENT:
        return "WM_DPICHANGED_BEFOREPARENT";
    case WM_DPICHANGED_AFTERPARENT:
        return "WM_DPICHANGED_AFTERPARENT";
    case WM_GETDPISCALEDSIZE:
        return "WM_GETDPISCALEDSIZE";
    case WM_CUT:
        return "WM_CUT";
    case WM_COPY:
        return "WM_COPY";
    case WM_PASTE:
        return "WM_PASTE";
    case WM_CLEAR:
        return "WM_CLEAR";
    case WM_UNDO:
        return "WM_UNDO";
    case WM_RENDERFORMAT:
        return "WM_RENDERFORMAT";
    case WM_RENDERALLFORMATS:
        return "WM_RENDERALLFORMATS";
    case WM_DESTROYCLIPBOARD:
        return "WM_DESTROYCLIPBOARD";
    case WM_DRAWCLIPBOARD:
        return "WM_DRAWCLIPBOARD";
    case WM_PAINTCLIPBOARD:
        return "WM_PAINTCLIPBOARD";
    case WM_VSCROLLCLIPBOARD:
        return "WM_VSCROLLCLIPBOARD";
    case WM_SIZECLIPBOARD:
        return "WM_SIZECLIPBOARD";
    case WM_ASKCBFORMATNAME:
        return "WM_ASKCBFORMATNAME";
    case WM_CHANGECBCHAIN:
        return "WM_CHANGECBCHAIN";
    case WM_HSCROLLCLIPBOARD:
        return "WM_HSCROLLCLIPBOARD";
    case WM_QUERYNEWPALETTE:
        return "WM_QUERYNEWPALETTE";
    case WM_PALETTEISCHANGING:
        return "WM_PALETTEISCHANGING";
    case WM_PALETTECHANGED:
        return "WM_PALETTECHANGED";
    case WM_HOTKEY:
        return "WM_HOTKEY";
    case WM_PRINT:
        return "WM_PRINT";
    case WM_PRINTCLIENT:
        return "WM_PRINTCLIENT";
    case WM_APPCOMMAND:
        return "WM_APPCOMMAND";
    case WM_THEMECHANGED:
        return "WM_THEMECHANGED";
    case WM_CLIPBOARDUPDATE:
        return "WM_CLIPBOARDUPDATE";
    case WM_DWMCOMPOSITIONCHANGED:
        return "WM_DWMCOMPOSITIONCHANGED";
    case WM_DWMNCRENDERINGCHANGED:
        return "WM_DWMNCRENDERINGCHANGED";
    case WM_DWMCOLORIZATIONCOLORCHANGED:
        return "WM_DWMCOLORIZATIONCOLORCHANGED";
    case WM_DWMWINDOWMAXIMIZEDCHANGE:
        return "WM_DWMWINDOWMAXIMIZEDCHANGE";
    case WM_DWMSENDICONICTHUMBNAIL:
        return "WM_DWMSENDICONICTHUMBNAIL";
    case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
        return "WM_DWMSENDICONICLIVEPREVIEWBITMAP";
    case WM_GETTITLEBARINFOEX:
        return "WM_GETTITLEBARINFOEX";
    case WM_HANDHELDFIRST:
        return "WM_HANDHELDFIRST";
    case WM_HANDHELDLAST:
        return "WM_HANDHELDLAST";
    case WM_AFXFIRST:
        return "WM_AFXFIRST";
    case WM_AFXLAST:
        return "WM_AFXLAST";
    case WM_PENWINFIRST:
        return "WM_PENWINFIRST";
    case WM_PENWINLAST:
        return "WM_PENWINLAST";
    case WM_APP:
        return "WM_APP";
    case WM_USER:
        return "WM_USER";
    case WM_TOOLTIPDISMISS:
        return "WM_TOOLTIPDISMISS";
    default:
        return "<unknown>";
    }
}

tavros::system::keys tavros::system::map_key(UINT vk)
{
    switch (vk) {
    case VK_LBUTTON:
        return keys::k_lbutton;
    case VK_RBUTTON:
        return keys::k_rbutton;
    case VK_MBUTTON:
        return keys::k_mbutton;
    case VK_XBUTTON1:
        return keys::k_xbutton1;
    case VK_XBUTTON2:
        return keys::k_xbutton2;
    case VK_BACK:
        return keys::k_backspace;
    case VK_TAB:
        return keys::k_tab;
    case VK_RETURN:
        return keys::k_enter;
    case VK_PAUSE:
        return keys::k_pause;
    case VK_CAPITAL:
        return keys::k_capslock;
    case VK_ESCAPE:
        return keys::k_escape;
    case VK_SPACE:
        return keys::k_space;
    case VK_PRIOR:
        return keys::k_pageup;
    case VK_NEXT:
        return keys::k_pagedown;
    case VK_END:
        return keys::k_end;
    case VK_HOME:
        return keys::k_home;
    case VK_LEFT:
        return keys::k_left;
    case VK_UP:
        return keys::k_up;
    case VK_RIGHT:
        return keys::k_right;
    case VK_DOWN:
        return keys::k_down;
    case VK_PRINT:
        return keys::k_print;
    case VK_SNAPSHOT:
        return keys::k_print_screen;
    case VK_INSERT:
        return keys::k_insert;
    case VK_DELETE:
        return keys::k_delete;
    case VK_HELP:
        return keys::k_help;
    case '0':
        return keys::k_0;
    case '1':
        return keys::k_1;
    case '2':
        return keys::k_2;
    case '3':
        return keys::k_3;
    case '4':
        return keys::k_4;
    case '5':
        return keys::k_5;
    case '6':
        return keys::k_6;
    case '7':
        return keys::k_7;
    case '8':
        return keys::k_8;
    case '9':
        return keys::k_9;
    case 'A':
        return keys::k_A;
    case 'B':
        return keys::k_B;
    case 'C':
        return keys::k_C;
    case 'D':
        return keys::k_D;
    case 'E':
        return keys::k_E;
    case 'F':
        return keys::k_F;
    case 'G':
        return keys::k_G;
    case 'H':
        return keys::k_H;
    case 'I':
        return keys::k_I;
    case 'J':
        return keys::k_J;
    case 'K':
        return keys::k_K;
    case 'L':
        return keys::k_L;
    case 'M':
        return keys::k_M;
    case 'N':
        return keys::k_N;
    case 'O':
        return keys::k_O;
    case 'P':
        return keys::k_P;
    case 'Q':
        return keys::k_Q;
    case 'R':
        return keys::k_R;
    case 'S':
        return keys::k_S;
    case 'T':
        return keys::k_T;
    case 'U':
        return keys::k_U;
    case 'V':
        return keys::k_V;
    case 'W':
        return keys::k_W;
    case 'X':
        return keys::k_X;
    case 'Y':
        return keys::k_Y;
    case 'Z':
        return keys::k_Z;
    case VK_LWIN:
        return keys::k_lsuper;
    case VK_RWIN:
        return keys::k_rsuper;
    case VK_APPS:
        return keys::k_menu;
    case VK_NUMPAD0:
        return keys::k_numpad0;
    case VK_NUMPAD1:
        return keys::k_numpad1;
    case VK_NUMPAD2:
        return keys::k_numpad2;
    case VK_NUMPAD3:
        return keys::k_numpad3;
    case VK_NUMPAD4:
        return keys::k_numpad4;
    case VK_NUMPAD5:
        return keys::k_numpad5;
    case VK_NUMPAD6:
        return keys::k_numpad6;
    case VK_NUMPAD7:
        return keys::k_numpad7;
    case VK_NUMPAD8:
        return keys::k_numpad8;
    case VK_NUMPAD9:
        return keys::k_numpad9;
    case VK_MULTIPLY:
        return keys::k_multiply;
    case VK_ADD:
        return keys::k_add;
    case VK_SUBTRACT:
        return keys::k_subtract;
    case VK_DECIMAL:
        return keys::k_decimal;
    case VK_DIVIDE:
        return keys::k_divide;
    case VK_F1:
        return keys::k_F1;
    case VK_F2:
        return keys::k_F2;
    case VK_F3:
        return keys::k_F3;
    case VK_F4:
        return keys::k_F4;
    case VK_F5:
        return keys::k_F5;
    case VK_F6:
        return keys::k_F6;
    case VK_F7:
        return keys::k_F7;
    case VK_F8:
        return keys::k_F8;
    case VK_F9:
        return keys::k_F9;
    case VK_F10:
        return keys::k_F10;
    case VK_F11:
        return keys::k_F11;
    case VK_F12:
        return keys::k_F12;
    case VK_F13:
        return keys::k_F13;
    case VK_F14:
        return keys::k_F14;
    case VK_F15:
        return keys::k_F15;
    case VK_F16:
        return keys::k_F16;
    case VK_F17:
        return keys::k_F17;
    case VK_F18:
        return keys::k_F18;
    case VK_F19:
        return keys::k_F19;
    case VK_F20:
        return keys::k_F20;
    case VK_OEM_1:
        return keys::k_semicolon;
    case VK_OEM_PLUS:
        return keys::k_equal;
    case VK_OEM_COMMA:
        return keys::k_comma;
    case VK_OEM_MINUS:
        return keys::k_minus;
    case VK_OEM_PERIOD:
        return keys::k_period;
    case VK_OEM_2:
        return keys::k_slash;
    case VK_OEM_3:
        return keys::k_grave_accent;
    case VK_OEM_4:
        return keys::k_lbracket;
    case VK_OEM_5:
        return keys::k_backslash;
    case VK_OEM_6:
        return keys::k_rbracket;
    case VK_OEM_7:
        return keys::k_apostrophe;
    case VK_NUMLOCK:
        return keys::k_numlock;
    case VK_SCROLL:
        return keys::k_scroll;
    case VK_LSHIFT:
        return keys::k_lshift;
    case VK_RSHIFT:
        return keys::k_rshift;
    case VK_LCONTROL:
        return keys::k_lcontrol;
    case VK_RCONTROL:
        return keys::k_rcontrol;
    case VK_LMENU:
        return keys::k_lalt;
    case VK_RMENU:
        return keys::k_ralt;
    default:
        return keys::none;
    }
}

const char* tavros::system::last_win_error_str()
{
    static char str[1024];

    if (DWORD e = GetLastError(); e != 0) {
        auto result = FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            e,
            0, // Lang ID
            str,
            sizeof(str),
            NULL
        );

        if (result) {
            // Trim the end of the line from spaces
            str[result] = 0;
            for (int32_t i = result - 1; i >= 0; i--) {
                if (str[i] == ' ' || str[i] == '\n' || str[i] == '\r') {
                    str[i] = 0;
                } else {
                    break;
                }
            }
        } else {
            strcpy_s(str, sizeof(str), "<Error generating message>");
        }
    } else {
        strcpy_s(str, sizeof(str), "<No error>");
    }

    SetLastError(0);

    return str;
}

tavros::math::ipoint2 tavros::system::create_point2(LPARAM lParam)
{
    int32_t x = GET_X_LPARAM(lParam);
    int32_t y = GET_Y_LPARAM(lParam);
    return tavros::math::ipoint2(x, y);
}

tavros::math::point2 create_fpoint2(LPARAM lParam)
{
    float x = static_cast<float>(GET_X_LPARAM(lParam));
    float y = static_cast<float>(GET_Y_LPARAM(lParam));
    return tavros::math::point2(x, y);
}

tavros::math::isize2 tavros::system::create_size2(LPARAM lParam)
{
    int32_t width = GET_X_LPARAM(lParam);
    int32_t height = GET_Y_LPARAM(lParam);
    return tavros::math::isize2(width, height);
}

mouse_event_args tavros::system::create_mouse_event_args(LPARAM lParam, mouse_button btn, bool double_click, int32 delta)
{
    return {.button = btn, .is_double_click = double_click, .is_relative_move = false, .delta = tavros::math::vec2(0.0f, static_cast<float>(delta)), .pos = create_fpoint2(lParam)};
}

mouse_button tavros::system::create_mouse_x_button(WPARAM wParam)
{
    switch (UINT xbutton = GET_XBUTTON_WPARAM(wParam); xbutton) {
    case XBUTTON1:
        return mouse_button::x_button1;
    case XBUTTON2:
        return mouse_button::x_button2;
    default:
        return mouse_button::none;
    }
}

key_event_args tavros::system::create_key_event_args(WPARAM wParam, LPARAM lParam)
{
    WORD vkCode = LOWORD(wParam);
    WORD keyFlags = HIWORD(lParam);
    BOOL wasKeyDown = (keyFlags & KF_REPEAT) == KF_REPEAT;
    WORD repeatCount = LOWORD(lParam);

    if (vkCode == VK_SHIFT || vkCode == VK_CONTROL || vkCode == VK_MENU) {
        WORD scanCode = LOBYTE(keyFlags);
        BOOL isExtendedKey = (keyFlags & KF_EXTENDED) == KF_EXTENDED;
        if (isExtendedKey) {
            scanCode = MAKEWORD(scanCode, 0xE0);
        }
        vkCode = LOWORD(MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX));
    }

    return {
        .key = map_key(vkCode),
        .is_prev_pressed = static_cast<bool>(wasKeyDown),
        .repeats = repeatCount,
        .key_char = 0
    };
}

key_event_args tavros::system::create_char_event_args(WPARAM wParam, LPARAM lParam)
{
    WORD keyFlags = HIWORD(lParam);
    BOOL wasKeyDown = (keyFlags & KF_REPEAT) == KF_REPEAT;
    WORD repeatCount = LOWORD(lParam);

    return {
        .key = keys::none,
        .is_prev_pressed = static_cast<bool>(wasKeyDown),
        .repeats = repeatCount,
        .key_char = static_cast<char>(wParam),
    };
}
