#pragma once

#include "cleantype/details/cleantype_clean.hpp"

namespace ck
{
#if WITH_ANGELSCRIPT_CK
    #define DEFINE_TYPE_MAPPING(_Type_, _Name_) \
    template <> struct RuntimeTypeToString_AngelScript<_Type_> { \
        static constexpr const TCHAR* Value = TEXT(_Name_); \
        using Type = _Type_; \
    };

    template <typename T>
    struct RuntimeTypeToString_AngelScript
    {
        static constexpr const TCHAR* Value = nullptr;
    };

    DEFINE_TYPE_MAPPING(float, "float32")
    DEFINE_TYPE_MAPPING(double, "float64")
    DEFINE_TYPE_MAPPING(uint8, "uint8")
    DEFINE_TYPE_MAPPING(uint32, "uint32")
    DEFINE_TYPE_MAPPING(uint64, "uint64")
    DEFINE_TYPE_MAPPING(int8, "int8")
    DEFINE_TYPE_MAPPING(int32, "int32")
    DEFINE_TYPE_MAPPING(int64, "int64")
    DEFINE_TYPE_MAPPING(FVector, "FVector")
    DEFINE_TYPE_MAPPING(FVector2D, "FVector2D")
    DEFINE_TYPE_MAPPING(FRotator, "FRotator")
    DEFINE_TYPE_MAPPING(FTransform, "FTransform")
    DEFINE_TYPE_MAPPING(FIntVector, "FIntVector")
    DEFINE_TYPE_MAPPING(FIntPoint, "FIntPoint")
    DEFINE_TYPE_MAPPING(FQuat, "FQuat")
    DEFINE_TYPE_MAPPING(FBox, "FBox")
    DEFINE_TYPE_MAPPING(FBox2D, "FBox2D")

    // Helper template to extract container element types
    template<typename T>
    struct ContainerTraits
    {
        static constexpr bool IsContainer = false;
        static FString Simplify() { return FString(); }
    };

    template <typename T>
    auto Get_RuntimeTypeToString_AngelScript() -> FString
    {
        // First check if we have a direct mapping
        if constexpr (RuntimeTypeToString_AngelScript<T>::Value != nullptr)
        {
            return RuntimeTypeToString_AngelScript<T>::Value;
        }
        // Check if it's a container type we can handle directly
        else if constexpr (ContainerTraits<T>::IsContainer)
        {
            return ContainerTraits<T>::Simplify();
        }
        // Check for enum types
        else if constexpr (std::is_enum_v<T>)
        {
            const auto& CleanName = cleantype::clean<T>();
            const auto CleanNameStr = FString{static_cast<int32>(CleanName.length()), CleanName.data()};
            return CleanNameStr.Replace(TEXT("enum "), TEXT(""), ESearchCase::CaseSensitive);
        }
        // Check for pointer types
        else if constexpr (std::is_pointer_v<T>)
        {
            const auto& CleanName = cleantype::clean<T>();
            const auto CleanNameStr = FString{static_cast<int32>(CleanName.length()), CleanName.data()};
            return CleanNameStr.Replace(TEXT("*"), TEXT(""), ESearchCase::IgnoreCase);
        }
        // Default case: just use cleantype output
        else
        {
            const auto& CleanName = cleantype::clean<T>();
            return FString{static_cast<int32>(CleanName.length()), CleanName.data()};
        }
    }

    // Specialization for TArray
    template<typename ElementType, typename AllocatorType>
    struct ContainerTraits<TArray<ElementType, AllocatorType>>
    {
        static constexpr bool IsContainer = true;
        using Element = ElementType;

        static FString Simplify()
        {
            return FString::Printf(TEXT("TArray<%s>"),
                *Get_RuntimeTypeToString_AngelScript<ElementType>());
        }
    };

    // Specialization for TSet
    template<typename ElementType, typename KeyFuncs, typename AllocatorType>
    struct ContainerTraits<TSet<ElementType, KeyFuncs, AllocatorType>>
    {
        static constexpr bool IsContainer = true;
        using Element = ElementType;

        static FString Simplify()
        {
            return FString::Printf(TEXT("TSet<%s>"),
                *Get_RuntimeTypeToString_AngelScript<ElementType>());
        }
    };

    // Specialization for TMap
    template<typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs>
    struct ContainerTraits<TMap<KeyType, ValueType, SetAllocator, KeyFuncs>>
    {
        static constexpr bool IsContainer = true;
        using Key = KeyType;
        using Value = ValueType;

        static FString Simplify()
        {
            return FString::Printf(TEXT("TMap<%s, %s>"),
                *Get_RuntimeTypeToString_AngelScript<KeyType>(),
                *Get_RuntimeTypeToString_AngelScript<ValueType>());
        }
    };

    // Specialization for TWeakPtr
    template<typename ObjectType, ESPMode InMode>
    struct ContainerTraits<TWeakPtr<ObjectType, InMode>>
    {
        static constexpr bool IsContainer = true;
        using Element = ObjectType;

        static FString Simplify()
        {
            return FString::Printf(TEXT("TWeakPtr<%s>"),
                *Get_RuntimeTypeToString_AngelScript<ObjectType>());
        }
    };

    // Specialization for TUniquePtr
    template<typename T, typename Deleter>
    struct ContainerTraits<TUniquePtr<T, Deleter>>
    {
        static constexpr bool IsContainer = true;
        using Element = T;

        static FString Simplify()
        {
            return FString::Printf(TEXT("TUniquePtr<%s>"),
                *Get_RuntimeTypeToString_AngelScript<T>());
        }
    };

    // Specialization for TOptional
    template<typename OptionalType>
    struct ContainerTraits<TOptional<OptionalType>>
    {
        static constexpr bool IsContainer = true;
        using Element = OptionalType;

        static FString Simplify()
        {
            return FString::Printf(TEXT("TOptional<%s>"),
                *Get_RuntimeTypeToString_AngelScript<OptionalType>());
        }
    };

    // Specialization for TSubclassOf
    template<typename TClass>
    struct ContainerTraits<TSubclassOf<TClass>>
    {
        static constexpr bool IsContainer = true;
        using Element = TClass;

        static FString Simplify()
        {
            return FString::Printf(TEXT("TSubclassOf<%s>"),
                *Get_RuntimeTypeToString_AngelScript<TClass>());
        }
    };

#else
    template <typename T>
    auto Get_RuntimeTypeToString_AngelScript() -> FString
    {
        return ck::Get_RuntimeTypeToString<T>();
    }
#endif
}