/**
 * @brief Defines bitwise operators for specific enum classes.
 * @author Aaron Jones
 * @copyright MIT License
 *
 * This can be used to allow an enum class to gain the following bitwise and
 * logical operations: |, |=, &, &=, ^, ^=, ~, !. This additional functionality
 * comes at a cost in that values that are not defined in the enum can more
 * easily be created.  Switch statements that cover every explicitly defined
 * value in the enum may miss values because of this.
 *
 * One oddity is that implicit conversion to bool is not enabled, so a statement
 * like "if (val)" does not work. In order to check if any bits are set in a
 * value the logical-not operator (!) must be applied twice, for example
 * "if (!!val)". To check if no bits are set the normal way works, e.g.
 * "if (!val)".
 *
 * Despite those shortcomings, many useful properties of enum classes are
 * retained. Enum values of one type cannot be mixed up with enum values of
 * another, or even with the underlying type of the enum. So "Foo::VALUE |
 * Bar::VALUE" and "Foo::VALUE | 3" will not compile without explicit casting.
 * The < operator and std::hash implementations are provided by enum classes,
 * so they can still be used in ordered and unordered collections. All
 * arithmetic and bit shifting operations are still denied since they typically
 * do not make sense in the context of bitwise flags.
 *
 * The operators are all constexpr, so the bitwise operations may be used at
 * compile-time.
 *
 * Example:
 * \code
 * #include <cassert>
 * #include <cstdint>
 * #include <set>
 * #include <unordered_set>
 * #include "bitwise_enum.hpp"
 *
 * enum class foo : uint8_t {
 *     FOO0 = 0b00,
 *     FOO1 = 0b01,
 *     FOO2 = 0b10
 * };
 * ENABLE_ENUM_BITWISE_OPERATORS(foo);
 *
 * enum class bar : uint8_t {
 *     BAR1 = 0b01,
 *     BAR2 = 0b10
 * };
 * ENABLE_ENUM_BITWISE_OPERATORS(bar);
 *
 * // FAILS TO COMPILE - ENABLE_ENUM_BITWISE_OPERATORS(int);
 *
 * int main(int, char**) {
 *     constexpr foo foo1 = foo::FOO1;
 *     constexpr foo foo2 = foo::FOO2;
 *     constexpr foo foo3 = foo::FOO1 | foo::FOO2;
 *     static_assert(foo3 & foo1, "Error.");
 *     static_assert((foo3 & ~foo2) == foo1, "Error.");
 *     static_assert(!!foo1, "Error.");
 *     static_assert(!foo::FOO0, "Error.");
 *     // FAILS TO COMPILE - constexpr foo foo4 = foo1 | 0b01;
 *     // FAILS TO COMPILE - constexpr foo foo5 = foo1 | bar::BAR1;
 *     // FAILS TO COMPILE - constexpr foo foo6 = foo::FOO1 + foo::FOO2;
 *
 *     std::set<foo> foo_set{foo::FOO1, foo::FOO2};
 *     assert(foo_set.count(foo::FOO1));
 *     std::unordered_set<foo> foo_unordered_set{foo::FOO1, foo::FOO2};
 *     assert(foo_unordered_set.count(foo::FOO1));
 * }
 * \endcode
 */

#ifndef __BITWISE_ENUM_HPP
#define __BITWISE_ENUM_HPP

#include <type_traits>

/**
 * Type trait class for enabling bitwise operations on enums. By default
 * bitwise operations on enums are disabled.
 *
 * @tparam enum_t The enum type on which the type trait is being queried.
 */
template <typename enum_t>
struct enable_enum_bitwise_operators {
    /**
     * Whether or not bitwise operators should be enabled for the enum type.
     * To enable make a template specilization of this class for your type and
     * set value to true.
     */
    static constexpr bool value = false;
};

/**
 * Helper to simplify syntax for querying whether or not bitwise operations are
 * enabled for a given enum.
 *
 * @tparam enum_t The enum type on which the type trait is being queried.
 */
template <typename enum_t>
constexpr bool enable_enum_bitwise_operators_v =
    enable_enum_bitwise_operators<enum_t>::value;

/**
 * Bitwise-or operator for enums that have bitwise operations enabled.
 *
 * @tparam enum_t The enum type on which the bitwise-or operation is being
 *                performed.
 *
 * @param lhs The left-hand side of the operation.
 * @param rhs The right-hand side of the operation.
 *
 * @returns The result of the bitwise-or operation on the underlying enum type.
 */
template <typename enum_t>
constexpr auto operator|(const enum_t& lhs, const enum_t& rhs)
    -> typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>,
                                 enum_t> {
    return static_cast<enum_t>(
        static_cast<std::underlying_type_t<enum_t>>(lhs) |
        static_cast<std::underlying_type_t<enum_t>>(rhs));
}

/**
 * Bitwise-or-equals operator for enums that have bitwise operations enabled.
 *
 * @tparam enum_t The enum type on which the bitwise-or-equals operation is
 *                being performed.
 *
 * @param lhs The left-hand side of the operation.
 * @param rhs The right-hand side of the operation.
 */
template <typename enum_t>
constexpr auto operator|=(enum_t& lhs, const enum_t& rhs)
    -> typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>,
                                 void> {
    lhs = lhs | rhs;
}

/**
 * Bitwise-and operator for enums that have bitwise operations enabled.
 *
 * @tparam enum_t The enum type on which the bitwise-and operation is being
 *                performed.
 *
 * @param lhs The left-hand side of the operation.
 * @param rhs The right-hand side of the operation.
 *
 * @returns The result of the bitwise-and operation on the underlying enum type.
 */
template <typename enum_t>
constexpr auto operator&(const enum_t& lhs, const enum_t& rhs)
    -> typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>,
                                 enum_t> {
    return static_cast<enum_t>(
        static_cast<std::underlying_type_t<enum_t>>(lhs) &
        static_cast<std::underlying_type_t<enum_t>>(rhs));
}

/**
 * Bitwise-and-equals operator for enums that have bitwise operations enabled.
 *
 * @tparam enum_t The enum type on which the bitwise-and-equals operation is
 *                being performed.
 *
 * @param lhs The left-hand side of the operation.
 * @param rhs The right-hand side of the operation.
 */
template <typename enum_t>
constexpr auto operator&=(enum_t& lhs, const enum_t& rhs)
    -> typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>,
                                 void> {
    lhs = lhs & rhs;
}

/**
 * Bitwise-xor operator for enums that have bitwise operations enabled.
 *
 * @tparam enum_t The enum type on which the bitwise-xor operation is being
 *                performed.
 *
 * @param lhs The left-hand side of the operation.
 * @param rhs The right-hand side of the operation.
 *
 * @returns The result of the bitwise-xor operation on the underlying enum type.
 */
template <typename enum_t>
constexpr auto operator^(const enum_t& lhs, const enum_t& rhs)
    -> typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>,
                                 enum_t> {
    return static_cast<enum_t>(
        static_cast<std::underlying_type_t<enum_t>>(lhs) ^
        static_cast<std::underlying_type_t<enum_t>>(rhs));
}

/**
 * Bitwise-xor-equals operator for enums that have bitwise operations enabled.
 *
 * @tparam enum_t The enum type on which the bitwise-xor-equals operation is
 *                being performed.
 *
 * @param lhs The left-hand side of the operation.
 * @param rhs The right-hand side of the operation.
 */
template <typename enum_t>
constexpr auto operator^=(enum_t& lhs, const enum_t& rhs)
    -> typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>,
                                 void> {
    lhs = lhs ^ rhs;
}

/**
 * Bitwise-not operator for enums that have bitwise operations enabled.
 *
 * @tparam enum_t The enum type on which the bitwise-not operation is being
 *                performed.
 *
 * @param val The value to negate.
 *
 * @returns The result of the bitwise-not operation on the underlying enum type.
 */
template <typename enum_t>
constexpr auto operator~(const enum_t& val)
    -> typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>,
                                 enum_t> {
    return static_cast<enum_t>(
        ~static_cast<std::underlying_type_t<enum_t>>(val));
}

/**
 * Logical-not operator for enums that have bitwise operations enabled. Returns
 * the result of comparing the underlying enum value with the underlying type's
 * representation of zero.
 *
 * @tparam enum_t The enum type on which the logical-not operation is being
 *                performed.
 *
 * @param val The value on which to perform logical-not.
 *
 * @returns True if the enum value is zero, false otherwise.
 */
template <typename enum_t>
constexpr auto operator!(const enum_t& val)
    -> typename std::enable_if_t<enable_enum_bitwise_operators_v<enum_t>,
                                 bool> {
    return static_cast<std::underlying_type_t<enum_t>>(val) ==
           static_cast<std::underlying_type_t<enum_t>>(0);
}

/**
 * Helper macro to enable bitwise operations on a given enum type.
 *
 * @param enum_t The enum type for which bitwise operations should be enabled.
 */
#define ENABLE_ENUM_BITWISE_OPERATORS(enum_t)                                  \
    template <>                                                                \
    struct enable_enum_bitwise_operators<enum_t> {                             \
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum."); \
        static constexpr bool value = true;                                    \
    }

#endif /* __BITWISE_ENUM_HPP */
