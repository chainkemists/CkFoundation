#pragma once

#include "CkCore/Format/CkFormat.h"

#include <Templates/SubclassOf.h>

#if WITH_EDITOR
#include <UnrealEdMisc.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ctti::detail
{
    class cstring;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_format_default::details
{
    inline const auto Invalid_FProperty = FProperty {{}, NAME_None, {}};
}

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FString, [](const FString& InObj) { return *InObj; });

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FName);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FText);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FGuid);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FNetworkGUID);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FKey);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FInputChord);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FSoftObjectPath);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FSoftClassPath);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FRandomStream);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FRotator);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FVector);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FVector2D);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FIntVector);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FIntVector2);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FIntVector4);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FQuat);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FBox);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FBox2D);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FTransform);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FTimespan);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FNativeGameplayTag);

#if CK_DISABLE_GAMEPLAYTAG_STALENESS_VALIDATION
CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FGameplayTag);
#else
CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FGameplayTag);
#endif

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FGameplayTagContainer);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FAssetData);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FCollisionProfileName);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FDataTableRowHandle);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FCurveTableRowHandle);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FPlayInEditorID);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FInstancedStruct);

// --------------------------------------------------------------------------------------------------------------------

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, SWindow);

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER_NULLPTR_VALIDITY(SWindow, [&]() -> const SWindow&
{
    return *InObj;
});

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, UObject);

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(UObject, [&]() -> const UObject&
{
    return *InObj;
});

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, UActorComponent);

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(UActorComponent, [&]() -> const UActorComponent&
{
    return *InObj;
});

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, AActor);

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(AActor, [&]() -> const AActor&
{
    return *InObj;
});

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, UClass);

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(UClass, [&]() -> const UClass&
{
    return *InObj;
});

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, UFunction);

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(UFunction, [&]() -> const UFunction&
{
    return *InObj;
});

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, FProperty);

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(FProperty, []() -> const FProperty&
{
    return ck_format_default::details::Invalid_FProperty;
});

CK_DEFINE_CUSTOM_FORMATTER_T(TScriptInterface<T>, [](const auto& InObj)
{
    return ck::Format(TEXT("{}"), InObj.GetObject());
});

CK_DEFINE_CUSTOM_FORMATTER_T(TWeakObjectPtr<T>, [](const auto& InObj)
{
    return ck::Format(TEXT("{}"), InObj.Get());
});

CK_DEFINE_CUSTOM_FORMATTER_T(TSubclassOf<T>, [](const auto& InObj)
{
    return ck::Format(TEXT("{}"), InObj.Get());
});

CK_DEFINE_CUSTOM_FORMATTER_T(TObjectPtr<T>, [](const auto& InObj)
{
    return ck::Format(TEXT("{}"), InObj.Get());
});

CK_DEFINE_CUSTOM_FORMATTER_T(TOptional<T>, [](const auto& InObj)
{
    if (NOT ck::IsValid(InObj))
    { return ck::Format(TEXT("nullopt")); }

    return ck::Format(TEXT("{}"), *InObj);
});

CK_DEFINE_CUSTOM_FORMATTER_T(TSoftObjectPtr<T>, [](const auto& InObj)
{
    if (NOT ck::IsValid(InObj))
    { return ck::Format(TEXT("nullopt")); }

    return ck::Format(TEXT("{}"), *InObj);
});

CK_DEFINE_CUSTOM_FORMATTER_T(TSoftClassPtr<T>, [](const auto& InObj)
{
    if (NOT ck::IsValid(InObj))
    { return ck::Format(TEXT("nullopt")); }

    return ck::Format(TEXT("{}"), *InObj);
});

CK_DEFINE_CUSTOM_FORMATTER_T(TEnumAsByte<T>, [](const auto& InObj)
{
    return ck::Format(TEXT("{}"), T(InObj.GetValue()));
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER_ENUM(EAppReturnType::Type);
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ETickingGroup);
CK_DEFINE_CUSTOM_FORMATTER_ENUM(EComponentMobility::Type);

CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, ENetMode);
#if WITH_EDITOR
CK_DECLARE_CUSTOM_FORMATTER(CKCORE_API, EMapChangeType);
#endif

// --------------------------------------------------------------------------------------------------------------------

CK_DECLARE_CUSTOM_FORMATTER_NAMESPACE(CKCORE_API, ctti::detail::cstring, cstring);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // a simple class to standardize the way we print context
    template <typename T>
    struct FContext
    {
    public:
        CK_ENABLE_CUSTOM_FORMATTER(FContext<T>);

    public:
        explicit FContext(T InContext)
            : _Context(InContext)
        {
        }

    private:
        T _Context;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    auto Context(T&& InContext) -> FContext<T>
    {
        return FContext<T>{InContext};
    }
}

CK_DEFINE_CUSTOM_FORMATTER_T(ck::FContext<T>, [](const auto& InObj)
{
    return ck::Format(TEXT("\nContext: [{}]"), InObj._Context);
});

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace ck_format_detail
    {
        template <typename T>
        constexpr auto&& ArgsForward(T&& InType)
        {
            using decayed_t = std::decay_t<T>;
            using original_t = std::remove_const_t<decayed_t>;

            if constexpr (std::is_pointer_v<decayed_t> && NOT std::is_void_v<std::remove_pointer_t<std::remove_cv_t<original_t>>> && NOT std::is_same_v<decayed_t, const wchar_t*>)
            {
                return ForwarderForPointers(InType);
            }
            else
            {
                return InType;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------