#include <stdint.h>
#include <stdio.h>
#include <climits>

#include <fuzzer/FuzzedDataProvider.h>
#include "utils.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);

    char* str = strdup(provider.ConsumeRandomLengthString(1000).c_str());
    char32_t cp = provider.ConsumeIntegral<char32_t>();
    asteria::utf8_encode(str, cp);

    return 0;
}
