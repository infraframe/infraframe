#ifndef PLUGIN_SHARED_LIBRARY_HPP
#define PLUGIN_SHARED_LIBRARY_HPP

// std includes
#include <string>
// path
#include <filesystem>

// includes from plugin
#include "config.hpp"

#include <dlfcn.h>
#include <iostream>

namespace owt_base {
class shared_library {
public:
    shared_library(const shared_library&) = delete;
    shared_library& operator=(const shared_library&) = delete;

    inline shared_library(shared_library&& other) noexcept;
    inline shared_library& operator=(shared_library&& other) noexcept;

    inline shared_library(std::filesystem::path path);
    inline ~shared_library();

    template <class T>
    inline T find_symbol(const std::string& name);

private:
    void* m_handle;
};

shared_library::shared_library(shared_library&& other) noexcept
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
}
shared_library& shared_library::operator=(shared_library&& other) noexcept
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    return *this;
}

shared_library::shared_library(std::filesystem::path path)
    : m_handle(nullptr)
{
    m_handle = dlopen(path.string().c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!m_handle) {
        throw std::runtime_error("could not open shared library from path: " + path.string() + " error: " + dlerror());
    }
}

template <class T>
T shared_library::find_symbol(const std::string& name)
{
    dlerror(); /* Clear any existing error */
    char* error;
    void* sym = dlsym(m_handle, name.c_str());
    if (sym == nullptr) {
        std::cout << "could not find symbol: " << name << "sym" << sym << std::endl;
    }
    if ((error = dlerror()) != NULL) {
        throw std::runtime_error("could not find symbol: " + name + " error: " + error);
    }
    return reinterpret_cast<T>(sym);
}

shared_library::~shared_library()
{
    if (m_handle) {
        dlclose(m_handle);
    }
}

} // namespace owt_base

#endif // PLUGIN_SHARED_LIBRARY_HPP
