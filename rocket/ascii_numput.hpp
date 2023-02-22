// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASCII_NUMPUT_
#define ROCKET_ASCII_NUMPUT_

#include "fwd.hpp"
namespace rocket {

class ascii_numput
  {
  private:
    // Configuration
    char m_rdxp = '.';

    // This storage must be sufficient for the longest result, which
    // at the moment is signed 64-bit integer in binary (`-0b111...1`
    // takes 68 bytes along with the null terminator).
    char m_stor[71];

    // These pointers may point to static, immutable storage.
    const char* m_data;
    uint32_t m_size;

  public:
    // Initializes an empty string.
    ascii_numput() noexcept
      {
        this->clear();
      }

    ascii_numput(const ascii_numput&) = delete;

    ascii_numput&
    operator=(const ascii_numput&) = delete;

  public:
    // accessors
    const char*
    begin() const noexcept
      { return this->m_data;  }

    const char*
    end() const noexcept
      { return this->m_data + this->m_size;  }

    bool
    empty() const noexcept
      { return this->m_size == 0;  }

    size_t
    size() const noexcept
      { return this->m_size;  }

    const char*
    data() const noexcept
      { return this->m_data;  }

    ascii_numput&
    clear() noexcept
      {
        this->m_data = this->m_stor;
        this->m_size = 0;
        this->m_stor[0] = 0;
        return *this;
      }

    // Gets and sets the radix point.
    char
    radix_point() const noexcept
      { return this->m_rdxp;  }

    ascii_numput&
    set_radix_point(char rdxp) noexcept
      {
        this->m_rdxp = rdxp;
        return *this;
      }

    // * boolean as `true` or `false`
    ascii_numput&
    put_TB(bool value) noexcept;

    // * pointer as an unsigned integer in hexadecimal
    ascii_numput&
    put_XP(const volatile void* value) noexcept;

    // * unsigned 64-bit integer in binary
    ascii_numput&
    put_BU(uint64_t value, uint32_t precision = 1) noexcept;

    // * unsigned 64-bit integer in hexadecimal
    ascii_numput&
    put_XU(uint64_t value, uint32_t precision = 1) noexcept;

    // * unsigned 64-bit integer in decimal
    ascii_numput&
    put_DU(uint64_t value, uint32_t precision = 1) noexcept;

    // * signed 64-bit integer in binary
    ascii_numput&
    put_BI(int64_t value, uint32_t precision = 1) noexcept;

    // * signed 64-bit integer in hexadecimal
    ascii_numput&
    put_XI(int64_t value, uint32_t precision = 1) noexcept;

    // * signed 64-bit integer in decimal
    ascii_numput&
    put_DI(int64_t value, uint32_t precision = 1) noexcept;

    // * IEEE-754 single-precision floating-point in binary
    ascii_numput&
    put_BF(float value) noexcept;

    // * IEEE-754 single-precision floating-point in binary scientific notation
    ascii_numput&
    put_BEF(float value) noexcept;

    // * IEEE-754 single-precision floating-point in hexadecimal
    ascii_numput&
    put_XF(float value) noexcept;

    // * IEEE-754 single-precision floating-point in hexadecimal scientific notation
    ascii_numput&
    put_XEF(float value) noexcept;

    // * IEEE-754 single-precision floating-point in decimal
    ascii_numput&
    put_DF(float value) noexcept;

    // * IEEE-754 single-precision floating-point in decimal scientific notation
    ascii_numput&
    put_DEF(float value) noexcept;

    // * IEEE-754 single-precision floating-point in binary
    ascii_numput&
    put_BD(double value) noexcept;

    // * IEEE-754 double-precision floating-point in binary scientific notation
    ascii_numput&
    put_BED(double value) noexcept;

    // * IEEE-754 double-precision floating-point in hexadecimal
    ascii_numput&
    put_XD(double value) noexcept;

    // * IEEE-754 double-precision floating-point in hexadecimal scientific notation
    ascii_numput&
    put_XED(double value) noexcept;

    // * IEEE-754 double-precision floating-point in decimal
    ascii_numput&
    put_DD(double value) noexcept;

    // * IEEE-754 double-precision floating-point in decimal scientific notation
    ascii_numput&
    put_DED(double value) noexcept;

    // These are easy functions that delegate to those above, passing
    // their default arguments. These functions are designed to produce
    // lossless outputs.
    ascii_numput&
    put(bool value) noexcept
      {
        this->put_TB(value);
        return *this;
      }

    ascii_numput&
    put(const volatile void* value) noexcept
      {
        this->put_XP(value);
        return *this;
      }

    template<typename valueT,
    ROCKET_ENABLE_IF(is_integral<valueT>::value && is_unsigned<valueT>::value)>
    ascii_numput&
    put(valueT value) noexcept
      {
        this->put_DU(value);
        return *this;
      }

    template<typename valueT,
    ROCKET_ENABLE_IF(is_integral<valueT>::value && is_signed<valueT>::value)>
    ascii_numput&
    put(valueT value) noexcept
      {
        this->put_DI(value);
        return *this;
      }

    ascii_numput&
    put(float value) noexcept
      {
        this->put_DF(value);
        return *this;
      }

    ascii_numput&
    put(double value) noexcept
      {
        this->put_DD(value);
        return *this;
      }
  };

}  // namespace rocket
#endif
