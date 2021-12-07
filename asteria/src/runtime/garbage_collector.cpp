// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "garbage_collector.hpp"
#include "variable.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

class Sentry
  {
  private:
    long* m_ptr;
    long m_old;

  public:
    explicit
    Sentry(long& ref) noexcept
      : m_ptr(&ref), m_old(ref)
      { ++*(this->m_ptr);  }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(Sentry)
      { --*(this->m_ptr);  }

  public:
    explicit operator
    bool() const noexcept
      { return this->m_old == 0;  }
  };

}  // namespace

Garbage_Collector::
~Garbage_Collector()
  {
  }

size_t
Garbage_Collector::
do_collect_generation(size_t gen)
  {
    // Ignore recursive requests.
    const Sentry sentry(this->m_recur);
    if(!sentry)
      return 0;

    size_t nvars = 0;
    rcptr<Variable> var;

    auto& tracked = this->m_tracked.mut(gMax-gen);
    const auto next_opt = this->m_tracked.mut_ptr(gMax-gen-1);
    const auto count_opt = this->m_counts.mut_ptr(gMax-gen-1);

    this->m_staged.clear();
    this->m_temp_1.clear();
    this->m_temp_2.clear();
    this->m_unreachable.clear();
    this->m_reachable.clear();

    // This algorithm is described at
    //   https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/

    // Collect all variables from `tracked` into `m_staged`. Each variable
    // that is encountered in the loop shall have a direct reference from either
    // `tracked` or `m_staged`, so its `gc_ref` counter is initialized to one.
    this->m_temp_1.merge(tracked);

    while(this->m_temp_1.erase_random(nullptr, &var)) {
      ROCKET_ASSERT(var);

      var->set_gc_ref(1);
      ROCKET_ASSERT(var->get_gc_ref() <= var->use_count() - 1);

      var->get_value().get_variables(this->m_staged, this->m_temp_1);
    }

    // Each key in `m_staged` denotes an internal reference, so its `gc_ref`
    // counter shall be incremented.
    while(this->m_staged.erase_random(nullptr, &var)) {
      ROCKET_ASSERT(var);

      var->set_gc_ref(var->get_gc_ref() + 1);
      ROCKET_ASSERT(var->get_gc_ref() <= var->use_count() - 1);

      this->m_temp_1.insert(var.get(), var);
    }

    // Mark all variables that have been collected so far.
    this->m_temp_1.merge(tracked);

    while(this->m_temp_1.erase_random(nullptr, &var)) {
      ROCKET_ASSERT(var);

      if(var->get_gc_ref() == var->use_count() - 1) {
        // This variable is possibly reachable.
        this->m_unreachable.insert(var.get(), var);
        continue;
      }

      // This variable is reachable.
      // Mark variables that are indirectly reachable, too.
      do {
        ROCKET_ASSERT(var);

        var->set_gc_ref(0);
        this->m_unreachable.erase(var.get());
        this->m_reachable.insert(var.get(), var);

        var->get_value().get_variables(this->m_staged, this->m_temp_2);
      }
      while(this->m_temp_2.erase_random(nullptr, &var));
    }

    // Collect all variables from `m_unreachable`.
    while(this->m_unreachable.erase_random(nullptr, &var)) {
      ROCKET_ASSERT(var);

      ROCKET_ASSERT(var->get_gc_ref() != 0);
      var->uninitialize();
      bool erased = tracked.erase(var);
      nvars += 1;

      // Pool the variable. This shall be the last operation due to
      // possible exceptions. If the variable cannot be pooled, it is
      // deallocated immediately.
      if(erased)
        this->m_pool.insert(var.get(), var);
    }

    if(next_opt) {
      // Push reachable variables to the next generation, if any.
      // Note exception safety.
      while(this->m_reachable.erase_random(nullptr, &var)) {
        ROCKET_ASSERT(var);

        ROCKET_ASSERT(var->get_gc_ref() == 0);
        next_opt->insert(var.get(), var);
        bool erased = tracked.erase(var);

        // Undo the operation if the variable was not in `tracked`.
        if(erased)
          *count_opt += 1;
        else
          next_opt->erase(var);
      }
    }

    // Reset the GC counter to zero only if the operation completes
    // normally i.e. don't reset it if an exception is thrown.
    this->m_counts[gMax-gen] = 0;

    // Return the number of variables that have been collected.
    return nvars;
  }

rcptr<Variable>
Garbage_Collector::
create_variable(GC_Generation gen_hint)
  {
    // Perform automatic garbage collection.
    for(size_t gen = 0;  gen <= gMax;  ++gen)
      if(this->m_counts[gMax-gen] >= this->m_thres[gMax-gen])
        this->do_collect_generation(gen);

    // Get a cached variable.
    // If the pool has been exhausted, allocate a new one.
    rcptr<Variable> var;
    this->m_pool.erase_random(nullptr, &var);
    if(!var)
      var = ::rocket::make_refcnt<Variable>();

    // Track it.
    size_t gen = gMax - gen_hint;
    this->m_tracked.mut(gen).insert(var.get(), var);
    this->m_counts[gen] += 1;
    return var;
  }

size_t
Garbage_Collector::
collect_variables(GC_Generation gen_limit)
  {
    // Collect all variables up to generation `gen_limit`.
    size_t nvars = 0;
    for(size_t gen = 0;  (gen <= gMax) && (gen <= gen_limit);  ++gen)
      nvars += this->do_collect_generation(gen);

    // Clear cached variables.
    // Return the number of variables that have been collected.
    this->m_pool.clear();
    return nvars;
  }

size_t
Garbage_Collector::
finalize() noexcept
  {
    // Ensure no garbage collection is in progress.
    const Sentry sentry(this->m_recur);
    if(!sentry)
      ASTERIA_TERMINATE("garbage collector not finalizable while in use");

    size_t nvars = 0;
    rcptr<Variable> var;

    this->m_staged.clear();
    this->m_temp_1.clear();
    this->m_temp_2.clear();
    this->m_unreachable.clear();
    this->m_reachable.clear();

    // Wipe out all tracked variables. Indirect ones may be foreign so they
    // must not be wiped.
    for(size_t gen = 0;  gen <= gMax;  ++gen) {
      auto& tracked = this->m_tracked.mut(gMax-gen);
      nvars += tracked.size();

      while(tracked.erase_random(nullptr, &var))
        var->uninitialize();
    }

    // Clear cached variables.
    nvars += this->m_pool.size();
    this->m_pool.clear();
    return nvars;
  }

}  // namespace asteria
