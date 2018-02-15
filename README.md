# Inlined Vector

A c++11 vector-like data structure that stores elements internally. It can grow beyond its capacity, in which case it becomes a wrapper around std::vector. 

It is designed for use with simple lightweight types. For a production quality inlined vector see e.g., [abseil](https://github.com/abseil/abseil-cpp/blob/master/absl/container/inlined_vector.h) or similar.

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

API is similar to `std::vector`:

```
inlined_vector<int, 16, true> v { 1, 2, 3, 4, 5 };
assert(v.size() == 5);
v.push_back(13);
assert(v.back() == 13);
v.pop_back();
```

A basic iterator interface is available.

```
#include <iterator>
...
auto it = v.insert(std::next(v.begin(), 3), 42);
assert(*it == 42);
v.erase(it);
```

Use `expanded()` to check if the inlined_vector has grown into a dynamically-allocated vector.

```
inlined_vector<int, 16, true> v { 1, 2, 3, 4, 5 };
for (int i=0; i<32; ++i) v.push_back(i);
assert(v.expanded());
```

## Running the tests

Basic Catch2 tests are provided in `tests/`.

## Deficiencies

An `inlined_vector` stores default instances in the unused space. For simple types this is acceptable, but for heavier types this is inefficient.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
