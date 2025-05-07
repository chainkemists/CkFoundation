#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct IsValid_Policy { };

    template <typename T>
    concept IsValidPolicy = std::is_base_of_v<ck::IsValid_Policy, T>;
}

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CUSTOM_IS_VALID_POLICY(_InPolicyType_)        \
namespace ck                                                    \
{                                                               \
    struct _InPolicyType_ : public ck::IsValid_Policy { };      \
}

CK_DEFINE_CUSTOM_IS_VALID_POLICY(IsValid_Policy_Default);

namespace ck
{
    template <typename T>
    struct IsValid_Executor_RefOrNoRef
    {
        using decayed_type = std::remove_cv_t<std::remove_reference_t<T>>;
        using base_type = std::remove_pointer_t<decayed_type>;
        using parameter_type = std::conditional_t<std::is_pointer_v<decayed_type>, const base_type*, const base_type&>;
    };

// --------------------------------------------------------------------------------------------------------------------

    template <typename T, typename T_Policy, typename = void>
    class IsValid_Executor
    {
    public:
        auto IsValid(const T& InObj) -> bool = delete;
    };

// --------------------------------------------------------------------------------------------------------------------

    template <typename T_Base, typename T_Derived>
    class IsValid_Executor_IsBaseOf
    {
    public:
        // double remove_pointer to deal with double pointers
        using base_type     = std::remove_pointer_t<std::remove_pointer_t<std::decay_t<T_Base>>>;
        using derived_type  = std::remove_pointer_t<std::remove_pointer_t<std::decay_t<T_Derived>>>;
    public:
        enum { value = (NOT std::is_class_v<base_type> && NOT std::is_class_v<derived_type>) ? true : std::is_base_of_v<base_type, derived_type> };
    };

// --------------------------------------------------------------------------------------------------------------------

    template <typename T, typename T_Policy>
    auto IsValid(T&& InObj, T_Policy InPolicy) -> bool
    {
        using decayed_type = std::remove_cv_t<std::remove_reference_t<T>>;

        return IsValid_Executor<decayed_type, T_Policy>{}.IsValid(std::forward<T>(InObj));
    }

    template <typename T>
    auto IsValid(T&& InObj) -> bool
    {
        return IsValid(std::forward<T>(InObj), IsValid_Policy_Default{});
    }

    template <typename T, typename T_Policy>
    auto Is_NOT_Valid(T&& InObj, T_Policy InPolicy) -> bool
    {
        return NOT IsValid(std::forward<T>(InObj), InPolicy);
    }

    template <typename T>
    auto Is_NOT_Valid(T&& InObj) -> bool
    {
        return NOT IsValid(InObj, IsValid_Policy_Default{});
    }
}

// convenience macro to enable the validator to access private members of a class/struct
#define CK_ENABLE_CUSTOM_VALIDATION()                 \
    template <typename T, typename T_Policy, typename>\
    friend class ck::IsValid_Executor

// --------------------------------------------------------------------------------------------------------------------

// A custom validator for a type is defined by 'IsValid_Executor' (See DEFINE_CUSTOM_IS_VALID variants) which
// works well for non-templated types but has trouble with templated types because the underlying value_type
// is not extractable in a standard way. The purpose of this macro is to help with the extraction of the templated
// type and using IsValid_Executor_IsBaseOf<...> to determine whether the passed types are related or not.
//
// USAGE: See usage examples in IsValid_Defaults
#define CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(_t_type_, ...)                                             \
namespace ck                                                                                              \
{                                                                                                         \
    template <typename T_Base, typename T_Derived>                                                        \
    class IsValid_Executor_IsBaseOf<_t_type_<T_Base, ##__VA_ARGS__>, _t_type_<T_Derived, ##__VA_ARGS__>>  \
    {                                                                                                     \
    public:                                                                                               \
        enum { value = IsValid_Executor_IsBaseOf<T_Base, T_Derived>::value };                             \
    };                                                                                                    \
                                                                                                          \
    template <typename T_Base, typename T_Derived>                                                        \
    class IsValid_Executor_IsBaseOf<_t_type_<T_Base, ##__VA_ARGS__>, T_Derived>                           \
    {                                                                                                     \
    public:                                                                                               \
        enum { value = IsValid_Executor_IsBaseOf<T_Base, T_Derived>::value };                             \
    };                                                                                                    \
}

// --------------------------------------------------------------------------------------------------------------------

// If we do not want certain types to have a custom validator, use this macro. It is better to define a deleted custom validator
// over getting cryptic compiler errors.
#define CK_DELETE_CUSTOM_IS_VALID(_type_)                                                                                        \
namespace ck                                                                                                                     \
{                                                                                                                                \
    template <typename T>                                                                                                        \
    class IsValid_Executor<T, IsValid_Policy_Default, typename std::enable_if_t<IsValid_Executor_IsBaseOf<_type_, T>::value>>    \
    {                                                                                                                            \
    public:                                                                                                                      \
        template <typename T_Type>                                                                                               \
        auto IsValid(typename IsValid_Executor_RefOrNoRef<_type_>::parameter_type InValue) -> bool                               \
        {                                                                                                                        \
            static_assert(std::is_same_v<T_Type, void>, "IsValid for " #_type_ " is explicitly deleted");                        \
            return false;                                                                                                        \
        }                                                                                                                        \
    };                                                                                                                           \
}

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CUSTOM_IS_VALID_INLINE(_type_, _policy_, _lambda_)                                           \
namespace ck                                                                                                   \
{                                                                                                              \
    template <typename T>                                                                                      \
    class IsValid_Executor<T, _policy_, typename std::enable_if_t<IsValid_Executor_IsBaseOf<_type_, T>::value>>\
    {                                                                                                          \
    public:                                                                                                    \
        auto IsValid(IsValid_Executor_RefOrNoRef<_type_>::parameter_type InValue) -> bool                      \
        {                                                                                                      \
            return _lambda_(InValue);                                                                          \
        }                                                                                                      \
    };                                                                                                         \
}

// --------------------------------------------------------------------------------------------------------------------

#define CK_DECLARE_CUSTOM_IS_VALID_INTERNAL(_api_name_, _type_, _policy_, _function_name_)                                 \
namespace ck                                                                                                   \
{                                                                                                              \
    _api_name_ auto _function_name_(IsValid_Executor_RefOrNoRef<_type_>::parameter_type InValue) -> bool;                 \
                                                                                                               \
    template <typename T>                                                                                      \
    class IsValid_Executor<T, _policy_, typename std::enable_if_t<IsValid_Executor_IsBaseOf<_type_, T>::value>>\
    {                                                                                                          \
    public:                                                                                                    \
        auto IsValid(IsValid_Executor_RefOrNoRef<_type_>::parameter_type InValue) -> bool                      \
        {                                                                                                      \
            return _function_name_(InValue);                                                                   \
        }                                                                                                      \
    };                                                                                                         \
}
#define CK_DECLARE_CUSTOM_IS_VALID(_api_name_, _type_, _policy_)\
CK_DECLARE_CUSTOM_IS_VALID_INTERNAL(_api_name_, _type_, _policy_, IsValid_##_type_##_policy_)

#define CK_DECLARE_CUSTOM_IS_VALID_NAMESPACE(_api_name_, _namespace_, _type_, _policy_)\
CK_DECLARE_CUSTOM_IS_VALID_INTERNAL(_api_name_, _namespace_::_type_, _policy_, IsValid_name_space_##_type_##_policy_)

#define CK_DECLARE_CUSTOM_IS_VALID_PTR(_api_name_, _type_, _policy_)\
CK_DECLARE_CUSTOM_IS_VALID_INTERNAL(_api_name_, _type_*, _policy_, IsValid_ptr_##_type_##_policy_)

#define CK_DECLARE_CUSTOM_IS_VALID_CONST_PTR(_api_name_, _type_, _policy_)\
CK_DECLARE_CUSTOM_IS_VALID_INTERNAL(_api_name_, const _type_*, _policy_, IsValid_const_ptr_##_type_##_policy_)

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CUSTOM_IS_VALID_INTERNAL(_type_, _function_name_, _lambda_)                              \
namespace ck                                                                                               \
{                                                                                                          \
    auto _function_name_(IsValid_Executor_RefOrNoRef<_type_>::parameter_type InValue) -> bool              \
    {                                                                                                      \
        return _lambda_(InValue);                                                                          \
    }                                                                                                      \
}

#define CK_DEFINE_CUSTOM_IS_VALID(_type_, _policy_, _lambda_)\
CK_DEFINE_CUSTOM_IS_VALID_INTERNAL(_type_, IsValid_##_type_##_policy_, _lambda_)

#define CK_DEFINE_CUSTOM_IS_VALID_NAMESPACE(_namespace_, _type_, _policy_, _lambda_)\
CK_DEFINE_CUSTOM_IS_VALID_INTERNAL(_namespace_::_type_, IsValid_name_space_##_type_##_policy_, _lambda_)

#define CK_DEFINE_CUSTOM_IS_VALID_PTR(_type_, _policy_, _lambda_)\
CK_DEFINE_CUSTOM_IS_VALID_INTERNAL(_type_*, IsValid_ptr_##_type_##_policy_, _lambda_)

#define CK_DEFINE_CUSTOM_IS_VALID_CONST_PTR(_type_, _policy_, _lambda_)\
CK_DEFINE_CUSTOM_IS_VALID_INTERNAL(const _type_*, IsValid_const_ptr_##_type_##_policy_, _lambda_)

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CUSTOM_IS_VALID_T(_t_type_, _type_, _policy_, _lambda_)                                                  \
namespace ck                                                                                                               \
{                                                                                                                          \
    template <typename _t_type_>                                                                                           \
    class IsValid_Executor<_type_, _policy_, typename std::enable_if_t<IsValid_Executor_IsBaseOf<_type_, _t_type_>::value>>\
    {                                                                                                                      \
    public:                                                                                                                \
        auto IsValid(typename IsValid_Executor_RefOrNoRef<_type_>::parameter_type InValue) -> bool                         \
        {                                                                                                                  \
            return _lambda_(InValue);                                                                                      \
        }                                                                                                                  \
    };                                                                                                                     \
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    // --------------------------------------------------------------------------------------------------------------------

    struct CKCORE_API IsValid
    {
    public:
        template <typename T>
        auto operator()(const T& InObject) const -> bool;
    };

    struct CKCORE_API Is_NOT_Valid
    {
    public:
        template <typename T>
        auto operator()(const T& InObject) const -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------
    // Definitions

    template <typename T>
    auto
        IsValid::
        operator()(
            const T& InObject) const
        -> bool
    {
        return ck::IsValid(InObject);
    }

    template <typename T>
    auto
        Is_NOT_Valid::
        operator()(
            const T& InObject) const
        -> bool
    {
        return ck::Is_NOT_Valid(InObject);
    }
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkIsValid_Defaults.h"

// --------------------------------------------------------------------------------------------------------------------
