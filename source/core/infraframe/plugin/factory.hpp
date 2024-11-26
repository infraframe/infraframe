#ifndef PLUGIN_FACTORY_HPP
#define PLUGIN_FACTORY_HPP

#include "config.hpp"

#include <memory>

#define CREATE_PLUGIN_FACTORY(FACTORY_TYPE)                                                 \
    extern "C" PLUGIN_API typename FACTORY_TYPE::factory_base_type* create_plugin_factory() \
    {                                                                                       \
        return new FACTORY_TYPE();                                                          \
    }

namespace owt_base {

template <class BASE_TYPE, class... ARGS>
class plugin_factory_base {
public:
    using base_type = BASE_TYPE;
    virtual ~plugin_factory_base() = default;
    virtual std::unique_ptr<base_type> create(ARGS...) = 0;
};

// default implementation of the factory
template <class CONCRETE_TYPE, class BASE_TYPE, class... ARGS>
class factory : public plugin_factory_base<BASE_TYPE, ARGS...> {
public:
    using concrete_type = CONCRETE_TYPE;
    using base_type = BASE_TYPE;
    using factory_base_type = plugin_factory_base<BASE_TYPE, ARGS...>;
    virtual ~factory() = default;
    std::unique_ptr<base_type> create(ARGS... args) override;
};

template <class CONCRETE_TYPE, class BASE_TYPE, class... ARGS>
auto factory<CONCRETE_TYPE, BASE_TYPE, ARGS...>::create(ARGS... args) -> std::unique_ptr<base_type>
{
    return std::make_unique<concrete_type>(args...);
}

} // namespace owt_base

#endif // PLUGIN_FACTORY_HPP
