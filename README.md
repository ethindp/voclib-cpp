# voclib-cpp
A thin wrapper around Blastbays' vocoder library written in C

This wrapper is written in C++. You can use it like any other C++ header:

```cpp
#include <voclib.hpp>
//...
Vocoder voc(bands, filters_per_band, samplerate);
//...
auto audio = voc.Process(carrier_buffer, modulator_buffer, frames);
```
