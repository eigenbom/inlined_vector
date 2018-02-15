# Inlined Vector

A simple c++11 vector-like data structure that stores elements internally. It can grow beyond its capacity, in which case it becomes a wrapper around std::vector. 

It is designed for simple use-cases with simple types. For a production quality inlined vector see e.g., [abseil](https://github.com/abseil/abseil-cpp/blob/master/absl/container/inlined_vector.h) or similar.

## Usage

Drop the header into your project. Provide the following defines before including the header if you wish to get exceptions or error messages.

```
#define INLINED_VECTOR_THROWS
#define INLINED_VECTOR_LOG_ERROR(message) std::cerr << message << "\n"
#include "inlined_vector.h"
```

The vector is templated on type, initial capacity, and whether it is allowed to expand or not. For example, `v1` is a non-expanding vector of 16 integers and `v2` is an expanding vector of 8 integers.

```
inlined_vector<int, 16, false> v1;
inlined_vector<int, 8, true> v2;
```

## Running the tests

Basic Catch2 tests are provided in `tests/`.

## Deficiencies

An `inlined_vector` stores default instances in the unused space. For simple types this is acceptable, but for heavier types this is inefficient.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
