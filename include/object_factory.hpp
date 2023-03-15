#ifndef OBJECT_FACTORY_HPP
#define OBJECT_FACTORY_HPP

#include "object.hpp"

#include <ether/ether.hpp>

namespace mw {


object*
build_object(const eth::value &objdata);


} // namespace mw

#endif
