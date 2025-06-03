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
#include "tr_local.h"

#include <stb/stb_image.h>

#include <thread>

static tavros::core::logger logger("tr_image");

static std::vector<texture_type>                                   s_storage;
static std::unordered_map<tavros::core::string_view, texture_type> s_textures;

constexpr size_t max_textures = 1024 * 2;

void set_texture_sampler(texture_type tex, int32_t min_filter, int32_t mag_filter, int32_t wrap)
{
    qglBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(tex));
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    qglBindTexture(GL_TEXTURE_2D, 0);
}

void init_texture_manager()
{
    s_storage.reserve(max_textures);
    s_textures.reserve(max_textures);
}

void clear_textures()
{
    s_storage.clear();
    s_textures.clear();
}

texture_type create_texture(tavros::core::string_view name, int32_t width, int32_t height, const uint8_t* pixels)
{
    if (s_storage.size() >= max_textures) {
        ::logger.error("max_textures hit %s", name.data());
        return 0;
    }

    auto t = s_textures.find(name);
    if (t != s_textures.end()) {
        auto htex = static_cast<GLuint>(t->second);
        qglDeleteTextures(1, &htex);
    }

    GLuint texid = 0;
    qglGenTextures(1, &texid);
    qglBindTexture(GL_TEXTURE_2D, texid);
    qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    qglBindTexture(GL_TEXTURE_2D, 0);

    texture_type ref = 0;
    if (t != s_textures.end()) {
        t->second = texid;
        ref = t->second;
    }
    else {
        s_storage.push_back(texid);
        ref = s_storage.back();
    }
    s_textures.emplace(std::make_pair(name, ref));

    return ref;
}

texture_type find_texture(tavros::core::string_view name)
{
    if (auto f = s_textures.find(tavros::core::string(name)); f != s_textures.end()) {
        return f->second;
    }
    return 0;
}
// texture_manager::load
texture_type load_texture(tavros::core::string_view name, tavros::core::string_view path)
{
    if (auto r = find_texture(name); r) {
        return r;
    }

    uint8_t* buf;
    int32_t  len = FS_ReadFile(path.data(), (void**)&buf);
    if (len == -1) {
        return 0;
    }

    int32_t  w, h, c;
    uint8_t* pixels = stbi_load_from_memory(buf, len, &w, &h, &c, STBI_rgb_alpha);
    auto     tex = create_texture(path, w, h, pixels);
    stbi_image_free(pixels);

    FS_FreeFile(buf);

    return tex;
}


void generate_texture_mipmaps(texture_type tex)
{
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(tex));
    qglGenerateMipmap(GL_TEXTURE_2D);

    // TODO: change to manual generation, mipmap generation does not work, fixed by waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(0));

    glBindTexture(GL_TEXTURE_2D, 0);
}

/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
texture_type R_FindImageFile(const char* name, bool mipmap, int32 glWrapClampMode)
{
    if (auto ref = s_textures.find(name); ref != s_textures.end()) {
        return ref->second;
    }

    auto tex = load_texture(name, name);
    if (!tex) {
        char altname[MAX_QPATH];
        auto len = strlen(name);
        strncpy(altname, name, len);
        altname[len - 3] = 'j';
        altname[len - 2] = 'p';
        altname[len - 1] = 'g';
        altname[len - 0] = 0;
        tex = load_texture(altname, altname);
    }

    if (!tex) {
        return 0;
    }

    if (mipmap) {
        generate_texture_mipmaps(tex);
        set_texture_sampler(tex, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, glWrapClampMode);
    } else {
        set_texture_sampler(tex, GL_LINEAR, GL_LINEAR, glWrapClampMode);
    }

    return tex;
}


/*
================
R_CreateDlightImage
================
*/
#define DLIGHT_SIZE 16
static void R_CreateDlightImage()
{
    int32 x, y;
    uint8 data[DLIGHT_SIZE][DLIGHT_SIZE][4];
    int32 b;

    // make a centered inverse-square falloff blob for dynamic lighting
    for (x = 0; x < DLIGHT_SIZE; x++) {
        for (y = 0; y < DLIGHT_SIZE; y++) {
            float d;

            d = (DLIGHT_SIZE / 2 - 0.5f - x) * (DLIGHT_SIZE / 2 - 0.5f - x) + (DLIGHT_SIZE / 2 - 0.5f - y) * (DLIGHT_SIZE / 2 - 0.5f - y);
            b = 4000 / d;
            if (b > 255) {
                b = 255;
            } else if (b < 75) {
                b = 0;
            }
            data[y][x][0] = data[y][x][1] = data[y][x][2] = b;
            data[y][x][3] = 255;
        }
    }
    auto sz = DLIGHT_SIZE;
    tr.dlightImage = create_texture("*dlight", sz, sz, (uint8*) data);
    set_texture_sampler(tr.dlightImage, GL_LINEAR, GL_LINEAR, GL_CLAMP);
}


/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable()
{
    int32 i;
    float d;
    float exp;

    exp = 0.5;

    for (i = 0; i < FOG_TABLE_SIZE; i++) {
        d = pow((float) i / (FOG_TABLE_SIZE - 1), exp);

        tr.fogTable[i] = d;
    }
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float R_FogFactor(float s, float t)
{
    float d;

    s -= 1.0 / 512;
    if (s < 0) {
        return 0;
    }
    if (t < 1.0 / 32) {
        return 0;
    }
    if (t < 31.0 / 32) {
        s *= (t - 1.0f / 32.0f) / (30.0f / 32.0f);
    }

    // we need to leave a lot of clamp range
    s *= 8;

    if (s > 1.0) {
        s = 1.0;
    }

    d = tr.fogTable[(int32) (s * (FOG_TABLE_SIZE - 1))];

    return d;
}

/*
================
R_CreateFogImage
================
*/
#define FOG_S 256
#define FOG_T 32
static void R_CreateFogImage()
{
    int32  x, y;
    uint8* data;
    float  g;
    float  d;
    float  borderColor[4];

    data = (uint8*) Hunk_AllocateTempMemory(FOG_S * FOG_T * 4);

    g = 2.0;

    // S is distance, T is depth
    for (x = 0; x < FOG_S; x++) {
        for (y = 0; y < FOG_T; y++) {
            d = R_FogFactor((x + 0.5f) / FOG_S, (y + 0.5f) / FOG_T);

            data[(y * FOG_S + x) * 4 + 0] =
                data[(y * FOG_S + x) * 4 + 1] =
                    data[(y * FOG_S + x) * 4 + 2] = 255;
            data[(y * FOG_S + x) * 4 + 3] = 255 * d;
        }
    }
    // standard openGL clamping doesn't really do what we want -- it includes
    // the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
    // what we want.
    tr.fogImage = create_texture("*fog", FOG_S, FOG_T, (uint8*) data);
    set_texture_sampler(tr.fogImage, GL_LINEAR, GL_LINEAR, GL_CLAMP);
    Hunk_FreeTempMemory(data);

    borderColor[0] = 1.0;
    borderColor[1] = 1.0;
    borderColor[2] = 1.0;
    borderColor[3] = 1;

    qglTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
}

/*
==================
R_CreateDefaultImage
==================
*/
#define DEFAULT_SIZE 16
static void R_CreateDefaultImage()
{
    int32 x;
    uint8 data[DEFAULT_SIZE][DEFAULT_SIZE][4];

    // the default image will be a box, to allow you to see the mapping coordinates
    Com_Memset(data, 32, sizeof(data));
    for (x = 0; x < DEFAULT_SIZE; x++) {
        data[0][x][0] =
            data[0][x][1] =
                data[0][x][2] =
                    data[0][x][3] = 255;

        data[x][0][0] =
            data[x][0][1] =
                data[x][0][2] =
                    data[x][0][3] = 255;

        data[DEFAULT_SIZE - 1][x][0] =
            data[DEFAULT_SIZE - 1][x][1] =
                data[DEFAULT_SIZE - 1][x][2] =
                    data[DEFAULT_SIZE - 1][x][3] = 255;

        data[x][DEFAULT_SIZE - 1][0] =
            data[x][DEFAULT_SIZE - 1][1] =
                data[x][DEFAULT_SIZE - 1][2] =
                    data[x][DEFAULT_SIZE - 1][3] = 255;
    }
    auto sz = DEFAULT_SIZE;
    tr.defaultImage = create_texture("*default", sz, sz, (uint8*) data);
    set_texture_sampler(tr.defaultImage, GL_LINEAR, GL_LINEAR, GL_REPEAT);
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages()
{
    int32 x, y;
    uint8 data[DEFAULT_SIZE][DEFAULT_SIZE][4];

    R_CreateDefaultImage();

    // we use a solid white image instead of disabling texturing
    Com_Memset(data, 255, sizeof(data));
    create_texture("*white", 8, 8, (uint8*) data);
    set_texture_sampler(tr.whiteImage, GL_LINEAR, GL_LINEAR, GL_REPEAT);

    // with overbright bits active, we need an image which is some fraction of full color,
    // for default lightmaps, etc
    for (x = 0; x < DEFAULT_SIZE; x++) {
        for (y = 0; y < DEFAULT_SIZE; y++) {
            data[y][x][0] = data[y][x][1] = data[y][x][2] = data[y][x][3] = 255;
        }
    }

    R_CreateDlightImage();
    R_CreateFogImage();
}

/*
===============
R_InitImages
===============
*/
void R_InitImages()
{
    clear_textures();
    // create default texture and white texture
    R_CreateBuiltinImages();
}

/*
===============
R_DeleteTextures
===============
*/
void R_DeleteTextures()
{
    clear_textures();
}

/*
============================================================================

SKINS

============================================================================
*/

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static const char* CommaParse(char** data_p)
{
    int32       c = 0, len;
    char*       data;
    static char com_token[MAX_TOKEN_CHARS];

    data = *data_p;
    len = 0;
    com_token[0] = 0;

    // make sure incoming data is valid
    if (!data) {
        *data_p = NULL;
        return com_token;
    }

    while (1) {
        // skip whitespace
        while ((c = *data) <= ' ') {
            if (!c) {
                break;
            }
            data++;
        }


        c = *data;

        // skip double slash comments
        if (c == '/' && data[1] == '/') {
            while (*data && *data != '\n') {
                data++;
            }
        }
        // skip /* */ comments
        else if (c == '/' && data[1] == '*') {
            while (*data && (*data != '*' || data[1] != '/')) {
                data++;
            }
            if (*data) {
                data += 2;
            }
        } else {
            break;
        }
    }

    if (c == 0) {
        return "";
    }

    // handle quoted strings
    if (c == '\"') {
        data++;
        while (1) {
            c = *data++;
            if (c == '\"' || !c) {
                com_token[len] = 0;
                *data_p = (char*) data;
                return com_token;
            }
            if (len < MAX_TOKEN_CHARS) {
                com_token[len] = c;
                len++;
            }
        }
    }

    // parse a regular word
    do {
        if (len < MAX_TOKEN_CHARS) {
            com_token[len] = c;
            len++;
        }
        data++;
        c = *data;
    } while (c > 32 && c != ',');

    if (len == MAX_TOKEN_CHARS) {
        len = 0;
    }
    com_token[len] = 0;

    *data_p = (char*) data;
    return com_token;
}


/*
===============
RE_RegisterSkin

===============
*/
qhandle_t RE_RegisterSkin(const char* name)
{
    qhandle_t      hSkin;
    skin_t*        skin;
    skinSurface_t* surf;
    char *         text, *text_p;
    char*          token;
    char           surfName[MAX_QPATH];

    if (!name || !name[0]) {
        logger.info("Empty name passed to RE_RegisterSkin");
        return 0;
    }

    if (strlen(name) >= MAX_QPATH) {
        logger.info("Skin name exceeds MAX_QPATH");
        return 0;
    }


    // see if the skin is already loaded
    for (hSkin = 1; hSkin < tr.numSkins; hSkin++) {
        skin = tr.skins[hSkin];
        if (!Q_stricmp(skin->name, name)) {
            if (skin->numSurfaces == 0) {
                return 0; // default skin
            }
            return hSkin;
        }
    }

    // allocate a new skin
    if (tr.numSkins == MAX_SKINS) {
        logger.warning("RE_RegisterSkin('%s') MAX_SKINS hit", name);
        return 0;
    }
    tr.numSkins++;
    skin = (skin_t*) Hunk_Alloc(sizeof(skin_t), h_low);
    tr.skins[hSkin] = skin;
    Q_strncpyz(skin->name, name, sizeof(skin->name));
    skin->numSurfaces = 0;

    // If not a .skin file, load as a single shader
    if (strcmp(name + strlen(name) - 5, ".skin")) {
        skin->numSurfaces = 1;
        skin->surfaces[0] = (skinSurface_t*) Hunk_Alloc(sizeof(skin->surfaces[0]), h_low);
        skin->surfaces[0]->shader = R_FindShader(name, LIGHTMAP_NONE, true);
        return hSkin;
    }

    // load and parse the skin file
    FS_ReadFile(name, (void**) &text);
    if (!text) {
        return 0;
    }

    text_p = text;
    while (text_p && *text_p) {
        // get surface name
        token = (char*) CommaParse(&text_p);
        Q_strncpyz(surfName, token, sizeof(surfName));

        if (!token[0]) {
            break;
        }
        // lowercase the surface name so skin compares are faster
        Q_strlwr(surfName);

        if (*text_p == ',') {
            text_p++;
        }

        if (strstr(token, "tag_")) {
            continue;
        }

        // parse the shader name
        token = (char*) CommaParse(&text_p);

        surf = skin->surfaces[skin->numSurfaces] = (skinSurface_t*) Hunk_Alloc(sizeof(*skin->surfaces[0]), h_low);
        Q_strncpyz(surf->name, surfName, sizeof(surf->name));
        surf->shader = R_FindShader(token, LIGHTMAP_NONE, true);
        skin->numSurfaces++;
    }

    FS_FreeFile(text);


    // never let a skin have 0 shaders
    if (skin->numSurfaces == 0) {
        return 0; // use default skin
    }

    return hSkin;
}


/*
===============
R_InitSkins
===============
*/
void R_InitSkins()
{
    skin_t* skin;

    tr.numSkins = 1;

    // make the default skin have all default shaders
    skin = tr.skins[0] = (skin_t*) Hunk_Alloc(sizeof(skin_t), h_low);
    Q_strncpyz(skin->name, "<default skin>", sizeof(skin->name));
    skin->numSurfaces = 1;
    skin->surfaces[0] = (skinSurface_t*) Hunk_Alloc(sizeof(*skin->surfaces), h_low);
    skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t* R_GetSkinByHandle(qhandle_t hSkin)
{
    if (hSkin < 1 || hSkin >= tr.numSkins) {
        return tr.skins[0];
    }
    return tr.skins[hSkin];
}
