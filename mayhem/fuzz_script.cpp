// In-process libFuzzer harness over Asteria's PARSE + EXECUTE code path — the exact path the
// `asteria <file>` CLI drives (repl/single.cpp: Simple_Script::reload_file(path) -> execute()).
// reload_string() is the in-memory twin of reload_file() (both funnel into the same tokenizer,
// parser and AST/bytecode generator); feeding the fuzz bytes as script source therefore exercises
// the lexer, parser, compiler and runtime just like the file-input target did — but as a fast,
// coverage-instrumented, in-process target with reliable edge reporting.
//
// Asteria reports script-level problems (syntax errors, runtime errors, thrown values, the
// recursion-sentry "stack overflow averted" guard) by THROWING C++ exceptions out of reload/execute;
// those are ordinary, expected control flow for a language interpreter, not memory-safety bugs, so we
// swallow them. Genuine defects — ASan/UBSan reports, SIGSEGV, heap corruption — are NOT C++
// exceptions and still abort the process, so libFuzzer/Mayhem still catch them.
#include <stddef.h>
#include <stdint.h>
#include <exception>

#include "asteria/simple_script.hpp"
#include "asteria/fwd.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Bound the script size so a single input can't monopolise the time budget.
    if(size > 64 * 1024)
      return 0;

    try {
      ::asteria::Simple_Script script;
      ::asteria::cow_string name("fuzz");
      ::asteria::cow_string code(reinterpret_cast<const char*>(data), size);
      script.reload_string(name, code);
      script.execute();
    }
    catch(::std::exception&) {
      // Expected: parse/runtime errors and thrown script values surface as C++ exceptions.
    }
    return 0;
}
