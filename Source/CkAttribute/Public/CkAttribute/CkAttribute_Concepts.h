#pragma once

#include "CkEcs/Concepts/CkConcepts.h"

#include <concepts>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::traits
{
    // ---- Handle Category Traits ----
    // Specialize these to true_type for each concrete attribute handle type.

    template <typename T>
    struct IsAttributeHandleType : std::false_type {};

    template <typename T>
    struct IsAttributeModifierHandleType : std::false_type {};

    template <typename T>
    struct IsAttributeRefillHandleType : std::false_type {};
}

// ---- Registration Macros ----

#define CK_REGISTER_ATTRIBUTE_HANDLE_TYPE(_HandleType_)                                         \
    namespace ck::traits { template <> struct IsAttributeHandleType<_HandleType_> : std::true_type {}; }

#define CK_REGISTER_ATTRIBUTE_MODIFIER_HANDLE_TYPE(_HandleType_)                                \
    namespace ck::traits { template <> struct IsAttributeModifierHandleType<_HandleType_> : std::true_type {}; }

#define CK_REGISTER_ATTRIBUTE_REFILL_HANDLE_TYPE(_HandleType_)                                  \
    namespace ck::traits { template <> struct IsAttributeRefillHandleType<_HandleType_> : std::true_type {}; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck::concepts
{
    // ---- Attribute Handle Concepts ----

    template <typename T>
    concept ValidAttributeHandleType = ValidHandleType<T>
        && ck::traits::IsAttributeHandleType<T>::value;

    template <typename T>
    concept ValidAttributeModifierHandleType = ValidHandleType<T>
        && ck::traits::IsAttributeModifierHandleType<T>::value;

    template <typename T>
    concept ValidAttributeRefillHandleType = ValidHandleType<T>
        && ck::traits::IsAttributeRefillHandleType<T>::value;

    // ---- Attribute Fragment ----

    template <typename T>
    concept ValidAttributeFragment = requires
    {
        typename T::AttributeDataType;
        typename T::HandleType;
    } && ValidAttributeHandleType<typename T::HandleType>;

    // ---- Attribute Modifier Fragment ----

    template <typename T>
    concept ValidAttributeModifierFragment = requires
    {
        typename T::AttributeFragmentType;
        typename T::HandleType;
    } && ValidAttributeFragment<typename T::AttributeFragmentType>
      && ValidAttributeModifierHandleType<typename T::HandleType>;
}

// --------------------------------------------------------------------------------------------------------------------
