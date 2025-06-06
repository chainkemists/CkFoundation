#pragma once

#include <functional>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T, typename T_Policy, typename>
    class IsValid_Executor;
}

// --------------------------------------------------------------------------------------------------------------------

// Mainly used to work around MSVC preprocessor bug
#define EXPAND( x ) x
#define EXPAND_ALL(...) __VA_ARGS__

// --------------------------------------------------------------------------------------------------------------------

#define NOT !
#define COMMA ,

#define CK_EMPTY

// --------------------------------------------------------------------------------------------------------------------

#define PASTE(a,b) a ## b
#define XPASTE(a,b) PASTE(a,b)

// --------------------------------------------------------------------------------------------------------------------

// Taken from: https://stackoverflow.com/a/26408195/368599 and modified to work with MSVC
// ReSharper disable once IdentifierTypo
#define NARG_(...)  NARG_I_(__VA_ARGS__,RSEQ_N())
// ReSharper disable once IdentifierTypo
#define NARG_I_(...) EXPAND(ARG_N(__VA_ARGS__))
#define ARG_N( \
      _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
     _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
     _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
     _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
     _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
     _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
     _61,_62,_63,N,...) N
// ReSharper disable once IdentifierTypo
#define RSEQ_N() \
     63,62,61,60,                   \
     59,58,57,56,55,54,53,52,51,50, \
     49,48,47,46,45,44,43,42,41,40, \
     39,38,37,36,35,34,33,32,31,30, \
     29,28,27,26,25,24,23,22,21,20, \
     19,18,17,16,15,14,13,12,11,10, \
     9,8,7,6,5,4,3,2,1,0

// --------------------------------------------------------------------------------------------------------------------

#define CK_ENABLE_CUSTOM_VALIDATION()\
    template <typename T, typename T_Policy, typename>\
    friend class ck::IsValid_Executor

#define CK_GENERATED_BODY(_InClass_)\
    using ThisType = _InClass_;\
    CK_ENABLE_CUSTOM_VALIDATION()

#define CK_PROPERTY_GET(_InVar_)\
    const auto& Get##_InVar_() const { return _InVar_; }

#define CK_PROPERTY_GET_BY_COPY(_InVar_)\
    auto Get##_InVar_() const { return _InVar_; }

#define CK_PROPERTY_GET_NON_CONST(_InVar_)\
    auto& Get##_InVar_() { return _InVar_; }

#define CK_PROPERTY_GET_PASSTHROUGH(_InVar_, _Getter_)\
    decltype(_InVar_._Getter_()) _Getter_() const { return _InVar_._Getter_(); }

#define CK_PROPERTY_GET_STATIC(_InVar_)\
    static const auto& Get##_InVar_() { return _InVar_; }

#define CK_PROPERTY_SET(_InVar_)\
    auto Set##_InVar_(const decltype(_InVar_)& InValue) -> decltype(*this)& { _InVar_ = InValue; return *this; }

#define CK_PROPERTY_UPDATE(_InVar_)\
    auto Update##_InVar_(std::function<void(decltype(_InVar_)&)> InFunc) -> ThisType&\
    {\
        InFunc(_InVar_);\
        return *this;\
    }

#define CK_PROPERTY(_InVar_)\
    CK_PROPERTY_GET(_InVar_);\
    CK_PROPERTY_GET_NON_CONST(_InVar_);\
    CK_PROPERTY_SET(_InVar_);\
    CK_PROPERTY_UPDATE(_InVar_)

// --------------------------------------------------------------------------------------------------------------------

#define CK_PROPERTY_AND_VAR_GET(_Type_, _InVar_)\
private:\
_Type_ _InVar_;\
public:\
CK_PROPERTY_GET(_InVar_)

#define CK_PROPERTY_AND_VAR(_Type_, _InVar_)\
private:\
_Type_ _InVar_;\
public:\
CK_PROPERTY(_InVar_)

// --------------------------------------------------------------------------------------------------------------------
// Constructor definition

#define CK_DEFINE_CONSTRUCTOR_1(_ClassType_, _1)\
    explicit _ClassType_(decltype(_1) _1) : _1(std::move(_1)) {}

#define CK_DEFINE_CONSTRUCTOR_2(_ClassType_, _1, _2)\
    _ClassType_(decltype(_1) _1, decltype(_2) _2)\
        : _1(std::move(_1)), _2(std::move(_2)) {}

#define CK_DEFINE_CONSTRUCTOR_3(_ClassType_, _1, _2, _3)\
    _ClassType_(decltype(_1) _1, decltype(_2) _2, decltype(_3) _3)\
        : _1(std::move(_1)), _2(std::move(_2)), _3(std::move(_3)) {}

#define CK_DEFINE_CONSTRUCTOR_4(_ClassType_, _1, _2, _3, _4)\
    _ClassType_(decltype(_1) _1, decltype(_2) _2, decltype(_3) _3, decltype(_4) _4)\
        : _1(std::move(_1)), _2(std::move(_2)), _3(std::move(_3)), _4(std::move(_4)) {}

#define CK_DEFINE_CONSTRUCTOR_5(_ClassType_, _1, _2, _3, _4, _5)\
    _ClassType_(decltype(_1) _1, decltype(_2) _2, decltype(_3) _3, decltype(_4) _4, decltype(_5) _5)\
        : _1(std::move(_1)), _2(std::move(_2)), _3(std::move(_3)), _4(std::move(_4)), _5(std::move(_5)){}

#define CK_DEFINE_CONSTRUCTOR_6(_ClassType_, _1, _2, _3, _4, _5, _6)\
    _ClassType_(decltype(_1) _1, decltype(_2) _2, decltype(_3) _3, decltype(_4) _4, decltype(_5) _5, decltype(_6) _6)\
        : _1(std::move(_1)), _2(std::move(_2)), _3(std::move(_3)), _4(std::move(_4)), _5(std::move(_5)), _6(std::move(_6)){}

#define CK_DEFINE_CONSTRUCTOR_7(_ClassType_, _1, _2, _3, _4, _5, _6, _7)\
    _ClassType_(decltype(_1) _1, decltype(_2) _2, decltype(_3) _3, decltype(_4) _4, decltype(_5) _5, decltype(_6) _6, decltype(_7) _7)\
        : _1(std::move(_1)), _2(std::move(_2)), _3(std::move(_3)), _4(std::move(_4)), _5(std::move(_5)), _6(std::move(_6)), _7(std::move(_7)){}

#define CK_DEFINE_CONSTRUCTOR_8(_ClassType_, _1, _2, _3, _4, _5, _6, _7, _8)\
    _ClassType_(decltype(_1) _1, decltype(_2) _2, decltype(_3) _3, decltype(_4) _4, decltype(_5) _5, decltype(_6) _6, decltype(_7) _7, decltype(_8) _8)\
        : _1(std::move(_1)), _2(std::move(_2)), _3(std::move(_3)), _4(std::move(_4)), _5(std::move(_5)), _6(std::move(_6)), _7(std::move(_7)), _8(std::move(_8)){}

#define CK_DEFINE_CONSTRUCTOR_9(_ClassType_, _1, _2, _3, _4, _5, _6, _7, _8, _9)\
    _ClassType_(decltype(_1) _1, decltype(_2) _2, decltype(_3) _3, decltype(_4) _4, decltype(_5) _5, decltype(_6) _6, decltype(_7) _7, decltype(_8) _8, decltype(_9) _9)\
        : _1(std::move(_1)), _2(std::move(_2)), _3(std::move(_3)), _4(std::move(_4)), _5(std::move(_5)), _6(std::move(_6)), _7(std::move(_7)), _8(std::move(_8)), _9(std::move(_9)){}

#define CK_DEFINE_CONSTRUCTOR_VARIADIC(_ClassType_, M, ...)\
    EXPAND(M(_ClassType_, __VA_ARGS__))

#define CK_DEFINE_CONSTRUCTOR(_ClassType_, ...)\
    CK_DEFINE_CONSTRUCTOR_VARIADIC(_ClassType_, XPASTE(CK_DEFINE_CONSTRUCTOR_, EXPAND(NARG_(__VA_ARGS__))), __VA_ARGS__)

#define CK_DEFINE_CONSTRUCTORS(_ClassType_, ...)\
    _ClassType_() = default;\
    CK_DEFINE_CONSTRUCTOR(_ClassType_, __VA_ARGS__)

// --------------------------------------------------------------------------------------------------------------------

#define CK_USING_BASE_CONSTRUCTORS(_InObject_)\
    using _InObject_::_InObject_

// --------------------------------------------------------------------------------------------------------------------

#define CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(_InObject_)\
    bool operator !=(_InObject_ const& InOther) const { return NOT (operator==(InOther)); }

#define CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL_T(_Template_, _InObject_)\
    _Template_\
    bool operator !=(_InObject_ const& InOther) const { return NOT (operator==(InOther)); }

#define CK_USING_OPERATORS_EQUAL_NOT_EQUAL(_InObject_)\
    using _InObject_::operator==;\
    using _InObject_::operator!=

#define CK_DECL_AND_DEF_OPERATORS(_InObject_)\
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(_InObject_)\
    bool operator > (_InObject_ const& InOther) const { return InOther.operator<(*this); }\
    bool operator <=(_InObject_ const& InOther) const { return NOT (operator>(InOther)); }\
    bool operator >=(_InObject_ const& InOther) const { return NOT (operator<(InOther)); }

#define CK_USING_OPERATORS(_InObject_)\
    CK_USING_OPERATORS_EQUAL_NOT_EQUAL(_InObject_);\
    using _InObject_::operator>;\
    using _InObject_::operator<=;\
    using _InObject_::operator>=

#define CK_DECL_AND_DEF_OPERATORS_T(_Template_, _InObject_)\
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL_T(_Template_, _InObject_)\
    _Template_\
    bool operator > (_InObject_ const& InOther) const { return InOther.operator<(*this); }\
    _Template_\
    bool operator <=(_InObject_ const& InOther) const { return NOT (operator>(InOther)); }\
    _Template_\
    bool operator >=(_InObject_ const& InOther) const { return NOT (operator<(InOther)); }

#define CK_DECL_AND_DEF_ADD_SUBTRACT_ASSIGNMENT_OPERATORS(_InObject_)\
    auto operator +=(_InObject_ const& InOther) -> _InObject_ { *this = operator+(InOther); return *this; }\
    auto operator -=(_InObject_ const& InOther) -> _InObject_ { *this = operator-(InOther); return *this; }

#define CK_DECL_AND_DEF_MULTIPLY_DIVIDE_ASSIGNMENT_OPERATORS(_InObject_)\
    auto operator *=(_InObject_ const& InOther) -> _InObject_ { *this = operator*(InOther); return *this; }\
    auto operator /=(_InObject_ const& InOther) -> _InObject_ { *this = operator/(InOther); return *this; }

#define CK_DECL_AND_DEF_SHORTHAND_ASSIGNMENT_OPERATORS(_InObject_)\
    CK_DECL_AND_DEF_ADD_SUBTRACT_ASSIGNMENT_OPERATORS(_InObject_)\
    CK_DECL_AND_DEF_MULTIPLY_DIVIDE_ASSIGNMENT_OPERATORS(_InObject_)

// --------------------------------------------------------------------------------------------------------------------

// exposes a 'This' function for classes with static polymorphism
#define CK_ENABLE_SFINAE_THIS(_DerivedType_)           \
    auto This() -> _DerivedType_*                      \
    {                                                  \
        return static_cast<_DerivedType_*>(this);      \
    }                                                  \
    auto This() const -> const _DerivedType_*          \
    {                                                  \
        return static_cast<const _DerivedType_*>(this);\
    }

// --------------------------------------------------------------------------------------------------------------------

#define CK_INTENTIONALLY_EMPTY()

// Useful to scope other macros that may make certain assumptions about return type
// and make the nested macro callable in functions such as the constructor
#define CK_SCOPE_CALL(_NestedCall_)\
    [&]() -> bool { _NestedCall_; return {}; }();

// --------------------------------------------------------------------------------------------------------------------
