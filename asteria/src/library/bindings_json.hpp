// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_JSON_HPP_
#define ASTERIA_LIBRARY_BINDINGS_JSON_HPP_

#include "../fwd.hpp"

namespace Asteria {

Sval std_json_format(Value value, Sopt indent = nullopt);
Sval std_json_format(Value value, Ival indent);
Value std_json_parse(Sval text);

// Create an object that is to be referenced as `std.json`.
void create_bindings_json(Oval& result, API_Version version);

}  // namespace Asteria

#endif
