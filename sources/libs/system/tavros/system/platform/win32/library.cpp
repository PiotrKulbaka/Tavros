#include <tavros/system/library.hpp>

#include <tavros/system/platform/win32/utils.hpp>
#include <tavros/core/logger.hpp>
#include <libloaderapi.h>

static tavros::logger logger("library");

using namespace tavros::system;

library::library()
    : m_handle(nullptr)
{
}

library::library(const char* lib_name)
    : m_handle(nullptr)
{
    open(lib_name);
}

library::~library()
{
    close();
}

/* library::is_open */
bool library::is_open()
{
    return m_handle != nullptr;
}

/* library::open */
bool library::open(const char* lib_name)
{
    if (is_open()) {
        ::logger.error("The library is already open.");
        return false;
    }

    HMODULE lib = LoadLibrary(lib_name);
    if (lib == nullptr) {
        ::logger.error("Failed to load library '%s': %s", lib_name, last_win_error_str());
        return false;
    }
    m_handle = static_cast<handle>(lib);

    return true;
}

/* library::close */
void library::close()
{
    if (is_open()) {
        if (!FreeLibrary(static_cast<HMODULE>(m_handle))) {
            ::logger.error("Failed to unload library: %s", last_win_error_str());
        }
        m_handle = nullptr;
    }
}

/* library::get_symbol */
void* library::get_symbol(const char* symbol_name)
{
    FARPROC func = GetProcAddress(static_cast<HMODULE>(m_handle), symbol_name);
    if (func == nullptr) {
        ::logger.warning("Failed to load symbol '%s': %s", symbol_name, last_win_error_str());
    }
    return static_cast<void*>(func);
}
