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
#include "client.h"

static tavros::core::logger logger("cl_keys");

/*

key up events are sent even if in console mode

*/

field_t historyEditLines[COMMAND_HISTORY];

int32 nextHistoryLine; // the last line in the history buffer, not masked
int32 historyLine;     // the line being displayed from history buffer
                       // will be <= nextHistoryLine

field_t g_consoleField;
field_t chatField;
bool    chat_team;

int32 chat_playerNum;


bool key_overstrikeMode;

bool   anykeydown;
qkey_t keys[MAX_KEYS];


typedef struct
{
    const char* name;
    int32       keynum;
} keyname_t;


// names not in this list can either be lowercase ascii, or '0xnn' hex sequences
keyname_t keynames[] =
    {
        {"TAB", K_TAB},
        {"ENTER", K_ENTER},
        {"ESCAPE", K_ESCAPE},
        {"SPACE", K_SPACE},
        {"BACKSPACE", K_BACKSPACE},
        {"UPARROW", K_UPARROW},
        {"DOWNARROW", K_DOWNARROW},
        {"LEFTARROW", K_LEFTARROW},
        {"RIGHTARROW", K_RIGHTARROW},

        {"ALT", K_ALT},
        {"CTRL", K_CTRL},
        {"SHIFT", K_SHIFT},

        {"COMMAND", K_COMMAND},

        {"CAPSLOCK", K_CAPSLOCK},


        {"F1", K_F1},
        {"F2", K_F2},
        {"F3", K_F3},
        {"F4", K_F4},
        {"F5", K_F5},
        {"F6", K_F6},
        {"F7", K_F7},
        {"F8", K_F8},
        {"F9", K_F9},
        {"F10", K_F10},
        {"F11", K_F11},
        {"F12", K_F12},

        {"INS", K_INS},
        {"DEL", K_DEL},
        {"PGDN", K_PGDN},
        {"PGUP", K_PGUP},
        {"HOME", K_HOME},
        {"END", K_END},

        {"MOUSE1", K_MOUSE1},
        {"MOUSE2", K_MOUSE2},
        {"MOUSE3", K_MOUSE3},
        {"MOUSE4", K_MOUSE4},
        {"MOUSE5", K_MOUSE5},

        {"MWHEELUP", K_MWHEELUP},
        {"MWHEELDOWN", K_MWHEELDOWN},

        {"AUX1", K_AUX1},
        {"AUX2", K_AUX2},
        {"AUX3", K_AUX3},
        {"AUX4", K_AUX4},
        {"AUX5", K_AUX5},
        {"AUX6", K_AUX6},
        {"AUX7", K_AUX7},
        {"AUX8", K_AUX8},
        {"AUX9", K_AUX9},
        {"AUX10", K_AUX10},
        {"AUX11", K_AUX11},
        {"AUX12", K_AUX12},
        {"AUX13", K_AUX13},
        {"AUX14", K_AUX14},
        {"AUX15", K_AUX15},
        {"AUX16", K_AUX16},

        {"KP_HOME", K_KP_HOME},
        {"KP_UPARROW", K_KP_UPARROW},
        {"KP_PGUP", K_KP_PGUP},
        {"KP_LEFTARROW", K_KP_LEFTARROW},
        {"KP_5", K_KP_5},
        {"KP_RIGHTARROW", K_KP_RIGHTARROW},
        {"KP_END", K_KP_END},
        {"KP_DOWNARROW", K_KP_DOWNARROW},
        {"KP_PGDN", K_KP_PGDN},
        {"KP_ENTER", K_KP_ENTER},
        {"KP_INS", K_KP_INS},
        {"KP_DEL", K_KP_DEL},
        {"KP_SLASH", K_KP_SLASH},
        {"KP_MINUS", K_KP_MINUS},
        {"KP_PLUS", K_KP_PLUS},
        {"KP_NUMLOCK", K_KP_NUMLOCK},
        {"KP_STAR", K_KP_STAR},
        {"KP_EQUALS", K_KP_EQUALS},

        {"PAUSE", K_PAUSE},

        {"SEMICOLON", ';'}, // because a raw semicolon seperates commands

        {NULL, 0}
};

/*
=============================================================================

EDIT FIELDS

=============================================================================
*/


/*
===================
Field_Draw

Handles horizontal scrolling and cursor blinking
x, y, amd width are in pixels
===================
*/
void Field_VariableSizeDraw(field_t* edit, int32 x, int32 y, int32 width, int32 size, bool showCursor)
{
    int32 len;
    int32 drawLen;
    int32 prestep;
    int32 cursorChar;
    char  str[MAX_STRING_CHARS];
    int32 i;

    drawLen = edit->widthInChars;
    len = strlen(edit->buffer) + 1;

    // guarantee that cursor will be visible
    if (len <= drawLen) {
        prestep = 0;
    } else {
        if (edit->scroll + drawLen > len) {
            edit->scroll = len - drawLen;
            if (edit->scroll < 0) {
                edit->scroll = 0;
            }
        }
        prestep = edit->scroll;

        /*
                if ( edit->cursor < len - drawLen ) {
                    prestep = edit->cursor;    // cursor at start
                } else {
                    prestep = len - drawLen;
                }
        */
    }

    if (prestep + drawLen > len) {
        drawLen = len - prestep;
    }

    // extract <drawLen> characters from the field at <prestep>
    if (drawLen >= MAX_STRING_CHARS) {
        Com_Error(ERR_DROP, "drawLen >= MAX_STRING_CHARS");
    }

    Com_Memcpy(str, edit->buffer + prestep, drawLen);
    str[drawLen] = 0;

    // draw it
    if (size == SMALLCHAR_WIDTH) {
        float color[4];

        color[0] = color[1] = color[2] = color[3] = 1.0;
        SCR_DrawSmallStringExt(x, y, str, color, false);
    } else {
        // draw big string with drop shadow
        SCR_DrawBigString(x, y, str, 1.0);
    }

    // draw the cursor
    if (!showCursor) {
        return;
    }

    if ((int32) (cls.realtime >> 8) & 1) {
        return; // off blink
    }

    if (key_overstrikeMode) {
        cursorChar = 11;
    } else {
        cursorChar = 10;
    }

    i = drawLen - (Q_PrintStrlen(str) + 1);

    if (size == SMALLCHAR_WIDTH) {
        SCR_DrawSmallChar(x + (edit->cursor - prestep - i) * size, y, cursorChar);
    } else {
        str[0] = cursorChar;
        str[1] = 0;
        SCR_DrawBigString(x + (edit->cursor - prestep - i) * size, y, str, 1.0);
    }
}

void Field_Draw(field_t* edit, int32 x, int32 y, int32 width, bool showCursor)
{
    Field_VariableSizeDraw(edit, x, y, width, SMALLCHAR_WIDTH, showCursor);
}

void Field_BigDraw(field_t* edit, int32 x, int32 y, int32 width, bool showCursor)
{
    Field_VariableSizeDraw(edit, x, y, width, BIGCHAR_WIDTH, showCursor);
}

/*
================
Field_Paste
================
*/
void Field_Paste(field_t* edit)
{
    char* cbd;
    int32 pasteLen, i;

    cbd = Sys_GetClipboardData();

    if (!cbd) {
        return;
    }

    // send as if typed, so insert / overstrike works properly
    pasteLen = strlen(cbd);
    for (i = 0; i < pasteLen; i++) {
        Field_CharEvent(edit, cbd[i]);
    }

    Z_Free(cbd);
}

/*
=================
Field_KeyDownEvent

Performs the basic line editing functions for the console,
in-game talk, and menu fields

Key events are used for non-printable characters, others are gotten from char events.
=================
*/
void Field_KeyDownEvent(field_t* edit, int32 key)
{
    int32 len;

    // shift-insert is paste
    if (((key == K_INS) || (key == K_KP_INS)) && keys[K_SHIFT].down) {
        Field_Paste(edit);
        return;
    }

    len = strlen(edit->buffer);

    if (key == K_DEL) {
        if (edit->cursor < len) {
            memmove(edit->buffer + edit->cursor, edit->buffer + edit->cursor + 1, len - edit->cursor);
        }
        return;
    }

    if (key == K_RIGHTARROW) {
        if (edit->cursor < len) {
            edit->cursor++;
        }

        if (edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= len) {
            edit->scroll++;
        }
        return;
    }

    if (key == K_LEFTARROW) {
        if (edit->cursor > 0) {
            edit->cursor--;
        }
        if (edit->cursor < edit->scroll) {
            edit->scroll--;
        }
        return;
    }

    if (key == K_HOME || (tolower(key) == 'a' && keys[K_CTRL].down)) {
        edit->cursor = 0;
        return;
    }

    if (key == K_END || (tolower(key) == 'e' && keys[K_CTRL].down)) {
        edit->cursor = len;
        return;
    }

    if (key == K_INS) {
        key_overstrikeMode = !key_overstrikeMode;
        return;
    }
}

/*
==================
Field_CharEvent
==================
*/
void Field_CharEvent(field_t* edit, int32 ch)
{
    int32 len;

    if (ch == 'v' - 'a' + 1) { // ctrl-v is paste
        Field_Paste(edit);
        return;
    }

    if (ch == 'c' - 'a' + 1) { // ctrl-c clears the field
        Field_Clear(edit);
        return;
    }

    len = strlen(edit->buffer);

    if (ch == 'h' - 'a' + 1) { // ctrl-h is backspace
        if (edit->cursor > 0) {
            memmove(edit->buffer + edit->cursor - 1, edit->buffer + edit->cursor, len + 1 - edit->cursor);
            edit->cursor--;
            if (edit->cursor < edit->scroll) {
                edit->scroll--;
            }
        }
        return;
    }

    if (ch == 'a' - 'a' + 1) { // ctrl-a is home
        edit->cursor = 0;
        edit->scroll = 0;
        return;
    }

    if (ch == 'e' - 'a' + 1) { // ctrl-e is end
        edit->cursor = len;
        edit->scroll = edit->cursor - edit->widthInChars;
        return;
    }

    //
    // ignore any other non printable chars
    //
    if (ch < 32) {
        return;
    }

    if (key_overstrikeMode) {
        if (edit->cursor == MAX_EDIT_LINE - 1) {
            return;
        }
        edit->buffer[edit->cursor] = ch;
        edit->cursor++;
    } else {        // insert mode
        if (len == MAX_EDIT_LINE - 1) {
            return; // all full
        }
        memmove(edit->buffer + edit->cursor + 1, edit->buffer + edit->cursor, len + 1 - edit->cursor);
        edit->buffer[edit->cursor] = ch;
        edit->cursor++;
    }


    if (edit->cursor >= edit->widthInChars) {
        edit->scroll++;
    }

    if (edit->cursor == len + 1) {
        edit->buffer[edit->cursor] = 0;
    }
}

/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

/*
====================
Console_Key

Handles history and console scrollback
====================
*/
void Console_Key(int32 key)
{
    // ctrl-L clears screen
    if (key == 'l' && keys[K_CTRL].down) {
        Cbuf_AddText("clear\n");
        return;
    }

    // enter finishes the line
    if (key == K_ENTER || key == K_KP_ENTER) {
        // if not in the game explicitly prepent a slash if needed
        if (cls.state != CA_ACTIVE && g_consoleField.buffer[0] != '\\'
            && g_consoleField.buffer[0] != '/') {
            char temp[MAX_STRING_CHARS];

            Q_strncpyz(temp, g_consoleField.buffer, sizeof(temp));
            Com_sprintf(g_consoleField.buffer, sizeof(g_consoleField.buffer), "\\%s", temp);
            g_consoleField.cursor++;
        }

        logger.info("]%s", g_consoleField.buffer);

        // leading slash is an explicit command
        if (g_consoleField.buffer[0] == '\\' || g_consoleField.buffer[0] == '/') {
            Cbuf_AddText(g_consoleField.buffer + 1); // valid command
            Cbuf_AddText("\n");
        } else {
            // other text will be chat messages
            if (!g_consoleField.buffer[0]) {
                return; // empty lines just scroll the console without adding to history
            } else {
                Cbuf_AddText("cmd say ");
                Cbuf_AddText(g_consoleField.buffer);
                Cbuf_AddText("\n");
            }
        }

        // copy line to history buffer
        historyEditLines[nextHistoryLine % COMMAND_HISTORY] = g_consoleField;
        nextHistoryLine++;
        historyLine = nextHistoryLine;

        Field_Clear(&g_consoleField);

        g_consoleField.widthInChars = g_console_field_width;

        if (cls.state == CA_DISCONNECTED) {
            SCR_UpdateScreen(); // force an update, because the command
        } // may take some time
        return;
    }

    // command completion

    if (key == K_TAB) {
        Field_CompleteCommand(&g_consoleField);
        return;
    }

    // command history (ctrl-p ctrl-n for unix style)

    if ((key == K_MWHEELUP && keys[K_SHIFT].down) || (key == K_UPARROW) || (key == K_KP_UPARROW) || ((tolower(key) == 'p') && keys[K_CTRL].down)) {
        if (nextHistoryLine - historyLine < COMMAND_HISTORY
            && historyLine > 0) {
            historyLine--;
        }
        g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];
        return;
    }

    if ((key == K_MWHEELDOWN && keys[K_SHIFT].down) || (key == K_DOWNARROW) || (key == K_KP_DOWNARROW) || ((tolower(key) == 'n') && keys[K_CTRL].down)) {
        if (historyLine == nextHistoryLine) {
            return;
        }
        historyLine++;
        g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];
        return;
    }

    // console scrolling
    if (key == K_PGUP) {
        Con_PageUp();
        return;
    }

    if (key == K_PGDN) {
        Con_PageDown();
        return;
    }

    if (key == K_MWHEELUP) {     //----(SA)    added some mousewheel functionality to the console
        Con_PageUp();
        if (keys[K_CTRL].down) { // hold <ctrl> to accelerate scrolling
            Con_PageUp();
            Con_PageUp();
        }
        return;
    }

    if (key == K_MWHEELDOWN) {   //----(SA)    added some mousewheel functionality to the console
        Con_PageDown();
        if (keys[K_CTRL].down) { // hold <ctrl> to accelerate scrolling
            Con_PageDown();
            Con_PageDown();
        }
        return;
    }

    // ctrl-home = top of console
    if (key == K_HOME && keys[K_CTRL].down) {
        Con_Top();
        return;
    }

    // ctrl-end = bottom of console
    if (key == K_END && keys[K_CTRL].down) {
        Con_Bottom();
        return;
    }

    // pass to the normal editline routine
    Field_KeyDownEvent(&g_consoleField, key);
}

//============================================================================


/*
================
Message_Key

In game talk message
================
*/
void Message_Key(int32 key)
{
    char buffer[MAX_STRING_CHARS];


    if (key == K_ESCAPE) {
        cls.keyCatchers &= ~KEYCATCH_MESSAGE;
        Field_Clear(&chatField);
        return;
    }

    if (key == K_ENTER || key == K_KP_ENTER) {
        if (chatField.buffer[0] && cls.state == CA_ACTIVE) {
            if (chat_playerNum != -1) {
                Com_sprintf(buffer, sizeof(buffer), "tell %i \"%s\"\n", chat_playerNum, chatField.buffer);
            }

            else if (chat_team) {
                Com_sprintf(buffer, sizeof(buffer), "say_team \"%s\"\n", chatField.buffer);
            } else {
                Com_sprintf(buffer, sizeof(buffer), "say \"%s\"\n", chatField.buffer);
            }


            CL_AddReliableCommand(buffer);
        }
        cls.keyCatchers &= ~KEYCATCH_MESSAGE;
        Field_Clear(&chatField);
        return;
    }

    Field_KeyDownEvent(&chatField, key);
}

//============================================================================


bool Key_GetOverstrikeMode()
{
    return key_overstrikeMode;
}


void Key_SetOverstrikeMode(bool state)
{
    key_overstrikeMode = state;
}


/*
===================
Key_IsDown
===================
*/
bool Key_IsDown(int32 keynum)
{
    if (keynum == -1) {
        return false;
    }

    return keys[keynum].down;
}


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keys[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.

0x11 will be interpreted as raw hex, which will allow new controlers

to be configured even if they don't have defined names.
===================
*/
int32 Key_StringToKeynum(const char* str)
{
    keyname_t* kn;

    if (!str || !str[0]) {
        return -1;
    }
    if (!str[1]) {
        return str[0];
    }

    // check for hex code
    if (str[0] == '0' && str[1] == 'x' && strlen(str) == 4) {
        int32 n1, n2;

        n1 = str[2];
        if (n1 >= '0' && n1 <= '9') {
            n1 -= '0';
        } else if (n1 >= 'a' && n1 <= 'f') {
            n1 = n1 - 'a' + 10;
        } else {
            n1 = 0;
        }

        n2 = str[3];
        if (n2 >= '0' && n2 <= '9') {
            n2 -= '0';
        } else if (n2 >= 'a' && n2 <= 'f') {
            n2 = n2 - 'a' + 10;
        } else {
            n2 = 0;
        }

        return n1 * 16 + n2;
    }

    // scan for a text match
    for (kn = keynames; kn->name; kn++) {
        if (!Q_stricmp(str, kn->name)) {
            return kn->keynum;
        }
    }

    return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
given keynum.
===================
*/
const char* Key_KeynumToString(int32 keynum)
{
    keyname_t*  kn;
    static char tinystr[5];
    int32       i, j;

    if (keynum == -1) {
        return "<KEY NOT FOUND>";
    }

    if (keynum < 0 || keynum > 255) {
        return "<OUT OF RANGE>";
    }

    // check for printable ascii (don't use quote)
    if (keynum > 32 && keynum < 127 && keynum != '"' && keynum != ';') {
        tinystr[0] = keynum;
        tinystr[1] = 0;
        return tinystr;
    }

    // check for a key string
    for (kn = keynames; kn->name; kn++) {
        if (keynum == kn->keynum) {
            return kn->name;
        }
    }

    // make a hex string
    i = keynum >> 4;
    j = keynum & 15;

    tinystr[0] = '0';
    tinystr[1] = 'x';
    tinystr[2] = i > 9 ? i - 10 + 'a' : i + '0';
    tinystr[3] = j > 9 ? j - 10 + 'a' : j + '0';
    tinystr[4] = 0;

    return tinystr;
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding(int32 keynum, const char* binding)
{
    if (keynum == -1) {
        return;
    }

    // free old bindings
    if (keys[keynum].binding) {
        Z_Free(keys[keynum].binding);
    }

    // allocate memory for new binding
    keys[keynum].binding = CopyString(binding);

    // consider this like modifying an archived cvar, so the
    // file write will be triggered at the next oportunity
    cvar_modifiedFlags |= CVAR_ARCHIVE;
}


/*
===================
Key_GetBinding
===================
*/
const char* Key_GetBinding(int32 keynum)
{
    if (keynum == -1) {
        return "";
    }

    return keys[keynum].binding;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f()
{
    int32 b;

    if (Cmd_Argc() != 2) {
        logger.info("unbind <key> : remove commands from a key");
        return;
    }

    b = Key_StringToKeynum(Cmd_Argv(1));
    if (b == -1) {
        logger.info("'%s' isn't a valid key", Cmd_Argv(1));
        return;
    }

    Key_SetBinding(b, "");
}

/*
===================
Key_Unbindall_f
===================
*/
void Key_Unbindall_f()
{
    int32 i;

    for (i = 0; i < 256; i++) {
        if (keys[i].binding) {
            Key_SetBinding(i, "");
        }
    }
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f()
{
    int32 i, c, b;
    char  cmd[1024];

    c = Cmd_Argc();

    if (c < 2) {
        logger.info("bind <key> [command] : attach a command to a key");
        return;
    }
    b = Key_StringToKeynum(Cmd_Argv(1));
    if (b == -1) {
        logger.info("'%s' isn't a valid key", Cmd_Argv(1));
        return;
    }

    if (c == 2) {
        if (keys[b].binding) {
            logger.info("'%s' = '%s'", Cmd_Argv(1), keys[b].binding);
        } else {
            logger.info("'%s' is not bound", Cmd_Argv(1));
        }
        return;
    }

    // copy the rest of the command line
    cmd[0] = 0; // start out with a null string
    for (i = 2; i < c; i++) {
        strcat(cmd, Cmd_Argv(i));
        if (i != (c - 1)) {
            strcat(cmd, " ");
        }
    }

    Key_SetBinding(b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings(fileHandle_t f)
{
    int32 i;

    FS_Printf(f, "unbindall\n");

    for (i = 0; i < 256; i++) {
        if (keys[i].binding && keys[i].binding[0]) {
            FS_Printf(f, "bind %s \"%s\"\n", Key_KeynumToString(i), keys[i].binding);
        }
    }
}


/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f()
{
    int32 i;

    for (i = 0; i < 256; i++) {
        if (keys[i].binding && keys[i].binding[0]) {
            logger.info("%s '%s'", Key_KeynumToString(i), keys[i].binding);
        }
    }
}

/*
===================
CL_InitKeyCommands
===================
*/
void CL_InitKeyCommands()
{
    // register our functions
    Cmd_AddCommand("bind", Key_Bind_f);
    Cmd_AddCommand("unbind", Key_Unbind_f);
    Cmd_AddCommand("unbindall", Key_Unbindall_f);
    Cmd_AddCommand("bindlist", Key_Bindlist_f);
}

/*
===================
CL_AddKeyUpCommands
===================
*/
void CL_AddKeyUpCommands(int32 key, char* kb)
{
    int32 i;
    char  button[1024], *buttonPtr;
    char  cmd[1024];
    bool  keyevent;

    if (!kb) {
        return;
    }
    keyevent = false;
    buttonPtr = button;
    for (i = 0;; i++) {
        if (kb[i] == ';' || !kb[i]) {
            *buttonPtr = '\0';
            if (button[0] == '+') {
                // button commands add keynum and time as parms so that multiple
                // sources can be discriminated and subframe corrected
                Com_sprintf(cmd, sizeof(cmd), "-%s %i %i\n", button + 1, key, time);
                Cbuf_AddText(cmd);
                keyevent = true;
            } else {
                if (keyevent) {
                    // down-only command
                    Cbuf_AddText(button);
                    Cbuf_AddText("\n");
                }
            }
            buttonPtr = button;
            while ((kb[i] <= ' ' || kb[i] == ';') && kb[i] != 0) {
                i++;
            }
        }
        *buttonPtr++ = kb[i];
        if (!kb[i]) {
            break;
        }
    }
}

/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
void CL_KeyEvent(int32 key, bool down, uint32 time)
{
    char* kb;
    char  cmd[1024];

    // update auto-repeat status and BUTTON_ANY status
    keys[key].down = down;

    if (down) {
        keys[key].repeats++;
        if (keys[key].repeats == 1) {
            anykeydown = true;
        }
    } else {
        keys[key].repeats = 0;
        anykeydown = false;
        if (anykeydown < 0) {
            anykeydown = 0;
        }
    }

    // console key is hardcoded, so the user can never unbind it
    if (key == '`' || key == '~') {
        if (!down) {
            return;
        }
        Con_ToggleConsole_f();
        return;
    }


    // keys can still be used for bound actions
    if (down && (key < 128 || key == K_MOUSE1) && clc.demoplaying && !cls.keyCatchers) {
        if (Cvar_VariableValue("com_cameraMode") == 0) {
            Cvar_Set("nextdemo", "");
            key = K_ESCAPE;
        }
    }


    // escape is always handled special
    if (key == K_ESCAPE && down) {
        if (cls.keyCatchers & KEYCATCH_MESSAGE) {
            // clear message mode
            Message_Key(key);
            return;
        }

        // escape always gets out of CGAME stuff
        if (cls.keyCatchers & KEYCATCH_CGAME) {
            cls.keyCatchers &= ~KEYCATCH_CGAME;
            CG_EventHandling(CGAME_EVENT_NONE);
            return;
        }

        if (!(cls.keyCatchers & KEYCATCH_UI)) {
            if (cls.state == CA_ACTIVE && !clc.demoplaying) {
                UI_SetActiveMenu(UIMENU_INGAME);
            } else {
                CL_Disconnect_f();
                S_StopAllSounds();
                UI_SetActiveMenu(UIMENU_MAIN);
            }
            return;
        }

        UI_KeyEvent(key, down);
        return;
    }

    //
    // key up events only perform actions if the game key binding is
    // a button command (leading + sign).  These will be processed even in
    // console mode and menu mode, to keep the character from continuing
    // an action started before a mode switch.
    //
    if (!down) {
        kb = keys[key].binding;

        CL_AddKeyUpCommands(key, kb);

        if (cls.keyCatchers & KEYCATCH_UI) {
            UI_KeyEvent(key, down);
        } else if (cls.keyCatchers & KEYCATCH_CGAME) {
            CG_KeyEvent(key, down);
        }

        return;
    }


    // distribute the key down event to the apropriate handler
    if (cls.keyCatchers & KEYCATCH_CONSOLE) {
        Console_Key(key);
    } else if (cls.keyCatchers & KEYCATCH_UI) {
        UI_KeyEvent(key, down);
    } else if (cls.keyCatchers & KEYCATCH_CGAME) {
        CG_KeyEvent(key, down);
    } else if (cls.keyCatchers & KEYCATCH_MESSAGE) {
        Message_Key(key);
    } else if (cls.state == CA_DISCONNECTED) {
        Console_Key(key);
    } else {
        // send the bound action
        kb = keys[key].binding;
        if (!kb) {
            if (key >= 200) {
                logger.info("%s is unbound, use controls menu to set.", Key_KeynumToString(key));
            }
        } else if (kb[0] == '+') {
            int32 i;
            char  button[1024], *buttonPtr;
            buttonPtr = button;
            for (i = 0;; i++) {
                if (kb[i] == ';' || !kb[i]) {
                    *buttonPtr = '\0';
                    if (button[0] == '+') {
                        // button commands add keynum and time as parms so that multiple
                        // sources can be discriminated and subframe corrected
                        Com_sprintf(cmd, sizeof(cmd), "%s %i %i\n", button, key, time);
                        Cbuf_AddText(cmd);
                    } else {
                        // down-only command
                        Cbuf_AddText(button);
                        Cbuf_AddText("\n");
                    }
                    buttonPtr = button;
                    while ((kb[i] <= ' ' || kb[i] == ';') && kb[i] != 0) {
                        i++;
                    }
                }
                *buttonPtr++ = kb[i];
                if (!kb[i]) {
                    break;
                }
            }
        } else {
            // down-only command
            Cbuf_AddText(kb);
            Cbuf_AddText("\n");
        }
    }
}


/*
===================
CL_CharEvent

Normal keyboard characters, already shifted / capslocked / etc
===================
*/
void CL_CharEvent(int32 key)
{
    // the console key should never be used as a char
    if (key == '`' || key == '~') {
        return;
    }

    // distribute the key down event to the apropriate handler
    if (cls.keyCatchers & KEYCATCH_CONSOLE) {
        Field_CharEvent(&g_consoleField, key);
    } else if (cls.keyCatchers & KEYCATCH_UI) {
        UI_KeyEvent(key | K_CHAR_FLAG, true);
    } else if (cls.keyCatchers & KEYCATCH_MESSAGE) {
        Field_CharEvent(&chatField, key);
    } else if (cls.state == CA_DISCONNECTED) {
        Field_CharEvent(&g_consoleField, key);
    }
}


/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates()
{
    int32 i;

    anykeydown = false;

    for (i = 0; i < MAX_KEYS; i++) {
        if (keys[i].down) {
            CL_KeyEvent(i, false, 0);
        }
        keys[i].down = 0;
        keys[i].repeats = 0;
    }
}

