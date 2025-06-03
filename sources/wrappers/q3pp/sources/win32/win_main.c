/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// win_main.c

#include "../client/client.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

#pragma comment(lib, "wsock32")
#pragma comment(lib, "winmm")

#include <tavros/system/platform/win32/window.hpp>
#include "opengl_swapchain.hpp"
#include <tavros/core/timer.hpp>

#include <assert.h>
#include "../q3_renderer/tr_local.h"
#include "../client/keys.h"

#include <memory>

static tavros::core::logger logger("win_main");

static char sys_cmdline[MAX_STRING_CHARS];

WinVars_t g_wv;


int32 to_qkey(tavros::system::keys key)
{
    switch (key) {
    case tavros::system::keys::k_lbutton:
        return K_MOUSE1;
    case tavros::system::keys::k_rbutton:
        return K_MOUSE2;
    case tavros::system::keys::k_mbutton:
        return K_MOUSE3;
    case tavros::system::keys::k_xbutton1:
        return K_MOUSE4;
    case tavros::system::keys::k_xbutton2:
        return K_MOUSE5;
    case tavros::system::keys::k_backspace:
        return K_BACKSPACE;
    case tavros::system::keys::k_tab:
        return K_TAB;
    case tavros::system::keys::k_enter:
        return K_ENTER;
    case tavros::system::keys::k_pause:
        return K_PAUSE;
    case tavros::system::keys::k_capslock:
        return K_CAPSLOCK;
    case tavros::system::keys::k_escape:
        return K_ESCAPE;
    case tavros::system::keys::k_space:
        return K_SPACE;
    case tavros::system::keys::k_pageup:
        return K_PGUP;
    case tavros::system::keys::k_pagedown:
        return K_PGDN;
    case tavros::system::keys::k_end:
        return K_END;
    case tavros::system::keys::k_home:
        return K_HOME;
    case tavros::system::keys::k_left:
        return K_LEFTARROW;
    case tavros::system::keys::k_up:
        return K_UPARROW;
    case tavros::system::keys::k_right:
        return K_RIGHTARROW;
    case tavros::system::keys::k_down:
        return K_DOWNARROW;
    case tavros::system::keys::k_insert:
        return K_INS;
    case tavros::system::keys::k_delete:
        return K_DEL;
    case tavros::system::keys::k_0:
        return '0';
    case tavros::system::keys::k_1:
        return '1';
    case tavros::system::keys::k_2:
        return '2';
    case tavros::system::keys::k_3:
        return '3';
    case tavros::system::keys::k_4:
        return '4';
    case tavros::system::keys::k_5:
        return '5';
    case tavros::system::keys::k_6:
        return '6';
    case tavros::system::keys::k_7:
        return '7';
    case tavros::system::keys::k_8:
        return '8';
    case tavros::system::keys::k_9:
        return '9';
    case tavros::system::keys::k_A:
        return 'a';
    case tavros::system::keys::k_B:
        return 'b';
    case tavros::system::keys::k_C:
        return 'c';
    case tavros::system::keys::k_D:
        return 'd';
    case tavros::system::keys::k_E:
        return 'e';
    case tavros::system::keys::k_F:
        return 'f';
    case tavros::system::keys::k_G:
        return 'g';
    case tavros::system::keys::k_H:
        return 'h';
    case tavros::system::keys::k_I:
        return 'i';
    case tavros::system::keys::k_J:
        return 'j';
    case tavros::system::keys::k_K:
        return 'k';
    case tavros::system::keys::k_L:
        return 'l';
    case tavros::system::keys::k_M:
        return 'm';
    case tavros::system::keys::k_N:
        return 'n';
    case tavros::system::keys::k_O:
        return 'o';
    case tavros::system::keys::k_P:
        return 'p';
    case tavros::system::keys::k_Q:
        return 'q';
    case tavros::system::keys::k_R:
        return 'r';
    case tavros::system::keys::k_S:
        return 's';
    case tavros::system::keys::k_T:
        return 't';
    case tavros::system::keys::k_U:
        return 'u';
    case tavros::system::keys::k_V:
        return 'v';
    case tavros::system::keys::k_W:
        return 'w';
    case tavros::system::keys::k_X:
        return 'x';
    case tavros::system::keys::k_Y:
        return 'y';
    case tavros::system::keys::k_Z:
        return 'z';
    case tavros::system::keys::k_lsuper:
        return K_COMMAND;
    case tavros::system::keys::k_rsuper:
        return K_COMMAND;
    case tavros::system::keys::k_multiply:
        return '*';
    case tavros::system::keys::k_add:
        return '+';
    case tavros::system::keys::k_subtract:
        return '-';
    case tavros::system::keys::k_divide:
        return '/';
    case tavros::system::keys::k_F1:
        return K_F1;
    case tavros::system::keys::k_F2:
        return K_F2;
    case tavros::system::keys::k_F3:
        return K_F3;
    case tavros::system::keys::k_F4:
        return K_F4;
    case tavros::system::keys::k_F5:
        return K_F5;
    case tavros::system::keys::k_F6:
        return K_F6;
    case tavros::system::keys::k_F7:
        return K_F7;
    case tavros::system::keys::k_F8:
        return K_F8;
    case tavros::system::keys::k_F9:
        return K_F9;
    case tavros::system::keys::k_F10:
        return K_F10;
    case tavros::system::keys::k_F11:
        return K_F11;
    case tavros::system::keys::k_F12:
        return K_F12;
    case tavros::system::keys::k_F13:
        return K_F13;
    case tavros::system::keys::k_F14:
        return K_F14;
    case tavros::system::keys::k_F15:
        return K_F15;
    case tavros::system::keys::k_semicolon:
        return ';';
    case tavros::system::keys::k_equal:
        return K_KP_PLUS;
    case tavros::system::keys::k_comma:
        return ',';
    case tavros::system::keys::k_minus:
        return K_KP_MINUS;
    case tavros::system::keys::k_period:
        return '.';
    case tavros::system::keys::k_slash:
        return '/';
    case tavros::system::keys::k_grave_accent:
        return '`';
    case tavros::system::keys::k_lbracket:
        return '[';
    case tavros::system::keys::k_backslash:
        return '\\';
    case tavros::system::keys::k_rbracket:
        return ']';
    case tavros::system::keys::k_apostrophe:
        return '\'';
    case tavros::system::keys::k_numlock:
        return K_KP_NUMLOCK;
    case tavros::system::keys::k_lshift:
        return K_SHIFT;
    case tavros::system::keys::k_rshift:
        return K_SHIFT;
    case tavros::system::keys::k_lcontrol:
        return K_CTRL;
    case tavros::system::keys::k_rcontrol:
        return K_CTRL;
    case tavros::system::keys::k_lalt:
        return K_ALT;
    case tavros::system::keys::k_ralt:
        return K_ALT;
    default:
        return 0;
    }
}

int32 to_qkey(tavros::system::mouse_button btn)
{
    switch (btn) {
    case tavros::system::mouse_button::left:
        return K_MOUSE1;
    case tavros::system::mouse_button::right:
        return K_MOUSE2;
    case tavros::system::mouse_button::middle:
        return K_MOUSE3;
    case tavros::system::mouse_button::x_button1:
        return K_MWHEELUP;
    case tavros::system::mouse_button::x_button2:
        return K_MWHEELDOWN;
    default:
        return 0;
    }
}

class main_window
{
public:
    main_window(const char* title, int w, int h)
    {
        m_wnd = tavros::system::interfaces::window::create(title);

        g_wv.hWnd = (HWND) m_wnd->get_handle();
        m_swapchain = std::make_unique<opengl_swapchain>(m_wnd->get_handle());
        if (!m_swapchain->init()) {
            ::logger.error("Failed to initialize swapchain.");
        }
        m_swapchain->activate();

        if (!OpenGL_Init()) {
            ::logger.error("Failed to init OpenGL.");
        }

        m_wnd->set_on_activate_listener([this](tavros::system::window_ptr wnd) {
            g_wv.isMinimized = wnd->get_window_state() == tavros::system::window_state::minimized;
            g_wv.activeApp = true;
            Key_ClearStates();

            move_cursor_to_center();

            SetCapture(g_wv.hWnd);
            while (ShowCursor(FALSE) >= 0)
                ;

            SNDDMA_Activate();
        });

        m_wnd->set_on_deactivate_listener([](tavros::system::window_ptr wnd) {
            g_wv.isMinimized = wnd->get_window_state() == tavros::system::window_state::minimized;
            g_wv.activeApp = false;
            Key_ClearStates();

            ClipCursor(NULL);
            ReleaseCapture();
            while (ShowCursor(TRUE) < 0)
                ;

            SNDDMA_Activate();
        });

        m_wnd->set_on_mouse_wheel_listener([](tavros::system::window_ptr wnd, tavros::system::mouse_event_args& e) {
            int32 delta = static_cast<int32>(e.delta.y >= 0.0f ? e.delta.y : -e.delta.y);
            int32 event = static_cast<int32>(e.delta.y > 0 ? keyNum_t::K_MWHEELDOWN : keyNum_t::K_MWHEELUP);
            int32 times = delta / 120;
            for (auto i = 0; i < times; i++) {
                Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, event, true, 0, NULL);
                Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, event, false, 0, NULL);
            }
        });

        m_wnd->set_on_mouse_down_listener([](tavros::system::window_ptr wnd, tavros::system::mouse_event_args& e) {
            Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, to_qkey(e.button), true, 0, NULL);
        });

        m_wnd->set_on_mouse_up_listener([](tavros::system::window_ptr wnd, tavros::system::mouse_event_args& e) {
            Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, to_qkey(e.button), false, 0, NULL);
        });

        m_wnd->set_on_mouse_move_listener([this](tavros::system::window_ptr wnd, tavros::system::mouse_event_args& e) {
            if (e.is_relative_move) {
                Sys_QueEvent(0, SE_MOUSE, e.pos.x, e.pos.y, 0, NULL);
                move_cursor_to_center();
            }
        });

        m_wnd->set_on_key_down_listener([](tavros::system::window_ptr wnd, tavros::system::key_event_args& e) {
            Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, to_qkey(e.key), true, 0, NULL);
        });

        m_wnd->set_on_key_up_listener([](tavros::system::window_ptr wnd, tavros::system::key_event_args& e) {
            Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, to_qkey(e.key), false, 0, NULL);
        });

        m_wnd->set_on_key_press_listener([](tavros::system::window_ptr wnd, tavros::system::key_event_args& e) {
            Sys_QueEvent(g_wv.sysMsgTime, SE_CHAR, e.key_char, 0, 0, NULL);
        });

        m_wnd->set_on_close_listener([](tavros::system::window_ptr wnd, tavros::system::close_event_args& e) {
            Cbuf_ExecuteText(EXEC_APPEND, "quit");
            PostQuitMessage(0);
        });

        m_wnd->set_client_size(w, h);
        center();

        m_wnd->show();
    }

    ~main_window()
    {
        OpenGL_Shutdown();
        m_swapchain = nullptr;
    }

    void move_cursor_to_center()
    {
        auto cursor_pos = m_wnd->get_location() + m_wnd->get_client_size() / 2;
        SetCursorPos(cursor_pos.x, cursor_pos.y);
    }

    void center()
    {
        auto  size = m_wnd->get_client_size();
        int32 desktop_width = GetSystemMetrics(SM_CXSCREEN);
        int32 desktop_height = GetSystemMetrics(SM_CYSCREEN);
        int32 new_x = (desktop_width - size.width) / 2;
        int32 new_y = (desktop_height - size.height - 40) / 2;
        m_wnd->set_location(new_x, new_y);
    }

    void present()
    {
        m_swapchain->present();
    }

private:
    tavros::system::window_uptr                       m_wnd;
    std::unique_ptr<opengl_swapchain> m_swapchain;
};

std::unique_ptr<main_window> s_main_window;

/* GLimp_EndFrame */
void GLimp_EndFrame()
{
    s_main_window->present();
}

void GLimp_Init()
{
    logger.info("Initializing OpenGL subsystem");

    int32 width = 1366;
    int32 height = 768;

    if (!s_main_window) {
        glConfig.vidWidth = width;
        glConfig.vidHeight = height;
        s_main_window = std::make_unique<main_window>("Quake 3: Arena", width, height + 40);
    }
}

/* GLimp_Shutdown
 *
 * This routine does all OS specific shutdown procedures for the OpenGL subsystem.
 */
void GLimp_Shutdown()
{
    logger.info("Shutting down OpenGL subsystem");

    s_main_window = nullptr;

    memset(&glConfig, 0, sizeof(glConfig));
    memset(&glState, 0, sizeof(glState));
}


/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void QDECL Sys_Error(const char* error, ...)
{
    va_list argptr;
    char    text[4096];
    MSG     msg;

    va_start(argptr, error);
    vsprintf(text, error, argptr);
    va_end(argptr);

    // wait for the user to quit
    while (1) {
        if (!GetMessage(&msg, NULL, 0, 0)) {
            Com_Quit_f();
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    exit(1);
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit()
{
    PostQuitMessage(0);

    exit(0);
}

/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir(const char* path)
{
    _mkdir(path);
}

/*
==============
Sys_Cwd
==============
*/
const char* Sys_Cwd()
{
    static char cwd[MAX_OSPATH];

    _getcwd(cwd, sizeof(cwd) - 1);
    cwd[MAX_OSPATH - 1] = 0;

    return cwd;
}


/*
================
Sys_Milliseconds
================
*/
int32 sys_timeBase;
int32 Sys_Milliseconds()
{
    int32       sys_curtime;
    static bool initialized = false;

    if (!initialized) {
        sys_timeBase = timeGetTime();
        initialized = true;
    }
    sys_curtime = timeGetTime() - sys_timeBase;

    return sys_curtime;
}

//============================================

char* Sys_GetCurrentUser()
{
    static char s_userName[1024];
    uint32      size = sizeof(s_userName);


    if (!GetUserName(s_userName, (LPDWORD) &size)) {
        strcpy(s_userName, "player");
    }

    if (!s_userName[0]) {
        strcpy(s_userName, "player");
    }

    return s_userName;
}

const char* Sys_DefaultHomePath()
{
    return NULL;
}

const char* Sys_DefaultInstallPath()
{
    static char s[MAX_OSPATH];
    //char*       end = _getcwd(s, sizeof(s) - 1);
    strcpy(s, "D:\\Work\\q3pp_res\\");
    s[MAX_OSPATH - 1] = 0;

    return s;
    // return Sys_Cwd();
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define MAX_FOUND_FILES 0x1000

void Sys_ListFilteredFiles(const char* basedir, const char* subdirs, const char* filter, char** list, int32* numfiles)
{
    char               search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
    char               filename[MAX_OSPATH];
    intptr_t              findhandle;
    struct _finddata_t findinfo;

    if (*numfiles >= MAX_FOUND_FILES - 1) {
        return;
    }

    if (strlen(subdirs)) {
        Com_sprintf(search, sizeof(search), "%s\\%s\\*", basedir, subdirs);
    } else {
        Com_sprintf(search, sizeof(search), "%s\\*", basedir);
    }

    findhandle = _findfirst(search, &findinfo);
    if (findhandle == -1) {
        return;
    }

    do {
        if (findinfo.attrib & _A_SUBDIR) {
            if (Q_stricmp(findinfo.name, ".") && Q_stricmp(findinfo.name, "..")) {
                if (strlen(subdirs)) {
                    Com_sprintf(newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, findinfo.name);
                } else {
                    Com_sprintf(newsubdirs, sizeof(newsubdirs), "%s", findinfo.name);
                }
                Sys_ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
            }
        }
        if (*numfiles >= MAX_FOUND_FILES - 1) {
            break;
        }
        Com_sprintf(filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name);
        if (!Com_FilterPath(filter, filename, false)) {
            continue;
        }
        list[*numfiles] = CopyString(filename);
        (*numfiles)++;
    } while (_findnext(findhandle, &findinfo) != -1);

    _findclose(findhandle);
}

static bool strgtr(const char* s0, const char* s1)
{
    int32 l0, l1, i;

    l0 = strlen(s0);
    l1 = strlen(s1);

    if (l1 < l0) {
        l0 = l1;
    }

    for (i = 0; i < l0; i++) {
        if (s1[i] > s0[i]) {
            return true;
        }
        if (s1[i] < s0[i]) {
            return false;
        }
    }
    return false;
}

const char** Sys_ListFiles(const char* directory, const char* extension, const char* filter, int32* numfiles, bool wantsubs)
{
    char               search[MAX_OSPATH];
    int32              nfiles;
    const char**       listCopy;
    char*              list[MAX_FOUND_FILES];
    struct _finddata_t findinfo;
    intptr_t              findhandle;
    int32              flag;
    int32              i;

    if (filter) {
        nfiles = 0;
        Sys_ListFilteredFiles(directory, "", filter, list, &nfiles);

        list[nfiles] = 0;
        *numfiles = nfiles;

        if (!nfiles) {
            return NULL;
        }

        listCopy = (const char**) Z_Malloc((nfiles + 1) * sizeof(*listCopy));
        for (i = 0; i < nfiles; i++) {
            listCopy[i] = list[i];
        }
        listCopy[i] = NULL;

        return listCopy;
    }

    if (!extension) {
        extension = "";
    }

    // passing a slash as extension will find directories
    if (extension[0] == '/' && extension[1] == 0) {
        extension = "";
        flag = 0;
    } else {
        flag = _A_SUBDIR;
    }

    Com_sprintf(search, sizeof(search), "%s\\*%s", directory, extension);

    // search
    nfiles = 0;

    findhandle = _findfirst(search, &findinfo);
    if (findhandle == -1) {
        *numfiles = 0;
        return NULL;
    }

    do {
        if ((!wantsubs && flag ^ (findinfo.attrib & _A_SUBDIR)) || (wantsubs && findinfo.attrib & _A_SUBDIR)) {
            if (nfiles == MAX_FOUND_FILES - 1) {
                break;
            }
            list[nfiles] = CopyString(findinfo.name);
            nfiles++;
        }
    } while (_findnext(findhandle, &findinfo) != -1);

    list[nfiles] = 0;

    _findclose(findhandle);

    // return a copy of the list
    *numfiles = nfiles;

    if (!nfiles) {
        return NULL;
    }

    listCopy = (const char**) Z_Malloc((nfiles + 1) * sizeof(*listCopy));
    for (i = 0; i < nfiles; i++) {
        listCopy[i] = list[i];
    }
    listCopy[i] = NULL;

    do {
        flag = 0;
        for (i = 1; i < nfiles; i++) {
            if (strgtr(listCopy[i - 1], listCopy[i])) {
                const char* temp = listCopy[i];
                listCopy[i] = listCopy[i - 1];
                listCopy[i - 1] = temp;
                flag = 1;
            }
        }
    } while (flag);

    return listCopy;
}

void Sys_FreeFileList(char** list)
{
    int32 i;

    if (!list) {
        return;
    }

    for (i = 0; list[i]; i++) {
        Z_Free(list[i]);
    }

    Z_Free(list);
}

//========================================================

/*
================
Sys_GetClipboardData
================
*/
char* Sys_GetClipboardData()
{
    char* data = NULL;
    char* cliptext;

    if (OpenClipboard(NULL) != 0) {
        HANDLE hClipboardData;

        if ((hClipboardData = GetClipboardData(CF_TEXT)) != 0) {
            if ((cliptext = (char*) GlobalLock(hClipboardData)) != 0) {
                data = (char*) Z_Malloc(GlobalSize(hClipboardData) + 1);
                Q_strncpyz(data, cliptext, GlobalSize(hClipboardData));
                GlobalUnlock(hClipboardData);

                strtok(data, "\n\r\b");
            }
        }
        CloseClipboard();
    }
    return data;
}

/*
========================================================================
EVENT LOOP
========================================================================
*/

#define MAX_QUED_EVENTS  256
#define MASK_QUED_EVENTS (MAX_QUED_EVENTS - 1)

sysEvent_t eventQue[MAX_QUED_EVENTS];
int32      eventHead, eventTail;
uint8      sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent(int32 time, sysEventType_t type, int32 value, int32 value2, int32 ptrLength, void* ptr)
{
    sysEvent_t* ev;

    ev = &eventQue[eventHead & MASK_QUED_EVENTS];
    if (eventHead - eventTail >= MAX_QUED_EVENTS) {
        logger.warning("Sys_QueEvent: overflow");
        // we are discarding an event, but don't leak memory
        if (ev->evPtr) {
            Z_Free(ev->evPtr);
        }
        eventTail++;
    }

    eventHead++;

    if (time == 0) {
        time = Sys_Milliseconds();
    }

    ev->evTime = time;
    ev->evType = type;
    ev->evValue = value;
    ev->evValue2 = value2;
    ev->evPtrLength = ptrLength;
    ev->evPtr = ptr;
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent()
{
    MSG        msg;
    sysEvent_t ev;
    char*      s;
    msg_t      netmsg;
    netadr_t   adr;

    // return if we have data
    if (eventHead > eventTail) {
        eventTail++;
        return eventQue[(eventTail - 1) & MASK_QUED_EVENTS];
    }

    // pump the message loop
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            Com_Quit_f();
        }

        // save the msg time, because wndprocs don't have access to the timestamp
        g_wv.sysMsgTime = msg.time;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // check for console commands
    // TODO: console commands add here like below
    /*s = Sys_ConsoleInput();
    if (s) {
        char*   b;
        int32 len;

        len = strlen(s) + 1;
        b = (char*) Z_Malloc(len);
        Q_strncpyz(b, s, len - 1);
        Sys_QueEvent(0, SE_CONSOLE, 0, 0, len, b);
    }*/

    // check for network packets
    MSG_Init(&netmsg, sys_packetReceived, sizeof(sys_packetReceived));
    if (Sys_GetPacket(&adr, &netmsg)) {
        netadr_t* buf;
        int32     len;

        // copy out to a seperate buffer for qeueing
        // the readcount stepahead is for SOCKS support
        len = sizeof(netadr_t) + netmsg.cursize - netmsg.readcount;
        buf = (netadr_t*) Z_Malloc(len);
        *buf = adr;
        memcpy(buf + 1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount);
        Sys_QueEvent(0, SE_PACKET, 0, 0, len, buf);
    }

    // return if we have data
    if (eventHead > eventTail) {
        eventTail++;
        return eventQue[(eventTail - 1) & MASK_QUED_EVENTS];
    }

    // create an empty event to return

    memset(&ev, 0, sizeof(ev));
    ev.evTime = timeGetTime();

    return ev;
}

//================================================================

/*
=================
Sys_Net_Restart_f

Restart the network subsystem
=================
*/
void Sys_Net_Restart_f()
{
    NET_Restart();
}

/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
void Sys_Init()
{
    Cmd_AddCommand("net_restart", Sys_Net_Restart_f);
    Cvar_Set("username", Sys_GetCurrentUser());
}


//=======================================================================


FILE* log_f = nullptr;


/* WinMain */
int32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32 nCmdShow)
{
    char cwd[MAX_OSPATH];

    // should never get a previous instance in Win32
    if (hPrevInstance) {
        return 0;
    }

    log_f = fopen("qlog.log", "w");
    tavros::core::logger::add_consumer([](tavros::core::severity_level, tavros::core::string_view tag, tavros::core::string_view msg) {
        fprintf(log_f, "%s\n", msg.data());
        fflush(log_f);
    });

#if 0
    auto win = tavros::system::interfaces::window::create("Quake 3: Arena");
    win->set_on_close_listener([](tavros::system::window_ptr, tavros::system::close_event_args& e){
        PostQuitMessage(0);
    });
    win->set_on_key_down_listener([](tavros::system::window_ptr, tavros::system::key_event_args& e) {
        if (e.key == tavros::system::keys::k_Q) {
            exit(0);
        }
    });

    win->set_on_resize_listener([](tavros::system::window_ptr control, tavros::system::size_event_args& e) {
        
    });

    win->set_on_drop_listener([](tavros::system::window_ptr control, tavros::system::drop_event_args& e) {
        for (auto i = 0; e.files[i] != nullptr; ++i) {
            logger.debug("Drop file: %s", e.files[i]);
        }
    });

    win->show();

    MSG        msg;
    sysEvent_t ev;
    bool run = true;

    while (run) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                run = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Sleep(1);
    }

    CoUninitialize();

    return 0;
#endif


    Q_strncpyz(sys_cmdline, lpCmdLine, sizeof(sys_cmdline));

    SetErrorMode(SEM_FAILCRITICALERRORS); // no abort/retry/fail errors

    // get the initial time base
    Sys_Milliseconds();

    Com_Init(sys_cmdline);
    NET_Init();

    _getcwd(cwd, sizeof(cwd));
    logger.info("Working directory: %s", cwd);

    tavros::core::timer tm;
    // main game loop
    while (1) {
        // if not running as a game client, sleep a bit
        if (g_wv.isMinimized || (com_dedicated && com_dedicated->integer)) {
            Sleep(5);
        }

        // run the game
        Com_Frame();
    }

    // never gets here
    return 0;
}


