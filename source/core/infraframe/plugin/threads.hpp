#ifndef PLUGIN_THREADS_HPP
#define PLUGIN_THREADS_HPP

#ifdef PLUGIN_NO_THREADS
#else
#include <mutex>
#endif

// for environments without threads (like emscripten)
// we define a dummy mutex and plugin_scoped_lock
namespace owt_base {

struct plugin_mutex_dummy {
};
template <class... MutexTypes>
class plugin_scoped_lock_dummy {
public:
    plugin_scoped_lock_dummy(MutexTypes&...)
    {
    }
};

#ifdef PLUGIN_NO_THREADS
using plugin_mutex = plugin_mutex_dummy;
template <class... MutexTypes>
using plugin_scoped_lock = plugin_scoped_lock_dummy<MutexTypes...>;
#else
using plugin_mutex = std::mutex;
template <class... MutexTypes>
using plugin_scoped_lock = std::scoped_lock<MutexTypes...>;
#endif

template <bool THREAD_SAFE>
using plugin_mutex_t = std::conditional_t<THREAD_SAFE, plugin_mutex, plugin_mutex_dummy>;

template <bool THREAD_SAFE, class... MutexTypes>
using plugin_scoped_lock_t = std::conditional_t<THREAD_SAFE, plugin_scoped_lock<MutexTypes...>, plugin_scoped_lock_dummy<MutexTypes...>>;

} // namespace owt_base

#endif // PLUGIN_THREADS_HPP
