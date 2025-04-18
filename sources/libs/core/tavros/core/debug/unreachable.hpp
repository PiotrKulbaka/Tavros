#pragma once

[[noreturn]] void tav_unreachable_impl(const char* file, int line, const char* function);
#define TAV_UNREACHABLE() tav_unreachable_impl(__FILE__, __LINE__, __func__)
