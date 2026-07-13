#include <stdint.h>
#include <stdio.h>
#include <climits>
#include <cstring>

#include <fuzzer/FuzzedDataProvider.h>
#include "asteria/utils.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);

    // Round-trip the whole input through the UTF-8 codec: encode an arbitrary
    // code point, then decode every code point of an arbitrary string.
    char32_t cp = provider.ConsumeIntegral<char32_t>();
    std::string bytes = provider.ConsumeRandomLengthString(1000);
    ::asteria::cow_string text(bytes.data(), bytes.size());
    ::asteria::utf8_encode(text, cp);

    size_t offset = 0;
    char32_t decoded;
    while(offset < text.size())
      if(!::asteria::utf8_decode(decoded, text, offset))
        break;

    return 0;
}
