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

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "l_log.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "../game/be_ea.h"
#include "../game/be_ai_chat.h"


// escape character
#define ESCAPE_CHAR            0x01 //'_'
//
// "hi ", people, " ", 0, " entered the game"
// becomes:
// "hi _rpeople_ _v0_ entered the game"
//

// match piece types
#define MT_VARIABLE            1 // variable match piece
#define MT_STRING              2 // string match piece
// reply chat key flags
#define RCKFL_AND              1   // key must be present
#define RCKFL_NOT              2   // key must be absent
#define RCKFL_NAME             4   // name of bot must be present
#define RCKFL_STRING           8   // key is a string
#define RCKFL_VARIABLES        16  // key is a match template
#define RCKFL_BOTNAMES         32  // key is a series of botnames
#define RCKFL_GENDERFEMALE     64  // bot must be female
#define RCKFL_GENDERMALE       128 // bot must be male
#define RCKFL_GENDERLESS       256 // bot must be genderless
// time to ignore a chat message after using it
#define CHATMESSAGE_RECENTTIME 20

// the actuall chat messages
typedef struct bot_chatmessage_s
{
    char*                     chatmessage; // chat message string
    float                     time;        // last time used
    struct bot_chatmessage_s* next;        // next chat message in a list
} bot_chatmessage_t;
// bot chat type with chat lines
typedef struct bot_chattype_s
{
    char                   name[MAX_CHATTYPE_NAME];
    int32                  numchatmessages;
    bot_chatmessage_t*     firstchatmessage;
    struct bot_chattype_s* next;
} bot_chattype_t;
// bot chat lines
typedef struct bot_chat_s
{
    bot_chattype_t* types;
} bot_chat_t;

// random string
typedef struct bot_randomstring_s
{
    char*                      string;
    struct bot_randomstring_s* next;
} bot_randomstring_t;
// list with random strings
typedef struct bot_randomlist_s
{
    char*                    string;
    int32                    numstrings;
    bot_randomstring_t*      firstrandomstring;
    struct bot_randomlist_s* next;
} bot_randomlist_t;

// synonym
typedef struct bot_synonym_s
{
    char*                 string;
    float                 weight;
    struct bot_synonym_s* next;
} bot_synonym_t;
// list with synonyms
typedef struct bot_synonymlist_s
{
    uint32                    context;
    float                     totalweight;
    bot_synonym_t*            firstsynonym;
    struct bot_synonymlist_s* next;
} bot_synonymlist_t;

// fixed match string
typedef struct bot_matchstring_s
{
    char*                     string;
    struct bot_matchstring_s* next;
} bot_matchstring_t;

// piece of a match template
typedef struct bot_matchpiece_s
{
    int32                    type;
    bot_matchstring_t*       firststring;
    int32                    variable;
    struct bot_matchpiece_s* next;
} bot_matchpiece_t;
// match template
typedef struct bot_matchtemplate_s
{
    uint32                      context;
    int32                       type;
    int32                       subtype;
    bot_matchpiece_t*           first;
    struct bot_matchtemplate_s* next;
} bot_matchtemplate_t;

// reply chat key
typedef struct bot_replychatkey_s
{
    int32                      flags;
    char*                      string;
    bot_matchpiece_t*          match;
    struct bot_replychatkey_s* next;
} bot_replychatkey_t;
// reply chat
typedef struct bot_replychat_s
{
    bot_replychatkey_t*     keys;
    float                   priority;
    int32                   numchatmessages;
    bot_chatmessage_t*      firstchatmessage;
    struct bot_replychat_s* next;
} bot_replychat_t;

// string list
typedef struct bot_stringlist_s
{
    char*                    string;
    struct bot_stringlist_s* next;
} bot_stringlist_t;

// chat state of a bot
typedef struct bot_chatstate_s
{
    int32 gender;   // 0=it, 1=female, 2=male
    int32 client;   // client number
    char  name[32]; // name of the bot
    char  chatmessage[MAX_MESSAGE_SIZE];
    int32 handle;
    // the console messages visible to the bot
    bot_consolemessage_t* firstmessage; // first message is the first typed message
    bot_consolemessage_t* lastmessage;  // last message is the last typed message, bottom of console
    // number of console messages stored in the state
    int32 numconsolemessages;
    // the bot chat lines
    bot_chat_t* chat;
} bot_chatstate_t;

typedef struct
{
    bot_chat_t* chat;
    char        filename[MAX_QPATH];
    char        chatname[MAX_QPATH];
} bot_ichatdata_t;

bot_ichatdata_t* ichatdata[MAX_CLIENTS];

bot_chatstate_t* botchatstates[MAX_CLIENTS + 1];
// console message heap
bot_consolemessage_t* consolemessageheap = NULL;
bot_consolemessage_t* freeconsolemessages = NULL;
// list with match strings
bot_matchtemplate_t* matchtemplates = NULL;
// list with synonyms
bot_synonymlist_t* synonyms = NULL;
// list with random strings
bot_randomlist_t* randomstrings = NULL;
// reply chats
bot_replychat_t* replychats = NULL;


bot_chatstate_t* BotChatStateFromHandle(int32 handle)
{
    if (handle <= 0 || handle > MAX_CLIENTS) {
        botimport.Print(PRT_FATAL, "chat state handle %d out of range\n", handle);
        return NULL;
    }
    if (!botchatstates[handle]) {
        botimport.Print(PRT_FATAL, "invalid chat state %d\n", handle);
        return NULL;
    }
    return botchatstates[handle];
}
//===========================================================================
// initialize the heap with unused console messages
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//===========================================================================
void InitConsoleMessageHeap()
{
    int32 i, max_messages;

    if (consolemessageheap) {
        FreeMemory(consolemessageheap);
    }
    //
    max_messages = (int32) LibVarValue("max_messages", "1024");
    consolemessageheap = (bot_consolemessage_t*) GetClearedHunkMemory(max_messages * sizeof(bot_consolemessage_t));
    consolemessageheap[0].prev = NULL;
    consolemessageheap[0].next = &consolemessageheap[1];
    for (i = 1; i < max_messages - 1; i++) {
        consolemessageheap[i].prev = &consolemessageheap[i - 1];
        consolemessageheap[i].next = &consolemessageheap[i + 1];
    }
    consolemessageheap[max_messages - 1].prev = &consolemessageheap[max_messages - 2];
    consolemessageheap[max_messages - 1].next = NULL;
    // pointer to the free console messages
    freeconsolemessages = consolemessageheap;
}
//===========================================================================
// allocate one console message from the heap
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//===========================================================================
bot_consolemessage_t* AllocConsoleMessage()
{
    bot_consolemessage_t* message;
    message = freeconsolemessages;
    if (freeconsolemessages) {
        freeconsolemessages = freeconsolemessages->next;
    }
    if (freeconsolemessages) {
        freeconsolemessages->prev = NULL;
    }
    return message;
}
//===========================================================================
// deallocate one console message from the heap
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//===========================================================================
void FreeConsoleMessage(bot_consolemessage_t* message)
{
    if (freeconsolemessages) {
        freeconsolemessages->prev = message;
    }
    message->prev = NULL;
    message->next = freeconsolemessages;
    freeconsolemessages = message;
}


void BotRemoveConsoleMessage(int32 chatstate, int32 handle)
{
    bot_consolemessage_t *m, *nextm;
    bot_chatstate_t*      cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return;
    }

    for (m = cs->firstmessage; m; m = nextm) {
        nextm = m->next;
        if (m->handle == handle) {
            if (m->next) {
                m->next->prev = m->prev;
            } else {
                cs->lastmessage = m->prev;
            }
            if (m->prev) {
                m->prev->next = m->next;
            } else {
                cs->firstmessage = m->next;
            }

            FreeConsoleMessage(m);
            cs->numconsolemessages--;
            break;
        }
    }
}


void BotQueueConsoleMessage(int32 chatstate, int32 type, char* message)
{
    bot_consolemessage_t* m;
    bot_chatstate_t*      cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return;
    }

    m = AllocConsoleMessage();
    if (!m) {
        botimport.Print(PRT_ERROR, "empty console message heap\n");
        return;
    }
    cs->handle++;
    if (cs->handle <= 0 || cs->handle > 8192) {
        cs->handle = 1;
    }
    m->handle = cs->handle;
    m->time = AAS_Time();
    m->type = type;
    strncpy(m->message, message, MAX_MESSAGE_SIZE);
    m->next = NULL;
    if (cs->lastmessage) {
        cs->lastmessage->next = m;
        m->prev = cs->lastmessage;
        cs->lastmessage = m;
    } else {
        cs->lastmessage = m;
        cs->firstmessage = m;
        m->prev = NULL;
    }
    cs->numconsolemessages++;
}


int32 BotNextConsoleMessage(int32 chatstate, bot_consolemessage_t* cm)
{
    bot_chatstate_t* cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return 0;
    }
    if (cs->firstmessage) {
        Com_Memcpy(cm, cs->firstmessage, sizeof(bot_consolemessage_t));
        cm->next = cm->prev = NULL;
        return cm->handle;
    }
    return 0;
}


int32 BotNumConsoleMessages(int32 chatstate)
{
    bot_chatstate_t* cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return 0;
    }
    return cs->numconsolemessages;
}


int32 IsWhiteSpace(char c)
{
    if ((c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == '(' || c == ')'
        || c == '?' || c == ':'
        || c == '\'' || c == '/'
        || c == ',' || c == '.'
        || c == '[' || c == ']'
        || c == '-' || c == '_'
        || c == '+' || c == '=') {
        return false;
    }
    return true;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void BotRemoveTildes(char* message)
{
    int32 i;

    // remove all tildes from the chat message
    for (i = 0; message[i]; i++) {
        if (message[i] == '~') {
            memmove(&message[i], &message[i + 1], strlen(&message[i + 1]) + 1);
        }
    }
}


void UnifyWhiteSpaces(char* string)
{
    char *ptr, *oldptr;

    for (ptr = oldptr = string; *ptr; oldptr = ptr) {
        while (*ptr && IsWhiteSpace(*ptr)) {
            ptr++;
        }
        if (ptr > oldptr) {
            // if not at the start and not at the end of the string
            // write only one space
            if (oldptr > string && *ptr) {
                *oldptr++ = ' ';
            }
            // remove all other white spaces
            if (ptr > oldptr) {
                memmove(oldptr, ptr, strlen(ptr) + 1);
            }
        }
        while (*ptr && !IsWhiteSpace(*ptr)) {
            ptr++;
        }
    }
}


int32 StringContains(char* str1, char* str2, int32 casesensitive)
{
    int32 len, i, j, index;

    if (str1 == NULL || str2 == NULL) {
        return -1;
    }

    len = strlen(str1) - strlen(str2);
    index = 0;
    for (i = 0; i <= len; i++, str1++, index++) {
        for (j = 0; str2[j]; j++) {
            if (casesensitive) {
                if (str1[j] != str2[j]) {
                    break;
                }
            } else {
                if (toupper(str1[j]) != toupper(str2[j])) {
                    break;
                }
            }
        }
        if (!str2[j]) {
            return index;
        }
    }
    return -1;
}


char* StringContainsWord(char* str1, char* str2, int32 casesensitive)
{
    int32 len, i, j;

    len = strlen(str1) - strlen(str2);
    for (i = 0; i <= len; i++, str1++) {
        // if not at the start of the string
        if (i) {
            // skip to the start of the next word
            while (*str1 && *str1 != ' ' && *str1 != '.' && *str1 != ',' && *str1 != '!') {
                str1++;
            }
            if (!*str1) {
                break;
            }
            str1++;
        }
        // compare the word
        for (j = 0; str2[j]; j++) {
            if (casesensitive) {
                if (str1[j] != str2[j]) {
                    break;
                }
            } else {
                if (toupper(str1[j]) != toupper(str2[j])) {
                    break;
                }
            }
        }
        // if there was a word match
        if (!str2[j]) {
            // if the first string has an end of word
            if (!str1[j] || str1[j] == ' ' || str1[j] == '.' || str1[j] == ',' || str1[j] == '!') {
                return str1;
            }
        }
    }
    return NULL;
}


void StringReplaceWords(char* string, char* synonym, char* replacement)
{
    char *str, *str2;

    // find the synonym in the string
    str = StringContainsWord(string, synonym, false);
    // if the synonym occured in the string
    while (str) {
        // if the synonym isn't part of the replacement which is already in the string
        // usefull for abreviations
        str2 = StringContainsWord(string, replacement, false);
        while (str2) {
            if (str2 <= str && str < str2 + strlen(replacement)) {
                break;
            }
            str2 = StringContainsWord(str2 + 1, replacement, false);
        }
        if (!str2) {
            memmove(str + strlen(replacement), str + strlen(synonym), strlen(str + strlen(synonym)) + 1);
            // append the synonum replacement
            Com_Memcpy(str, replacement, strlen(replacement));
        }
        // find the next synonym in the string
        str = StringContainsWord(str + strlen(replacement), synonym, false);
    }
}

bot_synonymlist_t* BotLoadSynonyms(char* filename)
{
    int32              pass, size, contextlevel, numsynonyms;
    uint32             context, contextstack[32];
    char*              ptr = NULL;
    source_t*          source;
    token_t            token;
    bot_synonymlist_t *synlist, *lastsyn, *syn;
    bot_synonym_t *    synonym, *lastsynonym;

    size = 0;
    synlist = NULL; // make compiler happy
    syn = NULL;     // make compiler happy
    synonym = NULL; // make compiler happy
    // the synonyms are parsed in two phases
    for (pass = 0; pass < 2; pass++) {
        //
        if (pass && size) {
            ptr = (char*) GetClearedHunkMemory(size);
        }
        //
        PC_SetBaseFolder(BOTFILESBASEFOLDER);
        source = LoadSourceFile(filename);
        if (!source) {
            botimport.Print(PRT_ERROR, "counldn't load %s\n", filename);
            return NULL;
        }
        //
        context = 0;
        contextlevel = 0;
        synlist = NULL; // list synonyms
        lastsyn = NULL; // last synonym in the list
        //
        while (PC_ReadToken(source, &token)) {
            if (token.type == TT_NUMBER) {
                context |= token.intvalue;
                contextstack[contextlevel] = token.intvalue;
                contextlevel++;
                if (contextlevel >= 32) {
                    SourceError(source, "more than 32 context levels");
                    FreeSource(source);
                    return NULL;
                }
                if (!PC_ExpectTokenString(source, "{")) {
                    FreeSource(source);
                    return NULL;
                }
            } else if (token.type == TT_PUNCTUATION) {
                if (!strcmp(token.string, "}")) {
                    contextlevel--;
                    if (contextlevel < 0) {
                        SourceError(source, "too many }");
                        FreeSource(source);
                        return NULL;
                    }
                    context &= ~contextstack[contextlevel];
                } else if (!strcmp(token.string, "[")) {
                    size += sizeof(bot_synonymlist_t);
                    if (pass) {
                        syn = (bot_synonymlist_t*) ptr;
                        ptr += sizeof(bot_synonymlist_t);
                        syn->context = context;
                        syn->firstsynonym = NULL;
                        syn->next = NULL;
                        if (lastsyn) {
                            lastsyn->next = syn;
                        } else {
                            synlist = syn;
                        }
                        lastsyn = syn;
                    }
                    numsynonyms = 0;
                    lastsynonym = NULL;
                    while (1) {
                        if (!PC_ExpectTokenString(source, "(") || !PC_ExpectTokenType(source, TT_STRING, 0, &token)) {
                            FreeSource(source);
                            return NULL;
                        }
                        StripDoubleQuotes(token.string);
                        if (strlen(token.string) <= 0) {
                            SourceError(source, "empty string", token.string);
                            FreeSource(source);
                            return NULL;
                        }
                        size += sizeof(bot_synonym_t) + strlen(token.string) + 1;
                        if (pass) {
                            synonym = (bot_synonym_t*) ptr;
                            ptr += sizeof(bot_synonym_t);
                            synonym->string = ptr;
                            ptr += strlen(token.string) + 1;
                            strcpy(synonym->string, token.string);
                            //
                            if (lastsynonym) {
                                lastsynonym->next = synonym;
                            } else {
                                syn->firstsynonym = synonym;
                            }
                            lastsynonym = synonym;
                        }
                        numsynonyms++;
                        if (!PC_ExpectTokenString(source, ",") || !PC_ExpectTokenType(source, TT_NUMBER, 0, &token) || !PC_ExpectTokenString(source, ")")) {
                            FreeSource(source);
                            return NULL;
                        }
                        if (pass) {
                            synonym->weight = token.floatvalue;
                            syn->totalweight += synonym->weight;
                        }
                        if (PC_CheckTokenString(source, "]")) {
                            break;
                        }
                        if (!PC_ExpectTokenString(source, ",")) {
                            FreeSource(source);
                            return NULL;
                        }
                    }
                    if (numsynonyms < 2) {
                        SourceError(source, "synonym must have at least two entries\n");
                        FreeSource(source);
                        return NULL;
                    }
                } else {
                    SourceError(source, "unexpected %s", token.string);
                    FreeSource(source);
                    return NULL;
                }
            }
        }
        //
        FreeSource(source);
        //
        if (contextlevel > 0) {
            SourceError(source, "missing }");
            return NULL;
        }
    }
    botimport.Print(PRT_MESSAGE, "loaded %s\n", filename);
    return synlist;
}
//===========================================================================
// replace all the synonyms in the string
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//===========================================================================
void BotReplaceSynonyms(char* string, uint32 context)
{
    bot_synonymlist_t* syn;
    bot_synonym_t*     synonym;

    for (syn = synonyms; syn; syn = syn->next) {
        if (!(syn->context & context)) {
            continue;
        }
        for (synonym = syn->firstsynonym->next; synonym; synonym = synonym->next) {
            StringReplaceWords(string, synonym->string, syn->firstsynonym->string);
        }
    }
}


void BotReplaceWeightedSynonyms(char* string, uint32 context)
{
    bot_synonymlist_t* syn;
    bot_synonym_t *    synonym, *replacement;
    float              weight, curweight;

    for (syn = synonyms; syn; syn = syn->next) {
        if (!(syn->context & context)) {
            continue;
        }
        // choose a weighted random replacement synonym
        weight = random() * syn->totalweight;
        if (!weight) {
            continue;
        }
        curweight = 0;
        for (replacement = syn->firstsynonym; replacement; replacement = replacement->next) {
            curweight += replacement->weight;
            if (weight < curweight) {
                break;
            }
        }
        if (!replacement) {
            continue;
        }
        // replace all synonyms with the replacement
        for (synonym = syn->firstsynonym; synonym; synonym = synonym->next) {
            if (synonym == replacement) {
                continue;
            }
            StringReplaceWords(string, synonym->string, replacement->string);
        }
    }
}


void BotReplaceReplySynonyms(char* string, uint32 context)
{
    char *             str1, *str2, *replacement;
    bot_synonymlist_t* syn;
    bot_synonym_t*     synonym;

    for (str1 = string; *str1;) {
        // go to the start of the next word
        while (*str1 && *str1 <= ' ') {
            str1++;
        }
        if (!*str1) {
            break;
        }
        //
        for (syn = synonyms; syn; syn = syn->next) {
            if (!(syn->context & context)) {
                continue;
            }
            for (synonym = syn->firstsynonym->next; synonym; synonym = synonym->next) {
                str2 = synonym->string;
                // if the synonym is not at the front of the string continue
                str2 = StringContainsWord(str1, synonym->string, false);
                if (!str2 || str2 != str1) {
                    continue;
                }
                //
                replacement = syn->firstsynonym->string;
                // if the replacement IS in front of the string continue
                str2 = StringContainsWord(str1, replacement, false);
                if (str2 && str2 == str1) {
                    continue;
                }
                //
                memmove(str1 + strlen(replacement), str1 + strlen(synonym->string), strlen(str1 + strlen(synonym->string)) + 1);
                // append the synonum replacement
                Com_Memcpy(str1, replacement, strlen(replacement));
                //
                break;
            }
            // if a synonym has been replaced
            if (synonym) {
                break;
            }
        }
        // skip over this word
        while (*str1 && *str1 > ' ') {
            str1++;
        }
        if (!*str1) {
            break;
        }
    }
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
int32 BotLoadChatMessage(source_t* source, char* chatmessagestring)
{
    char*   ptr;
    token_t token;

    ptr = chatmessagestring;
    *ptr = 0;
    //
    while (1) {
        if (!PC_ExpectAnyToken(source, &token)) {
            return false;
        }
        // fixed string
        if (token.type == TT_STRING) {
            StripDoubleQuotes(token.string);
            if (strlen(ptr) + strlen(token.string) + 1 > MAX_MESSAGE_SIZE) {
                SourceError(source, "chat message too long\n");
                return false;
            }
            strcat(ptr, token.string);
        }
        // variable string
        else if (token.type == TT_NUMBER && (token.subtype & TT_INTEGER)) {
            if (strlen(ptr) + 7 > MAX_MESSAGE_SIZE) {
                SourceError(source, "chat message too long\n");
                return false;
            }
            sprintf(&ptr[strlen(ptr)], "%cv%ld%c", ESCAPE_CHAR, token.intvalue, ESCAPE_CHAR);
        }
        // random string
        else if (token.type == TT_NAME) {
            if (strlen(ptr) + 7 > MAX_MESSAGE_SIZE) {
                SourceError(source, "chat message too long\n");
                return false;
            }
            sprintf(&ptr[strlen(ptr)], "%cr%s%c", ESCAPE_CHAR, token.string, ESCAPE_CHAR);
        } else {
            SourceError(source, "unknown message component %s\n", token.string);
            return false;
        }
        if (PC_CheckTokenString(source, ";")) {
            break;
        }
        if (!PC_ExpectTokenString(source, ",")) {
            return false;
        }
    }
    //
    return true;
}

bot_randomlist_t* BotLoadRandomStrings(char* filename)
{
    int32               pass, size;
    char *              ptr = NULL, chatmessagestring[MAX_MESSAGE_SIZE];
    source_t*           source;
    token_t             token;
    bot_randomlist_t *  randomlist, *lastrandom, *random;
    bot_randomstring_t* randomstring;

#ifdef DEBUG
    int32 starttime = Sys_MilliSeconds();
#endif // DEBUG

    size = 0;
    randomlist = NULL;
    random = NULL;
    // the synonyms are parsed in two phases
    for (pass = 0; pass < 2; pass++) {
        //
        if (pass && size) {
            ptr = (char*) GetClearedHunkMemory(size);
        }
        //
        PC_SetBaseFolder(BOTFILESBASEFOLDER);
        source = LoadSourceFile(filename);
        if (!source) {
            botimport.Print(PRT_ERROR, "counldn't load %s\n", filename);
            return NULL;
        }
        //
        randomlist = NULL; // list
        lastrandom = NULL; // last
        //
        while (PC_ReadToken(source, &token)) {
            if (token.type != TT_NAME) {
                SourceError(source, "unknown random %s", token.string);
                FreeSource(source);
                return NULL;
            }
            size += sizeof(bot_randomlist_t) + strlen(token.string) + 1;
            if (pass) {
                random = (bot_randomlist_t*) ptr;
                ptr += sizeof(bot_randomlist_t);
                random->string = ptr;
                ptr += strlen(token.string) + 1;
                strcpy(random->string, token.string);
                random->firstrandomstring = NULL;
                random->numstrings = 0;
                //
                if (lastrandom) {
                    lastrandom->next = random;
                } else {
                    randomlist = random;
                }
                lastrandom = random;
            }
            if (!PC_ExpectTokenString(source, "=") || !PC_ExpectTokenString(source, "{")) {
                FreeSource(source);
                return NULL;
            }
            while (!PC_CheckTokenString(source, "}")) {
                if (!BotLoadChatMessage(source, chatmessagestring)) {
                    FreeSource(source);
                    return NULL;
                }
                size += sizeof(bot_randomstring_t) + strlen(chatmessagestring) + 1;
                if (pass) {
                    randomstring = (bot_randomstring_t*) ptr;
                    ptr += sizeof(bot_randomstring_t);
                    randomstring->string = ptr;
                    ptr += strlen(chatmessagestring) + 1;
                    strcpy(randomstring->string, chatmessagestring);
                    //
                    random->numstrings++;
                    randomstring->next = random->firstrandomstring;
                    random->firstrandomstring = randomstring;
                }
            }
        }
        // free the source after one pass
        FreeSource(source);
    }
    botimport.Print(PRT_MESSAGE, "loaded %s\n", filename);
    //
#ifdef DEBUG
    botimport.Print(PRT_MESSAGE, "random strings %d msec\n", Sys_MilliSeconds() - starttime);
#endif // DEBUG
    //
    return randomlist;
}


char* RandomString(char* name)
{
    bot_randomlist_t*   random;
    bot_randomstring_t* rs;
    int32               i;

    for (random = randomstrings; random; random = random->next) {
        if (!strcmp(random->string, name)) {
            i = random() * random->numstrings;
            for (rs = random->firstrandomstring; rs; rs = rs->next) {
                if (--i < 0) {
                    break;
                }
            }
            if (rs) {
                return rs->string;
            }
        }
    }
    return NULL;
}

void BotFreeMatchPieces(bot_matchpiece_t* matchpieces)
{
    bot_matchpiece_t * mp, *nextmp;
    bot_matchstring_t *ms, *nextms;

    for (mp = matchpieces; mp; mp = nextmp) {
        nextmp = mp->next;
        if (mp->type == MT_STRING) {
            for (ms = mp->firststring; ms; ms = nextms) {
                nextms = ms->next;
                FreeMemory(ms);
            }
        }
        FreeMemory(mp);
    }
}


bot_matchpiece_t* BotLoadMatchPieces(source_t* source, const char* endtoken)
{
    int32              lastwasvariable, emptystring;
    token_t            token;
    bot_matchpiece_t * matchpiece, *firstpiece, *lastpiece;
    bot_matchstring_t *matchstring, *lastmatchstring;

    firstpiece = NULL;
    lastpiece = NULL;
    //
    lastwasvariable = false;
    //
    while (PC_ReadToken(source, &token)) {
        if (token.type == TT_NUMBER && (token.subtype & TT_INTEGER)) {
            if (token.intvalue < 0 || token.intvalue >= MAX_MATCHVARIABLES) {
                SourceError(source, "can't have more than %d match variables\n", MAX_MATCHVARIABLES);
                FreeSource(source);
                BotFreeMatchPieces(firstpiece);
                return NULL;
            }
            if (lastwasvariable) {
                SourceError(source, "not allowed to have adjacent variables\n");
                FreeSource(source);
                BotFreeMatchPieces(firstpiece);
                return NULL;
            }
            lastwasvariable = true;
            //
            matchpiece = (bot_matchpiece_t*) GetClearedHunkMemory(sizeof(bot_matchpiece_t));
            matchpiece->type = MT_VARIABLE;
            matchpiece->variable = token.intvalue;
            matchpiece->next = NULL;
            if (lastpiece) {
                lastpiece->next = matchpiece;
            } else {
                firstpiece = matchpiece;
            }
            lastpiece = matchpiece;
        } else if (token.type == TT_STRING) {
            //
            matchpiece = (bot_matchpiece_t*) GetClearedHunkMemory(sizeof(bot_matchpiece_t));
            matchpiece->firststring = NULL;
            matchpiece->type = MT_STRING;
            matchpiece->variable = 0;
            matchpiece->next = NULL;
            if (lastpiece) {
                lastpiece->next = matchpiece;
            } else {
                firstpiece = matchpiece;
            }
            lastpiece = matchpiece;
            //
            lastmatchstring = NULL;
            emptystring = false;
            //
            do {
                if (matchpiece->firststring) {
                    if (!PC_ExpectTokenType(source, TT_STRING, 0, &token)) {
                        FreeSource(source);
                        BotFreeMatchPieces(firstpiece);
                        return NULL;
                    }
                }
                StripDoubleQuotes(token.string);
                matchstring = (bot_matchstring_t*) GetClearedHunkMemory(sizeof(bot_matchstring_t) + strlen(token.string) + 1);
                matchstring->string = (char*) matchstring + sizeof(bot_matchstring_t);
                strcpy(matchstring->string, token.string);
                if (!strlen(token.string)) {
                    emptystring = true;
                }
                matchstring->next = NULL;
                if (lastmatchstring) {
                    lastmatchstring->next = matchstring;
                } else {
                    matchpiece->firststring = matchstring;
                }
                lastmatchstring = matchstring;
            } while (PC_CheckTokenString(source, "|"));
            // if there was no empty string found
            if (!emptystring) {
                lastwasvariable = false;
            }
        } else {
            SourceError(source, "invalid token %s\n", token.string);
            FreeSource(source);
            BotFreeMatchPieces(firstpiece);
            return NULL;
        }
        if (PC_CheckTokenString(source, endtoken)) {
            break;
        }
        if (!PC_ExpectTokenString(source, ",")) {
            FreeSource(source);
            BotFreeMatchPieces(firstpiece);
            return NULL;
        }
    }
    return firstpiece;
}


void BotFreeMatchTemplates(bot_matchtemplate_t* mt)
{
    bot_matchtemplate_t* nextmt;

    for (; mt; mt = nextmt) {
        nextmt = mt->next;
        BotFreeMatchPieces(mt->first);
        FreeMemory(mt);
    }
}


bot_matchtemplate_t* BotLoadMatchTemplates(char* matchfile)
{
    source_t*            source;
    token_t              token;
    bot_matchtemplate_t *matchtemplate, *matches, *lastmatch;
    uint32               context;

    PC_SetBaseFolder(BOTFILESBASEFOLDER);
    source = LoadSourceFile(matchfile);
    if (!source) {
        botimport.Print(PRT_ERROR, "counldn't load %s\n", matchfile);
        return NULL;
    }
    //
    matches = NULL;   // list with matches
    lastmatch = NULL; // last match in the list

    while (PC_ReadToken(source, &token)) {
        if (token.type != TT_NUMBER || !(token.subtype & TT_INTEGER)) {
            SourceError(source, "expected integer, found %s\n", token.string);
            BotFreeMatchTemplates(matches);
            FreeSource(source);
            return NULL;
        }
        // the context
        context = token.intvalue;
        //
        if (!PC_ExpectTokenString(source, "{")) {
            BotFreeMatchTemplates(matches);
            FreeSource(source);
            return NULL;
        }
        //
        while (PC_ReadToken(source, &token)) {
            if (!strcmp(token.string, "}")) {
                break;
            }
            //
            PC_UnreadLastToken(source);
            //
            matchtemplate = (bot_matchtemplate_t*) GetClearedHunkMemory(sizeof(bot_matchtemplate_t));
            matchtemplate->context = context;
            matchtemplate->next = NULL;
            // add the match template to the list
            if (lastmatch) {
                lastmatch->next = matchtemplate;
            } else {
                matches = matchtemplate;
            }
            lastmatch = matchtemplate;
            // load the match template
            matchtemplate->first = BotLoadMatchPieces(source, "=");
            if (!matchtemplate->first) {
                BotFreeMatchTemplates(matches);
                return NULL;
            }
            // read the match type
            if (!PC_ExpectTokenString(source, "(") || !PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token)) {
                BotFreeMatchTemplates(matches);
                FreeSource(source);
                return NULL;
            }
            matchtemplate->type = token.intvalue;
            // read the match subtype
            if (!PC_ExpectTokenString(source, ",") || !PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token)) {
                BotFreeMatchTemplates(matches);
                FreeSource(source);
                return NULL;
            }
            matchtemplate->subtype = token.intvalue;
            // read trailing punctuations
            if (!PC_ExpectTokenString(source, ")") || !PC_ExpectTokenString(source, ";")) {
                BotFreeMatchTemplates(matches);
                FreeSource(source);
                return NULL;
            }
        }
    }
    // free the source
    FreeSource(source);
    botimport.Print(PRT_MESSAGE, "loaded %s\n", matchfile);
    return matches;
}


int32 StringsMatch(bot_matchpiece_t* pieces, bot_match_t* match)
{
    int32              lastvariable, index;
    char *             strptr, *newstrptr;
    bot_matchpiece_t*  mp;
    bot_matchstring_t* ms;

    // no last variable
    lastvariable = -1;
    // pointer to the string to compare the match string with
    strptr = match->string;
    // Log_Write("match: %s", strptr);
    // compare the string with the current match string
    for (mp = pieces; mp; mp = mp->next) {
        // if it is a piece of string
        if (mp->type == MT_STRING) {
            newstrptr = NULL;
            for (ms = mp->firststring; ms; ms = ms->next) {
                if (!strlen(ms->string)) {
                    newstrptr = strptr;
                    break;
                }
                // Log_Write("MT_STRING: %s", mp->string);
                index = StringContains(strptr, ms->string, false);
                if (index >= 0) {
                    newstrptr = strptr + index;
                    if (lastvariable >= 0) {
                        match->variables[lastvariable].length =
                            (newstrptr - match->string) - match->variables[lastvariable].offset;
                        // newstrptr - match->variables[lastvariable].ptr;
                        lastvariable = -1;
                        break;
                    } else if (index == 0) {
                        break;
                    }
                    newstrptr = NULL;
                }
            }
            if (!newstrptr) {
                return false;
            }
            strptr = newstrptr + strlen(ms->string);
        }
        // if it is a variable piece of string
        else if (mp->type == MT_VARIABLE) {
            // Log_Write("MT_VARIABLE");
            match->variables[mp->variable].offset = strptr - match->string;
            lastvariable = mp->variable;
        }
    }
    // if a match was found
    if (!mp && (lastvariable >= 0 || !strlen(strptr))) {
        // if the last piece was a variable string
        if (lastvariable >= 0) {
            assert(match->variables[lastvariable].offset >= 0); // bk001204
            match->variables[lastvariable].length =
                strlen(&match->string[(int32) match->variables[lastvariable].offset]);
        }
        return true;
    }
    return false;
}


int32 BotFindMatch(char* str, bot_match_t* match, uint32 context)
{
    int32                i;
    bot_matchtemplate_t* ms;

    strncpy(match->string, str, MAX_MESSAGE_SIZE);
    // remove any trailing enters
    while (strlen(match->string) && match->string[strlen(match->string) - 1] == '\n') {
        match->string[strlen(match->string) - 1] = '\0';
    }
    // compare the string with all the match strings
    for (ms = matchtemplates; ms; ms = ms->next) {
        if (!(ms->context & context)) {
            continue;
        }
        // reset the match variable offsets
        for (i = 0; i < MAX_MATCHVARIABLES; i++) {
            match->variables[i].offset = -1;
        }
        //
        if (StringsMatch(ms->first, match)) {
            match->type = ms->type;
            match->subtype = ms->subtype;
            return true;
        }
    }
    return false;
}


void BotMatchVariable(bot_match_t* match, int32 variable, char* buf, int32 size)
{
    if (variable < 0 || variable >= MAX_MATCHVARIABLES) {
        botimport.Print(PRT_FATAL, "BotMatchVariable: variable out of range\n");
        strcpy(buf, "");
        return;
    }

    if (match->variables[variable].offset >= 0) {
        if (match->variables[variable].length < size) {
            size = match->variables[variable].length + 1;
        }
        assert(match->variables[variable].offset >= 0); // bk001204
        strncpy(buf, &match->string[(int32) match->variables[variable].offset], size - 1);
        buf[size - 1] = '\0';
    } else {
        strcpy(buf, "");
    }
    return;
}


bot_stringlist_t* BotFindStringInList(bot_stringlist_t* list, char* string)
{
    bot_stringlist_t* s;

    for (s = list; s; s = s->next) {
        if (!strcmp(s->string, string)) {
            return s;
        }
    }
    return NULL;
}


bot_stringlist_t* BotCheckChatMessageIntegrety(char* message, bot_stringlist_t* stringlist)
{
    int32             i;
    char*             msgptr;
    char              temp[MAX_MESSAGE_SIZE];
    bot_stringlist_t* s;

    msgptr = message;
    //
    while (*msgptr) {
        if (*msgptr == ESCAPE_CHAR) {
            msgptr++;
            switch (*msgptr) {
            case 'v': // variable
            {
                // step over the 'v'
                msgptr++;
                while (*msgptr && *msgptr != ESCAPE_CHAR) {
                    msgptr++;
                }
                // step over the trailing escape char
                if (*msgptr) {
                    msgptr++;
                }
                break;
            }
            case 'r': // random
            {
                // step over the 'r'
                msgptr++;
                for (i = 0; (*msgptr && *msgptr != ESCAPE_CHAR); i++) {
                    temp[i] = *msgptr++;
                }
                temp[i] = '\0';
                // step over the trailing escape char
                if (*msgptr) {
                    msgptr++;
                }
                // find the random keyword
                if (!RandomString(temp)) {
                    if (!BotFindStringInList(stringlist, temp)) {
                        Log_Write("%s = {\"%s\"} //MISSING RANDOM\r\n", temp, temp);
                        s = (bot_stringlist_t*) GetClearedMemory(sizeof(bot_stringlist_t) + strlen(temp) + 1);
                        s->string = (char*) s + sizeof(bot_stringlist_t);
                        strcpy(s->string, temp);
                        s->next = stringlist;
                        stringlist = s;
                    }
                }
                break;
            }
            default: {
                botimport.Print(PRT_FATAL, "BotCheckChatMessageIntegrety: message \"%s\" invalid escape char\n", message);
                break;
            }
            }
        } else {
            msgptr++;
        }
    }
    return stringlist;
}


void BotCheckInitialChatIntegrety(bot_chat_t* chat)
{
    bot_chattype_t*    t;
    bot_chatmessage_t* cm;
    bot_stringlist_t * stringlist, *s, *nexts;

    stringlist = NULL;
    for (t = chat->types; t; t = t->next) {
        for (cm = t->firstchatmessage; cm; cm = cm->next) {
            stringlist = BotCheckChatMessageIntegrety(cm->chatmessage, stringlist);
        }
    }
    for (s = stringlist; s; s = nexts) {
        nexts = s->next;
        FreeMemory(s);
    }
}


void BotCheckReplyChatIntegrety(bot_replychat_t* replychat)
{
    bot_replychat_t*   rp;
    bot_chatmessage_t* cm;
    bot_stringlist_t * stringlist, *s, *nexts;

    stringlist = NULL;
    for (rp = replychat; rp; rp = rp->next) {
        for (cm = rp->firstchatmessage; cm; cm = cm->next) {
            stringlist = BotCheckChatMessageIntegrety(cm->chatmessage, stringlist);
        }
    }
    for (s = stringlist; s; s = nexts) {
        nexts = s->next;
        FreeMemory(s);
    }
}

void BotFreeReplyChat(bot_replychat_t* replychat)
{
    bot_replychat_t *   rp, *nextrp;
    bot_replychatkey_t *key, *nextkey;
    bot_chatmessage_t * cm, *nextcm;

    for (rp = replychat; rp; rp = nextrp) {
        nextrp = rp->next;
        for (key = rp->keys; key; key = nextkey) {
            nextkey = key->next;
            if (key->match) {
                BotFreeMatchPieces(key->match);
            }
            if (key->string) {
                FreeMemory(key->string);
            }
            FreeMemory(key);
        }
        for (cm = rp->firstchatmessage; cm; cm = nextcm) {
            nextcm = cm->next;
            FreeMemory(cm);
        }
        FreeMemory(rp);
    }
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void BotCheckValidReplyChatKeySet(source_t* source, bot_replychatkey_t* keys)
{
    int32               allprefixed, hasvariableskey, hasstringkey;
    bot_matchpiece_t*   m;
    bot_matchstring_t*  ms;
    bot_replychatkey_t *key, *key2;

    //
    allprefixed = true;
    hasvariableskey = hasstringkey = false;
    for (key = keys; key; key = key->next) {
        if (!(key->flags & (RCKFL_AND | RCKFL_NOT))) {
            allprefixed = false;
            if (key->flags & RCKFL_VARIABLES) {
                for (m = key->match; m; m = m->next) {
                    if (m->type == MT_VARIABLE) {
                        hasvariableskey = true;
                    }
                }
            } else if (key->flags & RCKFL_STRING) {
                hasstringkey = true;
            }
        } else if ((key->flags & RCKFL_AND) && (key->flags & RCKFL_STRING)) {
            for (key2 = keys; key2; key2 = key2->next) {
                if (key2 == key) {
                    continue;
                }
                if (key2->flags & RCKFL_NOT) {
                    continue;
                }
                if (key2->flags & RCKFL_VARIABLES) {
                    for (m = key2->match; m; m = m->next) {
                        if (m->type == MT_STRING) {
                            for (ms = m->firststring; ms; ms = ms->next) {
                                if (StringContains(ms->string, key->string, false) != -1) {
                                    break;
                                }
                            }
                            if (ms) {
                                break;
                            }
                        } else if (m->type == MT_VARIABLE) {
                            break;
                        }
                    }
                    if (!m) {
                        SourceWarning(source,
                                      "one of the match templates does not "
                                      "leave space for the key %s with the & prefix",
                                      key->string);
                    }
                }
            }
        }
        if ((key->flags & RCKFL_NOT) && (key->flags & RCKFL_STRING)) {
            for (key2 = keys; key2; key2 = key2->next) {
                if (key2 == key) {
                    continue;
                }
                if (key2->flags & RCKFL_NOT) {
                    continue;
                }
                if (key2->flags & RCKFL_STRING) {
                    if (StringContains(key2->string, key->string, false) != -1) {
                        SourceWarning(source, "the key %s with prefix ! is inside the key %s", key->string, key2->string);
                    }
                } else if (key2->flags & RCKFL_VARIABLES) {
                    for (m = key2->match; m; m = m->next) {
                        if (m->type == MT_STRING) {
                            for (ms = m->firststring; ms; ms = ms->next) {
                                if (StringContains(ms->string, key->string, false) != -1) {
                                    SourceWarning(source,
                                                  "the key %s with prefix ! is inside "
                                                  "the match template string %s",
                                                  key->string, ms->string);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (allprefixed) {
        SourceWarning(source, "all keys have a & or ! prefix");
    }
    if (hasvariableskey && hasstringkey) {
        SourceWarning(source,
                      "variables from the match template(s) could be "
                      "invalid when outputting one of the chat messages");
    }
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
bot_replychat_t* BotLoadReplyChat(char* filename)
{
    char                chatmessagestring[MAX_MESSAGE_SIZE];
    char                namebuffer[MAX_MESSAGE_SIZE];
    source_t*           source;
    token_t             token;
    bot_chatmessage_t*  chatmessage = NULL;
    bot_replychat_t *   replychat, *replychatlist;
    bot_replychatkey_t* key;

    PC_SetBaseFolder(BOTFILESBASEFOLDER);
    source = LoadSourceFile(filename);
    if (!source) {
        botimport.Print(PRT_ERROR, "counldn't load %s\n", filename);
        return NULL;
    }
    //
    replychatlist = NULL;
    //
    while (PC_ReadToken(source, &token)) {
        if (strcmp(token.string, "[")) {
            SourceError(source, "expected [, found %s", token.string);
            BotFreeReplyChat(replychatlist);
            FreeSource(source);
            return NULL;
        }
        //
        replychat = (bot_replychat_t*) GetClearedHunkMemory(sizeof(bot_replychat_t));
        replychat->keys = NULL;
        replychat->next = replychatlist;
        replychatlist = replychat;
        // read the keys, there must be at least one key
        do {
            // allocate a key
            key = (bot_replychatkey_t*) GetClearedHunkMemory(sizeof(bot_replychatkey_t));
            key->flags = 0;
            key->string = NULL;
            key->match = NULL;
            key->next = replychat->keys;
            replychat->keys = key;
            // check for MUST BE PRESENT and MUST BE ABSENT keys
            if (PC_CheckTokenString(source, "&")) {
                key->flags |= RCKFL_AND;
            } else if (PC_CheckTokenString(source, "!")) {
                key->flags |= RCKFL_NOT;
            }
            // special keys
            if (PC_CheckTokenString(source, "name")) {
                key->flags |= RCKFL_NAME;
            } else if (PC_CheckTokenString(source, "female")) {
                key->flags |= RCKFL_GENDERFEMALE;
            } else if (PC_CheckTokenString(source, "male")) {
                key->flags |= RCKFL_GENDERMALE;
            } else if (PC_CheckTokenString(source, "it")) {
                key->flags |= RCKFL_GENDERLESS;
            } else if (PC_CheckTokenString(source, "(")) // match key
            {
                key->flags |= RCKFL_VARIABLES;
                key->match = BotLoadMatchPieces(source, ")");
                if (!key->match) {
                    BotFreeReplyChat(replychatlist);
                    return NULL;
                }
            } else if (PC_CheckTokenString(source, "<")) // bot names
            {
                key->flags |= RCKFL_BOTNAMES;
                strcpy(namebuffer, "");
                do {
                    if (!PC_ExpectTokenType(source, TT_STRING, 0, &token)) {
                        BotFreeReplyChat(replychatlist);
                        FreeSource(source);
                        return NULL;
                    }
                    StripDoubleQuotes(token.string);
                    if (strlen(namebuffer)) {
                        strcat(namebuffer, "\\");
                    }
                    strcat(namebuffer, token.string);
                } while (PC_CheckTokenString(source, ","));
                if (!PC_ExpectTokenString(source, ">")) {
                    BotFreeReplyChat(replychatlist);
                    FreeSource(source);
                    return NULL;
                }
                key->string = (char*) GetClearedHunkMemory(strlen(namebuffer) + 1);
                strcpy(key->string, namebuffer);
            } else // normal string key
            {
                key->flags |= RCKFL_STRING;
                if (!PC_ExpectTokenType(source, TT_STRING, 0, &token)) {
                    BotFreeReplyChat(replychatlist);
                    FreeSource(source);
                    return NULL;
                }
                StripDoubleQuotes(token.string);
                key->string = (char*) GetClearedHunkMemory(strlen(token.string) + 1);
                strcpy(key->string, token.string);
            }
            //
            PC_CheckTokenString(source, ",");
        } while (!PC_CheckTokenString(source, "]"));
        //
        BotCheckValidReplyChatKeySet(source, replychat->keys);
        // read the = sign and the priority
        if (!PC_ExpectTokenString(source, "=") || !PC_ExpectTokenType(source, TT_NUMBER, 0, &token)) {
            BotFreeReplyChat(replychatlist);
            FreeSource(source);
            return NULL;
        }
        replychat->priority = token.floatvalue;
        // read the leading {
        if (!PC_ExpectTokenString(source, "{")) {
            BotFreeReplyChat(replychatlist);
            FreeSource(source);
            return NULL;
        }
        replychat->numchatmessages = 0;
        // while the trailing } is not found
        while (!PC_CheckTokenString(source, "}")) {
            if (!BotLoadChatMessage(source, chatmessagestring)) {
                BotFreeReplyChat(replychatlist);
                FreeSource(source);
                return NULL;
            }
            chatmessage = (bot_chatmessage_t*) GetClearedHunkMemory(sizeof(bot_chatmessage_t) + strlen(chatmessagestring) + 1);
            chatmessage->chatmessage = (char*) chatmessage + sizeof(bot_chatmessage_t);
            strcpy(chatmessage->chatmessage, chatmessagestring);
            chatmessage->time = -2 * CHATMESSAGE_RECENTTIME;
            chatmessage->next = replychat->firstchatmessage;
            // add the chat message to the reply chat
            replychat->firstchatmessage = chatmessage;
            replychat->numchatmessages++;
        }
    }
    FreeSource(source);
    botimport.Print(PRT_MESSAGE, "loaded %s\n", filename);
    //
    if (bot_developer) {
        BotCheckReplyChatIntegrety(replychatlist);
    }
    //
    if (!replychatlist) {
        botimport.Print(PRT_MESSAGE, "no rchats\n");
    }
    //
    return replychatlist;
}

bot_chat_t* BotLoadInitialChat(char* chatfile, char* chatname)
{
    int32              pass, foundchat, indent, size;
    char*              ptr = NULL;
    char               chatmessagestring[MAX_MESSAGE_SIZE];
    source_t*          source;
    token_t            token;
    bot_chat_t*        chat = NULL;
    bot_chattype_t*    chattype = NULL;
    bot_chatmessage_t* chatmessage = NULL;
#ifdef DEBUG
    int32 starttime;

    starttime = Sys_MilliSeconds();
#endif // DEBUG
    //
    size = 0;
    foundchat = false;
    // a bot chat is parsed in two phases
    for (pass = 0; pass < 2; pass++) {
        // allocate memory
        if (pass && size) {
            ptr = (char*) GetClearedMemory(size);
        }
        // load the source file
        PC_SetBaseFolder(BOTFILESBASEFOLDER);
        source = LoadSourceFile(chatfile);
        if (!source) {
            botimport.Print(PRT_ERROR, "counldn't load %s\n", chatfile);
            return NULL;
        }
        // chat structure
        if (pass) {
            chat = (bot_chat_t*) ptr;
            ptr += sizeof(bot_chat_t);
        }
        size = sizeof(bot_chat_t);
        //
        while (PC_ReadToken(source, &token)) {
            if (!strcmp(token.string, "chat")) {
                if (!PC_ExpectTokenType(source, TT_STRING, 0, &token)) {
                    FreeSource(source);
                    return NULL;
                }
                StripDoubleQuotes(token.string);
                // after the chat name we expect a opening brace
                if (!PC_ExpectTokenString(source, "{")) {
                    FreeSource(source);
                    return NULL;
                }
                // if the chat name is found
                if (!Q_stricmp(token.string, chatname)) {
                    foundchat = true;
                    // read the chat types
                    while (1) {
                        if (!PC_ExpectAnyToken(source, &token)) {
                            FreeSource(source);
                            return NULL;
                        }
                        if (!strcmp(token.string, "}")) {
                            break;
                        }
                        if (strcmp(token.string, "type")) {
                            SourceError(source, "expected type found %s\n", token.string);
                            FreeSource(source);
                            return NULL;
                        }
                        // expect the chat type name
                        if (!PC_ExpectTokenType(source, TT_STRING, 0, &token) || !PC_ExpectTokenString(source, "{")) {
                            FreeSource(source);
                            return NULL;
                        }
                        StripDoubleQuotes(token.string);
                        if (pass) {
                            chattype = (bot_chattype_t*) ptr;
                            strncpy(chattype->name, token.string, MAX_CHATTYPE_NAME);
                            chattype->firstchatmessage = NULL;
                            // add the chat type to the chat
                            chattype->next = chat->types;
                            chat->types = chattype;
                            //
                            ptr += sizeof(bot_chattype_t);
                        }
                        size += sizeof(bot_chattype_t);
                        // read the chat messages
                        while (!PC_CheckTokenString(source, "}")) {
                            if (!BotLoadChatMessage(source, chatmessagestring)) {
                                FreeSource(source);
                                return NULL;
                            }
                            if (pass) {
                                chatmessage = (bot_chatmessage_t*) ptr;
                                chatmessage->time = -2 * CHATMESSAGE_RECENTTIME;
                                // put the chat message in the list
                                chatmessage->next = chattype->firstchatmessage;
                                chattype->firstchatmessage = chatmessage;
                                // store the chat message
                                ptr += sizeof(bot_chatmessage_t);
                                chatmessage->chatmessage = ptr;
                                strcpy(chatmessage->chatmessage, chatmessagestring);
                                ptr += strlen(chatmessagestring) + 1;
                                // the number of chat messages increased
                                chattype->numchatmessages++;
                            }
                            size += sizeof(bot_chatmessage_t) + strlen(chatmessagestring) + 1;
                        }
                    }
                } else // skip the bot chat
                {
                    indent = 1;
                    while (indent) {
                        if (!PC_ExpectAnyToken(source, &token)) {
                            FreeSource(source);
                            return NULL;
                        }
                        if (!strcmp(token.string, "{")) {
                            indent++;
                        } else if (!strcmp(token.string, "}")) {
                            indent--;
                        }
                    }
                }
            } else {
                SourceError(source, "unknown definition %s\n", token.string);
                FreeSource(source);
                return NULL;
            }
        }
        // free the source
        FreeSource(source);
        // if the requested character is not found
        if (!foundchat) {
            botimport.Print(PRT_ERROR, "couldn't find chat %s in %s\n", chatname, chatfile);
            return NULL;
        }
    }
    //
    botimport.Print(PRT_MESSAGE, "loaded %s from %s\n", chatname, chatfile);
    //
    if (bot_developer) {
        BotCheckInitialChatIntegrety(chat);
    }
#ifdef DEBUG
    botimport.Print(PRT_MESSAGE, "initial chats loaded in %d msec\n", Sys_MilliSeconds() - starttime);
#endif // DEBUG
    // character was read succesfully
    return chat;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void BotFreeChatFile(int32 chatstate)
{
    bot_chatstate_t* cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return;
    }
    if (cs->chat) {
        FreeMemory(cs->chat);
    }
    cs->chat = NULL;
}


int32 BotLoadChatFile(int32 chatstate, char* chatfile, char* chatname)
{
    bot_chatstate_t* cs;
    int32            n, avail = 0;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return BLERR_CANNOTLOADICHAT;
    }
    BotFreeChatFile(chatstate);

    if (!LibVarGetValue("bot_reloadcharacters")) {
        avail = -1;
        for (n = 0; n < MAX_CLIENTS; n++) {
            if (!ichatdata[n]) {
                if (avail == -1) {
                    avail = n;
                }
                continue;
            }
            if (strcmp(chatfile, ichatdata[n]->filename) != 0) {
                continue;
            }
            if (strcmp(chatname, ichatdata[n]->chatname) != 0) {
                continue;
            }
            cs->chat = ichatdata[n]->chat;
            //        botimport.Print( PRT_MESSAGE, "retained %s from %s\n", chatname, chatfile );
            return BLERR_NOERROR;
        }

        if (avail == -1) {
            botimport.Print(PRT_FATAL, "ichatdata table full; couldn't load chat %s from %s\n", chatname, chatfile);
            return BLERR_CANNOTLOADICHAT;
        }
    }

    cs->chat = BotLoadInitialChat(chatfile, chatname);
    if (!cs->chat) {
        botimport.Print(PRT_FATAL, "couldn't load chat %s from %s\n", chatname, chatfile);
        return BLERR_CANNOTLOADICHAT;
    }
    if (!LibVarGetValue("bot_reloadcharacters")) {
        ichatdata[avail] = (bot_ichatdata_t*) GetClearedMemory(sizeof(bot_ichatdata_t));
        ichatdata[avail]->chat = cs->chat;
        Q_strncpyz(ichatdata[avail]->chatname, chatname, sizeof(ichatdata[avail]->chatname));
        Q_strncpyz(ichatdata[avail]->filename, chatfile, sizeof(ichatdata[avail]->filename));
    }

    return BLERR_NOERROR;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
int32 BotExpandChatMessage(char* outmessage, char* message, uint32 mcontext, bot_match_t* match, uint32 vcontext, int32 reply)
{
    int32 num, len, i, expansion;
    char *outputbuf, *ptr, *msgptr;
    char  temp[MAX_MESSAGE_SIZE];

    expansion = false;
    msgptr = message;
    outputbuf = outmessage;
    len = 0;
    //
    while (*msgptr) {
        if (*msgptr == ESCAPE_CHAR) {
            msgptr++;
            switch (*msgptr) {
            case 'v': // variable
            {
                msgptr++;
                num = 0;
                while (*msgptr && *msgptr != ESCAPE_CHAR) {
                    num = num * 10 + (*msgptr++) - '0';
                }
                // step over the trailing escape char
                if (*msgptr) {
                    msgptr++;
                }
                if (num > MAX_MATCHVARIABLES) {
                    botimport.Print(PRT_ERROR, "BotConstructChat: message %s variable %d out of range\n", message, num);
                    return false;
                }
                if (match->variables[num].offset >= 0) {
                    assert(match->variables[num].offset >= 0); // bk001204
                    ptr = &match->string[(int32) match->variables[num].offset];
                    for (i = 0; i < match->variables[num].length; i++) {
                        temp[i] = ptr[i];
                    }
                    temp[i] = 0;
                    // if it's a reply message
                    if (reply) {
                        // replace the reply synonyms in the variables
                        BotReplaceReplySynonyms(temp, vcontext);
                    } else {
                        // replace synonyms in the variable context
                        BotReplaceSynonyms(temp, vcontext);
                    }
                    //
                    if (len + strlen(temp) >= MAX_MESSAGE_SIZE) {
                        botimport.Print(PRT_ERROR, "BotConstructChat: message %s too long\n", message);
                        return false;
                    }
                    strcpy(&outputbuf[len], temp);
                    len += strlen(temp);
                }
                break;
            }
            case 'r': // random
            {
                msgptr++;
                for (i = 0; (*msgptr && *msgptr != ESCAPE_CHAR); i++) {
                    temp[i] = *msgptr++;
                }
                temp[i] = '\0';
                // step over the trailing escape char
                if (*msgptr) {
                    msgptr++;
                }
                // find the random keyword
                ptr = RandomString(temp);
                if (!ptr) {
                    botimport.Print(PRT_ERROR, "BotConstructChat: unknown random string %s\n", temp);
                    return false;
                }
                if (len + strlen(ptr) >= MAX_MESSAGE_SIZE) {
                    botimport.Print(PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message);
                    return false;
                }
                strcpy(&outputbuf[len], ptr);
                len += strlen(ptr);
                expansion = true;
                break;
            }
            default: {
                botimport.Print(PRT_FATAL, "BotConstructChat: message \"%s\" invalid escape char\n", message);
                break;
            }
            }
        } else {
            outputbuf[len++] = *msgptr++;
            if (len >= MAX_MESSAGE_SIZE) {
                botimport.Print(PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message);
                break;
            }
        }
    }
    outputbuf[len] = '\0';
    // replace synonyms weighted in the message context
    BotReplaceWeightedSynonyms(outputbuf, mcontext);
    // return true if a random was expanded
    return expansion;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void BotConstructChatMessage(bot_chatstate_t* chatstate, char* message, uint32 mcontext, bot_match_t* match, uint32 vcontext, int32 reply)
{
    int32 i;
    char  srcmessage[MAX_MESSAGE_SIZE];

    strcpy(srcmessage, message);
    for (i = 0; i < 10; i++) {
        if (!BotExpandChatMessage(chatstate->chatmessage, srcmessage, mcontext, match, vcontext, reply)) {
            break;
        }
        strcpy(srcmessage, chatstate->chatmessage);
    }
    if (i >= 10) {
        botimport.Print(PRT_WARNING, "too many expansions in chat message\n");
        botimport.Print(PRT_WARNING, "%s\n", chatstate->chatmessage);
    }
}
//===========================================================================
// randomly chooses one of the chat message of the given type
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//===========================================================================
char* BotChooseInitialChatMessage(bot_chatstate_t* cs, char* type)
{
    int32              n, numchatmessages;
    float              besttime;
    bot_chattype_t*    t;
    bot_chatmessage_t *m, *bestchatmessage;
    bot_chat_t*        chat;

    chat = cs->chat;
    for (t = chat->types; t; t = t->next) {
        if (!Q_stricmp(t->name, type)) {
            numchatmessages = 0;
            for (m = t->firstchatmessage; m; m = m->next) {
                if (m->time > AAS_Time()) {
                    continue;
                }
                numchatmessages++;
            }
            // if all chat messages have been used recently
            if (numchatmessages <= 0) {
                besttime = 0;
                bestchatmessage = NULL;
                for (m = t->firstchatmessage; m; m = m->next) {
                    if (!besttime || m->time < besttime) {
                        bestchatmessage = m;
                        besttime = m->time;
                    }
                }
                if (bestchatmessage) {
                    return bestchatmessage->chatmessage;
                }
            } else // choose a chat message randomly
            {
                n = random() * numchatmessages;
                for (m = t->firstchatmessage; m; m = m->next) {
                    if (m->time > AAS_Time()) {
                        continue;
                    }
                    if (--n < 0) {
                        m->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
                        return m->chatmessage;
                    }
                }
            }
            return NULL;
        }
    }
    return NULL;
}


int32 BotNumInitialChats(int32 chatstate, char* type)
{
    bot_chatstate_t* cs;
    bot_chattype_t*  t;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return 0;
    }

    for (t = cs->chat->types; t; t = t->next) {
        if (!Q_stricmp(t->name, type)) {
            if (LibVarGetValue("bot_testichat")) {
                botimport.Print(PRT_MESSAGE, "%s has %d chat lines\n", type, t->numchatmessages);
                botimport.Print(PRT_MESSAGE, "-------------------\n");
            }
            return t->numchatmessages;
        }
    }
    return 0;
}


void BotInitialChat(int32 chatstate, char* type, int32 mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
{
    char*            message;
    int32            index;
    bot_match_t      match;
    bot_chatstate_t* cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return;
    }
    // if no chat file is loaded
    if (!cs->chat) {
        return;
    }
    // choose a chat message randomly of the given type
    message = BotChooseInitialChatMessage(cs, type);
    // if there's no message of the given type
    if (!message) {
#ifdef DEBUG
        botimport.Print(PRT_MESSAGE, "no chat messages of type %s\n", type);
#endif // DEBUG
        return;
    }
    //
    Com_Memset(&match, 0, sizeof(match));
    index = 0;
    if (var0) {
        strcat(match.string, var0);
        match.variables[0].offset = index;
        match.variables[0].length = strlen(var0);
        index += strlen(var0);
    }
    if (var1) {
        strcat(match.string, var1);
        match.variables[1].offset = index;
        match.variables[1].length = strlen(var1);
        index += strlen(var1);
    }
    if (var2) {
        strcat(match.string, var2);
        match.variables[2].offset = index;
        match.variables[2].length = strlen(var2);
        index += strlen(var2);
    }
    if (var3) {
        strcat(match.string, var3);
        match.variables[3].offset = index;
        match.variables[3].length = strlen(var3);
        index += strlen(var3);
    }
    if (var4) {
        strcat(match.string, var4);
        match.variables[4].offset = index;
        match.variables[4].length = strlen(var4);
        index += strlen(var4);
    }
    if (var5) {
        strcat(match.string, var5);
        match.variables[5].offset = index;
        match.variables[5].length = strlen(var5);
        index += strlen(var5);
    }
    if (var6) {
        strcat(match.string, var6);
        match.variables[6].offset = index;
        match.variables[6].length = strlen(var6);
        index += strlen(var6);
    }
    if (var7) {
        strcat(match.string, var7);
        match.variables[7].offset = index;
        match.variables[7].length = strlen(var7);
        index += strlen(var7);
    }
    //
    BotConstructChatMessage(cs, message, mcontext, &match, 0, false);
}

int32 BotReplyChat(int32 chatstate, char* message, int32 mcontext, int32 vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
{
    bot_replychat_t *   rchat, *bestrchat;
    bot_replychatkey_t* key;
    bot_chatmessage_t * m, *bestchatmessage;
    bot_match_t         match, bestmatch;
    int32               bestpriority, num, found, res, numchatmessages, index;
    bot_chatstate_t*    cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return false;
    }
    Com_Memset(&match, 0, sizeof(bot_match_t));
    strcpy(match.string, message);
    bestpriority = -1;
    bestchatmessage = NULL;
    bestrchat = NULL;
    // go through all the reply chats
    for (rchat = replychats; rchat; rchat = rchat->next) {
        found = false;
        for (key = rchat->keys; key; key = key->next) {
            res = false;
            // get the match result
            if (key->flags & RCKFL_NAME) {
                res = (StringContains(message, cs->name, false) != -1);
            } else if (key->flags & RCKFL_BOTNAMES) {
                res = (StringContains(key->string, cs->name, false) != -1);
            } else if (key->flags & RCKFL_GENDERFEMALE) {
                res = (cs->gender == CHAT_GENDERFEMALE);
            } else if (key->flags & RCKFL_GENDERMALE) {
                res = (cs->gender == CHAT_GENDERMALE);
            } else if (key->flags & RCKFL_GENDERLESS) {
                res = (cs->gender == CHAT_GENDERLESS);
            } else if (key->flags & RCKFL_VARIABLES) {
                res = StringsMatch(key->match, &match);
            } else if (key->flags & RCKFL_STRING) {
                res = (StringContainsWord(message, key->string, false) != NULL);
            }
            // if the key must be present
            if (key->flags & RCKFL_AND) {
                if (!res) {
                    found = false;
                    break;
                }
            }
            // if the key must be absent
            else if (key->flags & RCKFL_NOT) {
                if (res) {
                    found = false;
                    break;
                }
            } else if (res) {
                found = true;
            }
        }
        //
        if (found) {
            if (rchat->priority > bestpriority) {
                numchatmessages = 0;
                for (m = rchat->firstchatmessage; m; m = m->next) {
                    if (m->time > AAS_Time()) {
                        continue;
                    }
                    numchatmessages++;
                }
                num = random() * numchatmessages;
                for (m = rchat->firstchatmessage; m; m = m->next) {
                    if (--num < 0) {
                        break;
                    }
                    if (m->time > AAS_Time()) {
                        continue;
                    }
                }
                // if the reply chat has a message
                if (m) {
                    Com_Memcpy(&bestmatch, &match, sizeof(bot_match_t));
                    bestchatmessage = m;
                    bestrchat = rchat;
                    bestpriority = rchat->priority;
                }
            }
        }
    }
    if (bestchatmessage) {
        index = strlen(bestmatch.string);
        if (var0) {
            strcat(bestmatch.string, var0);
            bestmatch.variables[0].offset = index;
            bestmatch.variables[0].length = strlen(var0);
            index += strlen(var0);
        }
        if (var1) {
            strcat(bestmatch.string, var1);
            bestmatch.variables[1].offset = index;
            bestmatch.variables[1].length = strlen(var1);
            index += strlen(var1);
        }
        if (var2) {
            strcat(bestmatch.string, var2);
            bestmatch.variables[2].offset = index;
            bestmatch.variables[2].length = strlen(var2);
            index += strlen(var2);
        }
        if (var3) {
            strcat(bestmatch.string, var3);
            bestmatch.variables[3].offset = index;
            bestmatch.variables[3].length = strlen(var3);
            index += strlen(var3);
        }
        if (var4) {
            strcat(bestmatch.string, var4);
            bestmatch.variables[4].offset = index;
            bestmatch.variables[4].length = strlen(var4);
            index += strlen(var4);
        }
        if (var5) {
            strcat(bestmatch.string, var5);
            bestmatch.variables[5].offset = index;
            bestmatch.variables[5].length = strlen(var5);
            index += strlen(var5);
        }
        if (var6) {
            strcat(bestmatch.string, var6);
            bestmatch.variables[6].offset = index;
            bestmatch.variables[6].length = strlen(var6);
            index += strlen(var6);
        }
        if (var7) {
            strcat(bestmatch.string, var7);
            bestmatch.variables[7].offset = index;
            bestmatch.variables[7].length = strlen(var7);
            index += strlen(var7);
        }
        if (LibVarGetValue("bot_testrchat")) {
            for (m = bestrchat->firstchatmessage; m; m = m->next) {
                BotConstructChatMessage(cs, m->chatmessage, mcontext, &bestmatch, vcontext, true);
                BotRemoveTildes(cs->chatmessage);
                botimport.Print(PRT_MESSAGE, "%s\n", cs->chatmessage);
            }
        } else {
            bestchatmessage->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
            BotConstructChatMessage(cs, bestchatmessage->chatmessage, mcontext, &bestmatch, vcontext, true);
        }
        return true;
    }
    return false;
}


int32 BotChatLength(int32 chatstate)
{
    bot_chatstate_t* cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return 0;
    }
    return strlen(cs->chatmessage);
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void BotEnterChat(int32 chatstate, int32 clientto, int32 sendto)
{
    bot_chatstate_t* cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return;
    }

    if (strlen(cs->chatmessage)) {
        BotRemoveTildes(cs->chatmessage);
        if (LibVarGetValue("bot_testichat")) {
            botimport.Print(PRT_MESSAGE, "%s\n", cs->chatmessage);
        } else {
            switch (sendto) {
            case CHAT_TEAM:
                EA_Command(cs->client, va("say_team %s", cs->chatmessage));
                break;
            case CHAT_TELL:
                EA_Command(cs->client, va("tell %d %s", clientto, cs->chatmessage));
                break;
            default: // CHAT_ALL
                EA_Command(cs->client, va("say %s", cs->chatmessage));
                break;
            }
        }
        // clear the chat message from the state
        strcpy(cs->chatmessage, "");
    }
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void BotGetChatMessage(int32 chatstate, char* buf, int32 size)
{
    bot_chatstate_t* cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return;
    }

    BotRemoveTildes(cs->chatmessage);
    strncpy(buf, cs->chatmessage, size - 1);
    buf[size - 1] = '\0';
    // clear the chat message from the state
    strcpy(cs->chatmessage, "");
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void BotSetChatGender(int32 chatstate, int32 gender)
{
    bot_chatstate_t* cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return;
    }
    switch (gender) {
    case CHAT_GENDERFEMALE:
        cs->gender = CHAT_GENDERFEMALE;
        break;
    case CHAT_GENDERMALE:
        cs->gender = CHAT_GENDERMALE;
        break;
    default:
        cs->gender = CHAT_GENDERLESS;
        break;
    }
}


void BotSetChatName(int32 chatstate, char* name, int32 client)
{
    bot_chatstate_t* cs;

    cs = BotChatStateFromHandle(chatstate);
    if (!cs) {
        return;
    }
    cs->client = client;
    Com_Memset(cs->name, 0, sizeof(cs->name));
    strncpy(cs->name, name, sizeof(cs->name));
    cs->name[sizeof(cs->name) - 1] = '\0';
}

int32 BotAllocChatState()
{
    int32 i;

    for (i = 1; i <= MAX_CLIENTS; i++) {
        if (!botchatstates[i]) {
            botchatstates[i] = (bot_chatstate_t*) GetClearedMemory(sizeof(bot_chatstate_t));
            return i;
        }
    }
    return 0;
}


void BotFreeChatState(int32 handle)
{
    bot_chatstate_t*     cs;
    bot_consolemessage_t m;
    int32                h;

    if (handle <= 0 || handle > MAX_CLIENTS) {
        botimport.Print(PRT_FATAL, "chat state handle %d out of range\n", handle);
        return;
    }
    if (!botchatstates[handle]) {
        botimport.Print(PRT_FATAL, "invalid chat state %d\n", handle);
        return;
    }
    cs = botchatstates[handle];
    if (LibVarGetValue("bot_reloadcharacters")) {
        BotFreeChatFile(handle);
    }
    // free all the console messages left in the chat state
    for (h = BotNextConsoleMessage(handle, &m); h; h = BotNextConsoleMessage(handle, &m)) {
        // remove the console message
        BotRemoveConsoleMessage(handle, h);
    }
    FreeMemory(botchatstates[handle]);
    botchatstates[handle] = NULL;
}


int32 BotSetupChatAI()
{
    char* file;

#ifdef DEBUG
    int32 starttime = Sys_MilliSeconds();
#endif // DEBUG

    file = LibVarString("synfile", "syn.c");
    synonyms = BotLoadSynonyms(file);
    file = LibVarString("rndfile", "rnd.c");
    randomstrings = BotLoadRandomStrings(file);
    file = LibVarString("matchfile", "match.c");
    matchtemplates = BotLoadMatchTemplates(file);
    //
    if (!LibVarValue("nochat", "0")) {
        file = LibVarString("rchatfile", "rchat.c");
        replychats = BotLoadReplyChat(file);
    }

    InitConsoleMessageHeap();

#ifdef DEBUG
    botimport.Print(PRT_MESSAGE, "setup chat AI %d msec\n", Sys_MilliSeconds() - starttime);
#endif // DEBUG
    return BLERR_NOERROR;
}


void BotShutdownChatAI()
{
    int32 i;

    // free all remaining chat states
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (botchatstates[i]) {
            BotFreeChatState(i);
        }
    }
    // free all cached chats
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (ichatdata[i]) {
            FreeMemory(ichatdata[i]->chat);
            FreeMemory(ichatdata[i]);
            ichatdata[i] = NULL;
        }
    }
    if (consolemessageheap) {
        FreeMemory(consolemessageheap);
    }
    consolemessageheap = NULL;
    if (matchtemplates) {
        BotFreeMatchTemplates(matchtemplates);
    }
    matchtemplates = NULL;
    if (randomstrings) {
        FreeMemory(randomstrings);
    }
    randomstrings = NULL;
    if (synonyms) {
        FreeMemory(synonyms);
    }
    synonyms = NULL;
    if (replychats) {
        BotFreeReplyChat(replychats);
    }
    replychats = NULL;
}
