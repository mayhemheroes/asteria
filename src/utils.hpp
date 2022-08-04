// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILS_
#define ASTERIA_UTILS_

#include "fwd.hpp"
#include "../rocket/tinyfmt_str.hpp"
#include "../rocket/format.hpp"
#include "details/utils.ipp"
#include <cmath>

namespace asteria {

// Formatting
template<typename... TemplT>
constexpr
array<cow_string::shallow_type, sizeof...(TemplT)>
make_string_template(const TemplT&... templs)
  {
    return { ::rocket::sref(templs)... };
  }

template<size_t N, typename... ParamsT>
ROCKET_NEVER_INLINE ROCKET_FLATTEN
cow_string
format_string(const array<cow_string::shallow_type, N>& templs, const ParamsT&... params)
  {
    // Make stream inserters.
    ::rocket::tinyfmt_str fmt;
    ::rocket::formatter insts[] = { ::rocket::make_default_formatter(fmt, params)..., { } };

    // Write all strings, seaprated by line feeds.
    if(N != 0) {
      ::rocket::vformat(fmt, templs[0].c_str(), templs[0].size(), insts, sizeof...(params));

      for(size_t k = 1;  k != N;  ++k)
        fmt << '\n',
          ::rocket::vformat(fmt, templs[k].c_str(), templs[k].size(), insts, sizeof...(params));
    }
    return fmt.extract_string();
  }

template<typename... ParamsT>
ROCKET_NEVER_INLINE ROCKET_FLATTEN
cow_string
format_string(const char* templ, const ParamsT&... params)
  {
    // Make stream inserters.
    ::rocket::tinyfmt_str fmt;
    ::rocket::formatter insts[] = { ::rocket::make_default_formatter(fmt, params)..., { } };

    // Write the string as is.
    ::rocket::vformat(fmt, templ, ::strlen(templ), insts, sizeof...(params));
    return fmt.extract_string();
  }

template<typename... ParamsT>
ROCKET_NEVER_INLINE ROCKET_FLATTEN
cow_string
format_string(const cow_string& templ, const ParamsT&... params)
  {
    // Make stream inserters.
    ::rocket::tinyfmt_str fmt;
    ::rocket::formatter insts[] = { ::rocket::make_default_formatter(fmt, params)..., { } };

    // Write the string as is.
    ::rocket::vformat(fmt, templ.c_str(), templ.length(), insts, sizeof...(params));
    return fmt.extract_string();
  }

// Error handling
// Note string templates must be parenthesized.
ptrdiff_t
write_log_to_stderr(const char* file, long line, const char* func, cow_string&& msg);

[[noreturn]]
void
throw_runtime_error(const char* file, long line, const char* func, cow_string&& msg);

#define ASTERIA_TERMINATE(TEMPLATE, ...)  \
    (::asteria::write_log_to_stderr(__FILE__, __LINE__, __func__,  \
       ::asteria::format_string(  \
         (::asteria::make_string_template TEMPLATE), ##__VA_ARGS__)  \
       ),  \
     ::std::terminate())

#define ASTERIA_THROW(TEMPLATE, ...)  \
    (::asteria::throw_runtime_error(__FILE__, __LINE__, __func__,  \
       ::asteria::format_string(  \
         (::asteria::make_string_template TEMPLATE), ##__VA_ARGS__)  \
       ),  \
     __builtin_unreachable())

// UTF-8 conversion functions
bool
utf8_encode(char*& pos, char32_t cp) noexcept;

bool
utf8_encode(cow_string& text, char32_t cp);

bool
utf8_decode(char32_t& cp, const char*& pos, size_t avail) noexcept;

bool
utf8_decode(char32_t& cp, const cow_string& text, size_t& offset);

// UTF-16 conversion functions
bool
utf16_encode(char16_t*& pos, char32_t cp) noexcept;

bool
utf16_encode(cow_u16string& text, char32_t cp);

bool
utf16_decode(char32_t& cp, const char16_t*& pos, size_t avail) noexcept;

bool
utf16_decode(char32_t& cp, const cow_u16string& text, size_t& offset);

// Type conversion
template<typename enumT>
ROCKET_CONST constexpr
typename ::std::underlying_type<enumT>::type
weaken_enum(enumT value) noexcept
  { return static_cast<typename ::std::underlying_type<enumT>::type>(value);  }

// Saturation subtraction
template<typename uintT,
ROCKET_ENABLE_IF(::std::is_unsigned<uintT>::value && !::std::is_same<uintT, bool>::value)>
constexpr
uintT
subsat(uintT x, uintT y) noexcept
  { return (x < y) ? 0 : (x - y);  }

// C character types
enum : uint8_t
  {
    cmask_space   = 0x01,  // [ \t\v\f\r\n]
    cmask_alpha   = 0x02,  // [A-Za-z]
    cmask_digit   = 0x04,  // [0-9]
    cmask_xdigit  = 0x08,  // [0-9A-Fa-f]
    cmask_namei   = 0x10,  // [A-Za-z_]
    cmask_blank   = 0x20,  // [ \t]
    cmask_cntrl   = 0x40,  // [[:cntrl:]]
  };

ROCKET_CONST inline
uint8_t
get_cmask(char ch) noexcept
  { return ((ch & 0x7F) == ch) ? details_utils::cmask_table[(uint8_t)ch] : 0;  }

ROCKET_CONST inline
bool
is_cmask(char ch, uint8_t mask) noexcept
  { return noadl::get_cmask(ch) & mask;  }

// Numeric conversion
ROCKET_CONST inline
bool
is_convertible_to_int64(double val) noexcept
  { return (-0x1p63 <= val) && (val < 0x1p63);  }

ROCKET_CONST inline
bool
is_exact_int64(double val) noexcept
  { return noadl::is_convertible_to_int64(val) && (::std::trunc(val) == val);  }

inline
int64_t
safe_double_to_int64(double val)
  {
    double fval = ::std::trunc(val);
    if(fval != val)
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
            "safe_double_to_int64: value `%.17g` is not an exact integer",
            val);

    if(!noadl::is_convertible_to_int64(val))
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
            "safe_double_to_int64: value `%.17g` is out of range for an `int64`",
            val);

    return static_cast<int64_t>(val);
  }

// C-style quoting
constexpr
details_utils::Quote_Wrapper
quote(const char* str, size_t len) noexcept
  { return { str, len };  }

inline
details_utils::Quote_Wrapper
quote(const char* str) noexcept
  { return noadl::quote(str, ::std::strlen(str));  }

inline
details_utils::Quote_Wrapper
quote(const cow_string& str) noexcept
  { return noadl::quote(str.data(), str.size());  }

// Justifying
constexpr
details_utils::Paragraph_Wrapper
pwrap(size_t indent, size_t hanging) noexcept
  { return { indent, hanging };  }

// Error numbers
constexpr
details_utils::Formatted_errno
format_errno(int err = errno) noexcept
  { return { err };  }

// Negative array index wrapper
struct Wrapped_Index
  {
    uint64_t nprepend;  // number of elements to prepend
    uint64_t nappend;   // number of elements to append
    size_t rindex;      // the wrapped index (valid if both `nprepend` and `nappend` are 0s)
  };

ROCKET_CONST
Wrapped_Index
wrap_index(int64_t index, size_t size) noexcept;

// Note that all bits in the result are filled.
uint64_t
generate_random_seed() noexcept;

}  // namespace asteria

#endif
