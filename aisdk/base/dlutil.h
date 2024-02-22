#pragma once

#include <iostream>
#include <string>

#if !defined(_WIN32)

#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace aisdk::base {

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define PATH_SEPARATOR ':'
#define DIRECTORY_SYMBOL '/'

// Dynamic Loading of libraries:
typedef void *LibraryHandle;
static inline LibraryHandle LibraryOpen(const std::string &path) {
    // When loading the library, we use RTLD_LAZY so that not all symbols have to be
    // resolved at this time (which improves performance). Note that if not all symbols
    // can be resolved, this could cause crashes later.
    // For experimenting/debugging: Define the LD_BIND_NOW environment variable to force all
    // symbols to be resolved here.
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
}

static inline std::string LibraryOpenError(const std::string &path) {
    (void)path;
    return std::string(dlerror());
}

static inline void LibraryClose(LibraryHandle library) { dlclose(library); }

static inline void *LibraryGetProcAddr(LibraryHandle library, const std::string &name) {
    assert(library);
    assert(!name.empty());
    return dlsym(library, name.c_str());
}

static inline std::string LibraryGetProcAddrError(const std::string &name) {
    (void)name;
    return std::string(dlerror());
}
static inline void LibraryCleanError() { dlerror(); }

}  // namespace aisdk::base

#else

#include <windows.h>

#include <cassert>
#include <sstream>

#define PATH_SEPARATOR ';'
#define DIRECTORY_SYMBOL '\\'

// Workaround for MS VS 2010/2013 missing snprintf and vsnprintf
#if defined(_MSC_VER) && _MSC_VER < 1900
#include <stdint.h>

static inline int32_t fvsnprintf(char *result_buffer, size_t buffer_size, const char *print_format,
                                 va_list varying_list) {
    int32_t copy_count = -1;
    if (buffer_size != 0) {
        copy_count = _vsnprintf_s(result_buffer, buffer_size, _TRUNCATE, print_format, varying_list);
    }
    if (copy_count == -1) {
        copy_count = _vscprintf(print_format, varying_list);
    }
    return copy_count;
}

static inline int32_t fsnprintf(char *result_buffer, size_t buffer_size, const char *print_format, ...) {
    va_list varying_list;
    va_start(varying_list, print_format);
    int32_t copy_count = fvsnprintf(result_buffer, buffer_size, print_format, varying_list);
    va_end(varying_list);
    return copy_count;
}

#define snprintf fsnprintf
#define vsnprintf fvsnprintf

#endif

namespace aisdk::base {

static inline std::wstring Utf8ToUnicode(std::string str) {
    wchar_t *wStr = NULL;
    int wLen = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
    if (wLen <= 0) {
        return L"";
    }
    wStr = new wchar_t[wLen + 1];
    memset(wStr, 0, sizeof(wchar_t) * (wLen + 1));
    wLen = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wStr, wLen);
    std::wstring strRet = wStr;
    delete[] wStr;
    return strRet;
}

static inline std::string UnicodeToUtf8(const wchar_t *lpwcszWString) {
    char *pElementText;
    int iTextLen = ::WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)lpwcszWString, -1, NULL, 0, NULL, NULL);
    pElementText = new char[iTextLen + 1];
    memset((void *)pElementText, 0, (iTextLen + 1) * sizeof(char));
    ::WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)lpwcszWString, -1, pElementText, iTextLen, NULL, NULL);
    std::string strReturn(pElementText);
    delete[] pElementText;
    return strReturn;
}

static inline std::string DescribeError(uint32_t code, bool prefixErrorCode = true) {
    std::string str;

    if (prefixErrorCode) {
        char prefixBuffer[64];
        snprintf(prefixBuffer, sizeof(prefixBuffer), "0x%llx (%lld): ", (uint64_t)code, (int64_t)code);
        str = prefixBuffer;
    }

    // Could use FORMAT_MESSAGE_FROM_HMODULE to specify an error source.
    WCHAR errorBufferW[1024]{};
    const DWORD errorBufferWCapacity = sizeof(errorBufferW) / sizeof(errorBufferW[0]);
    const DWORD length =
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, (DWORD)code,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorBufferW, errorBufferWCapacity, nullptr);

    if (length) {  // If errorBufferW contains what we are looking for...
        str += UnicodeToUtf8(errorBufferW);
    } else {
        str = "(unknown)";
    }

    return str;
}

// Dynamic Loading:
typedef HMODULE LibraryHandle;
static inline LibraryHandle LibraryOpen(const std::string &path) {
    const std::wstring pathW = Utf8ToUnicode(path);

    std::wcout << pathW << std::endl;
    // Try loading the library the original way first.
    LibraryHandle handle = LoadLibraryW(pathW.c_str());
    if (handle == NULL && GetLastError() == ERROR_MOD_NOT_FOUND) {
        const DWORD dwAttrib = GetFileAttributesW(pathW.c_str());
        const bool fileExists = (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
        if (fileExists) {
            // If that failed, then try loading it with broader search folders.
            handle = LoadLibraryExW(pathW.c_str(), NULL,
                                    LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
        }
    }

    return handle;
}

static inline std::string LibraryOpenError(const std::string &path) {
    std::stringstream ss;
    const DWORD dwLastError = GetLastError();
    const std::string strError = DescribeError(dwLastError);
    ss << "Failed to open dynamic library " << path << " with error " << dwLastError << ": " << strError;
    return ss.str();
}

static inline void LibraryClose(LibraryHandle library) { FreeLibrary(library); }

static inline void *LibraryGetProcAddr(LibraryHandle library, const std::string &name) {
    assert(library);
    assert(name.size() > 0);
    return reinterpret_cast<void *>(GetProcAddress(library, name.c_str()));
}

static inline std::string LibraryGetProcAddrError(const std::string &name) {
    std::stringstream ss;
    ss << "Failed to find function " << name << " in dynamic library";
    return ss.str();
}

static inline void LibraryCleanError() { SetLastError(0); }

}  // namespace aisdk::base

#endif
