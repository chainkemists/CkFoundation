# Bitwise Enum
C++14 header-only library for enabling bitwise operations on enum classes on a class-by-class basis. This is useful for defining bit flags in a reasonably safe way such that accidental mixing of types and accidental use of arithmetic operations will result in compilation errors.

# Usage
This library is entirely self-contained in the `bitwise_enum.hpp` header file. Simply include it and use the `ENABLE_ENUM_BITWISE_OPERATORS(type)` macro to enable bitwise operations for the given type.

# Details
This can be used to allow an enum class to gain the following bitwise and
logical operations: `|`, `|=`, `&`, `&=`, `^`, `^=`, `~`, `!`. This additional functionality
comes at a cost in that values that are not defined in the enum can more
easily be created.  Switch statements that cover every explicitly defined
value in the enum may miss values because of this.

One oddity is that implicit conversion to bool is not enabled, so a statement
like `if (val)` does not work. In order to check if any bits are set in a
value the logical-not operator (`!`) must be applied twice, for example
`if (!!val)`. To check if no bits are set the normal way works, e.g.
`if (!val)`.

Despite those shortcomings, many useful properties of enum classes are
retained. Enum values of one type cannot be mixed up with enum values of
another, or even with the underlying type of the enum. So `Foo::VALUE |
Bar::VALUE` and `Foo::VALUE | 3` will not compile without explicit casting.
The `<` operator and `std::hash` implementations are provided by enum classes,
so they can still be used in ordered and unordered collections. All
arithmetic and bit shifting operations are still denied since they typically
do not make sense in the context of bitwise flags.

The operators are all `constexpr`, so the bitwise operations may be used at
compile-time.

# Example

```c++
#include <cassert>
#include <cstdint>
#include <set>
#include <unordered_set>
#include "bitwise_enum.hpp"

enum class foo : uint8_t {
    FOO0 = 0b00,
    FOO1 = 0b01,
    FOO2 = 0b10
};
ENABLE_ENUM_BITWISE_OPERATORS(foo);

enum class bar : uint8_t {
    BAR1 = 0b01,
    BAR2 = 0b10
};
ENABLE_ENUM_BITWISE_OPERATORS(bar);

// FAILS TO COMPILE - ENABLE_ENUM_BITWISE_OPERATORS(int);

int main(int, char**) {
    constexpr foo foo1 = foo::FOO1;
    constexpr foo foo2 = foo::FOO2;
    constexpr foo foo3 = foo::FOO1 | foo::FOO2;
    static_assert(foo3 & foo1, "Error.");
    static_assert((foo3 & ~foo2) == foo1, "Error.");
    static_assert(!!foo1, "Error.");
    static_assert(!foo::FOO0, "Error.");
    // FAILS TO COMPILE - constexpr foo foo4 = foo1 | 0b01;
    // FAILS TO COMPILE - constexpr foo foo5 = foo1 | bar::BAR1;
    // FAILS TO COMPILE - constexpr foo foo6 = foo::FOO1 + foo::FOO2;

    std::set<foo> foo_set{foo::FOO1, foo::FOO2};
    assert(foo_set.count(foo::FOO1));
    std::unordered_set<foo> foo_unordered_set{foo::FOO1, foo::FOO2};
    assert(foo_unordered_set.count(foo::FOO1));
}
```
