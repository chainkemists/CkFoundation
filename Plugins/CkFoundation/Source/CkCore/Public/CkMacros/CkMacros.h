#pragma once

namespace ck
{
    template <typename T, typename T_Policy, typename>
    class IsValid_Executor;
}

#define NOT !
#define COMMA ,

#define CK_EMPTY

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

#define CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(_InObject_)\
    bool operator !=(_InObject_ const& InOther) const { return NOT (operator==(InOther)); }

#define CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL_T(_Template_, _InObject_)\
    _Template_\
    bool operator !=(_InObject_ const& InOther) const { return NOT (operator==(InOther)); }

#define CK_DECL_AND_DEF_OPERATORS(_InObject_)\
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(_InObject_)\
    bool operator > (_InObject_ const& InOther) const { return InOther.operator<(*this); }\
    bool operator <=(_InObject_ const& InOther) const { return NOT (operator>(InOther)); }\
    bool operator >=(_InObject_ const& InOther) const { return NOT (operator<(InOther)); }

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

#define CK_DEFINE_TYPEHASH(_type_) auto GetTypeHash(const _type_& InObj) -> uint32;

#define CK_INTENTIONALLY_EMPTY()
