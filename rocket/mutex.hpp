// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_MUTEX_
#define ROCKET_MUTEX_

#include "fwd.hpp"
#include "assert.hpp"
#include <pthread.h>

namespace rocket {

class mutex;
class condition_variable;

#include "details/mutex.ipp"

class mutex
  {
  public:
    class unique_lock;

  private:
    ::pthread_mutex_t m_mutex[1] = { PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP };

  public:
    constexpr
    mutex() noexcept
      { }

    mutex(const mutex&)
      = delete;

    mutex&
    operator=(const mutex&)
      = delete;

    ~mutex()
      {
        int r = ::pthread_mutex_destroy(this->m_mutex);
        ROCKET_ASSERT(r == 0);
      }
  };

class mutex::unique_lock
  {
    friend condition_variable;

  private:
    details_mutex::stored_pointer m_sth;

  public:
    constexpr
    unique_lock() noexcept
      { }

    explicit
    unique_lock(mutex& parent) noexcept
      {
        this->lock(parent);
      }

    unique_lock(unique_lock&& other) noexcept
      {
        this->m_sth.exchange_with(other.m_sth);
      }

    unique_lock&
    operator=(unique_lock&& other) noexcept
      {
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }

    unique_lock&
    swap(unique_lock& other) noexcept
      {
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }

    ~unique_lock()
      {
        this->unlock();
      }

  public:
    explicit operator
    bool() const noexcept
      { return this->m_sth.get() != nullptr;  }

    bool
    is_locking(const mutex& m) const noexcept
      { return this->m_sth.get() == m.m_mutex;  }

    bool
    is_locking(const mutex&&) const noexcept
      = delete;

    unique_lock&
    unlock() noexcept
      {
        this->m_sth.reset(nullptr);
        return *this;
      }

    unique_lock&
    try_lock(mutex& m) noexcept
      {
        // Return immediately if the same mutex is already held.
        // Otherwise deadlocks would occur.
        auto ptr = m.m_mutex;
        if(ROCKET_UNEXPECT(ptr == this->m_sth.get()))
          return *this;

        // There shall be no gap between the unlock and lock operations.
        // If the mutex cannot be locked, there is no effect.
        int r = ::pthread_mutex_trylock(ptr);
        ROCKET_ASSERT(r != EINVAL);
        if(r != 0)
          return *this;

        this->m_sth.reset(ptr);
        return *this;
      }

    unique_lock&
    lock(mutex& m) noexcept
      {
        // Return immediately if the same mutex is already held.
        // Otherwise deadlocks would occur.
        auto ptr = m.m_mutex;
        if(ROCKET_UNEXPECT(ptr == this->m_sth.get()))
          return *this;

        // There shall be no gap between the unlock and lock operations.
        int r = ::pthread_mutex_lock(ptr);
        ROCKET_ASSERT(r == 0);

        this->m_sth.reset(ptr);
        return *this;
      }
  };

inline
void
swap(mutex::unique_lock& lhs, mutex::unique_lock& rhs) noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
