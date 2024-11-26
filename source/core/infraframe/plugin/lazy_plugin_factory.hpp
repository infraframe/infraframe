#ifndef LAZY_SHARED_LIBRARY_PLUGIN_FACTORY_HPP
#define LAZY_SHARED_LIBRARY_PLUGIN_FACTORY_HPP

#include "shared_library.hpp"
#include "threads.hpp"
#include <filesystem>
#include <memory>

namespace owt_base {

// lazy storage of a shared library
// and the factory it contains.
// Both are loaded / created on demand
template <class FACTORY_BASE, bool THREAD_SAFE>
class lazy_plugin_factory {
public:
    using factory_base_type = FACTORY_BASE;
    lazy_plugin_factory(const lazy_plugin_factory&) = delete;
    lazy_plugin_factory& operator=(const lazy_plugin_factory&) = delete;

    lazy_plugin_factory(lazy_plugin_factory&& other) = delete;
    lazy_plugin_factory& operator=(lazy_plugin_factory&& other) = delete;

    ~lazy_plugin_factory() = default;

    inline lazy_plugin_factory(const std::filesystem::path& path);

    inline const std::filesystem::path& path() const noexcept;

    factory_base_type* factory() const;

private:
    using create_plugin_factory_type = factory_base_type* (*)();
    using mutex_type = plugin_mutex_t<THREAD_SAFE>;
    using scoped_lock_type = plugin_scoped_lock_t<THREAD_SAFE, mutex_type>;

    mutable mutex_type m_mutex;
    std::filesystem::path m_path;
    mutable std::unique_ptr<shared_library> m_library;
    mutable std::unique_ptr<factory_base_type> m_factory;
};

template <class FACTORY_BASE, bool THREAD_SAFE>
inline lazy_plugin_factory<FACTORY_BASE, THREAD_SAFE>::lazy_plugin_factory(
    const std::filesystem::path& path)
    : m_path(path)
    , m_library()
{
}

template <class FACTORY_BASE, bool THREAD_SAFE>
inline typename lazy_plugin_factory<FACTORY_BASE, THREAD_SAFE>::factory_base_type*
lazy_plugin_factory<FACTORY_BASE, THREAD_SAFE>::factory() const
{
    scoped_lock_type lock(m_mutex);
    if (!m_factory) {

        m_library.reset(new shared_library(m_path));
        m_factory.reset(m_library->find_symbol<create_plugin_factory_type>("create_plugin_factory")());
    }
    return m_factory.get();
}

template <class FACTORY_BASE, bool THREAD_SAFE>
const std::filesystem::path& lazy_plugin_factory<FACTORY_BASE, THREAD_SAFE>::path() const noexcept
{
    return m_path;
}

} // namespace owt_base

#endif // LAZY_SHARED_LIBRARY_PLUGIN_FACTORY_HPP
