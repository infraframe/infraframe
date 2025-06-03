

#ifndef OUT_PTR_H
#define OUT_PTR_H

#include <iostream>
#include <memory>

namespace owt_base {

template <typename Smart, typename Ptr>
class out_ptr_t {
public:
    explicit out_ptr_t(Smart& s) noexcept
        : _smart(s)
    {
    }

    out_ptr_t(const out_ptr_t&) = delete;
    out_ptr_t& operator=(const out_ptr_t&) = delete;

    ~out_ptr_t() noexcept { _smart.reset(_ptr); }

    operator Ptr*() noexcept { return std::addressof(_ptr); }

private:
    Smart& _smart;
    Ptr _ptr = nullptr;
};

template <typename Pointer = void, typename Smart>
inline auto out_ptr(Smart& s)
{
    if constexpr (!std::is_void_v<Pointer>) {
        return out_ptr_t<Smart, Pointer>(s);
    } else {
        return out_ptr_t<Smart, typename Smart::pointer>(s);
    }
}
} // namespace owt_base

#endif
