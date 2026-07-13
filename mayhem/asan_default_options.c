/* Baked ASan defaults for the fuzz binaries (Mayhem owns ASAN_OPTIONS):
 *  - detect_leaks=0: the interpreter frees via GC at exit; leak reports would flood every input.
 *  - detect_stack_use_after_return=0: asteria's recursion sentry measures stack depth by frame
 *    address distance; ASan's heap-allocated fake stacks make every script trip the sentry. */
const char *__asan_default_options(void)
{
    return "detect_leaks=0:detect_stack_use_after_return=0";
}
