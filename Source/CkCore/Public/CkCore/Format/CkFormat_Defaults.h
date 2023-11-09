#pragma once

#include "CkCore/Format/CkFormat.h"

#include <Templates/SubclassOf.h>

#include <GameFramework/Actor.h>
#include <GameplayTagContainer.h>
#include <CoreMinimal.h>
#include <InputCoreTypes.h>
#include <Framework/Commands/InputChord.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_format_default::details
{
    inline const auto Invalid_FProperty = FProperty {{}, NAME_None, {}};
}

CK_DEFINE_CUSTOM_FORMATTER(FString, [InObj]() { return *InObj; });
CK_DEFINE_CUSTOM_FORMATTER(FName,   [&]() { return InObj.ToString(); });
CK_DEFINE_CUSTOM_FORMATTER(FText,   [&]() { return InObj.ToString(); });
CK_DEFINE_CUSTOM_FORMATTER(FGuid,   [&]() { return InObj.ToString(); });
CK_DEFINE_CUSTOM_FORMATTER(FKey,   [&]() { return InObj.ToString(); });
CK_DEFINE_CUSTOM_FORMATTER(FInputChord,   [&]() { return InObj.GetInputText(); });
CK_DEFINE_CUSTOM_FORMATTER(FSoftObjectPath,   [&]() { return InObj.GetAssetName(); });

CK_DEFINE_CUSTOM_FORMATTER(FRandomStream, [&]()
{
    return ck::Format
    (
        TEXT("Initial Seed: [{}], Current Seed: [{}]"),
        InObj.GetInitialSeed(),
        InObj.GetCurrentSeed()
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FRotator, [&]()
{
    return ck::Format
    (
        TEXT("P{:.2f}, Y:{:.2f}, R:{:.2f}"),
        InObj.Pitch,
        InObj.Yaw,
        InObj.Roll
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FVector, [&]()
{
    return ck::Format
    (
        TEXT("X:{:.2f}, Y:{:.2f}, Z:{:.2f}"),
        InObj.X,
        InObj.Y,
        InObj.Z
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FVector2D, [&]()
{
    return ck::Format
    (
        TEXT("X:{:.2f}, Y:{:.2f}"),
        InObj.X,
        InObj.Y
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FQuat, [&]()
{
    return ck::Format
    (
        TEXT("X:{:.2f}, Y:{:.2f}, Z:{:.2f}, W:{:.2f}"),
        InObj.X,
        InObj.Y,
        InObj.Z,
        InObj.W
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FTransform, [&]()
{
    return ck::Format
    (
        TEXT("T:[{}] R:[{}] S:[{}]"),
        InObj.GetTranslation(),
        InObj.GetRotation(),
        InObj.GetScale3D()
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FTimespan, [&]()
{
    return ck::Format
    (
        TEXT("Days:[{}] Hours:[{}] Minutes:[{}] Seconds:[{}]"),
        InObj.GetDays(),
        InObj.GetHours(),
        InObj.GetMinutes(),
        InObj.GetSeconds()
    );
});

CK_DEFINE_CUSTOM_FORMATTER(FGameplayTag, [&]()
{
    if (ck::IsValid(InObj))
    {
        return ck::Format(TEXT("{}"), InObj.ToString());
    }

    return ck::Format(TEXT("TAG_NOT_SET"));
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(UObject, [&]()
{
    //  TODO: drive the verbosity through an Editor setting
    return UCk_Utils_Debug_UE::Get_DebugName(&InObj, ECk_DebugName_Verbosity::ShortName);
});

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(UObject, []() -> const UObject&
{
    return *UObject::StaticClass()->GetDefaultObject<UObject>();
});

CK_DEFINE_CUSTOM_FORMATTER(UActorComponent, [&]()
{
    return UCk_Utils_Debug_UE::Get_DebugName(&InObj);
});

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(UActorComponent, []() -> const UActorComponent&
{
    return *UActorComponent::StaticClass()->GetDefaultObject<UActorComponent>();
});

CK_DEFINE_CUSTOM_FORMATTER(AActor, [&]()
{
    return UCk_Utils_Debug_UE::Get_DebugName(&InObj);
});

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(AActor, []() -> const AActor&
{
    return *AActor::StaticClass()->GetDefaultObject<AActor>();
});

CK_DEFINE_CUSTOM_FORMATTER(UClass, [&]()
{
    return UCk_Utils_Debug_UE::Get_DebugName(&InObj);
});

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(UClass, []() -> const UClass&
{
    return *UClass::StaticClass()->GetDefaultObject<UClass>();
});

CK_DEFINE_CUSTOM_FORMATTER(UFunction, [&]()
{
    return ck::Format(TEXT("{}.{}[{}]"), InObj.GetOuter()->GetName(), InObj.GetName(), InObj.GetUniqueID());
});

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(UFunction, [&]() -> const UFunction&
{
    return *InObj;
});

CK_DEFINE_CUSTOM_FORMATTER(FProperty, [&]()
{
    return ck::Format(TEXT("{}[{}]"), InObj.GetName(), InObj.GetOffset_ForDebug());
});

CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(FProperty, []() -> const FProperty&
{
    return ck_format_default::details::Invalid_FProperty;
});

CK_DEFINE_CUSTOM_FORMATTER_T(TScriptInterface<T>, [&]()
{
    return ck::Format(TEXT("{}"), InObj.GetObject());
});

CK_DEFINE_CUSTOM_FORMATTER_T(TWeakObjectPtr<T>, [&]()
{
    return ck::Format(TEXT("{}"), InObj.Get());
});

CK_DEFINE_CUSTOM_FORMATTER_T(TSubclassOf<T>, [&]()
{
    return ck::Format(TEXT("{}"), InObj.Get());
});

CK_DEFINE_CUSTOM_FORMATTER_T(TObjectPtr<T>, [&]()
{
    return ck::Format(TEXT("{}"), InObj.Get());
});

CK_DEFINE_CUSTOM_FORMATTER_T(TOptional<T>, [&]()
{
    if (NOT ck::IsValid(InObj))
    { return ck::Format(TEXT("nullopt")); }

    return ck::Format(TEXT("{}"), *InObj);
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER_ENUM(EAppReturnType::Type);

// --------------------------------------------------------------------------------------------------------------------

#include "ctti/type_id.hpp"
CK_DEFINE_CUSTOM_FORMATTER(ctti::detail::cstring, [&]()
{
    return FString(InObj.size(), InObj.begin());
});

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

CK_DEFINE_CUSTOM_FORMATTER_T(ck::FContext<T>, [&]()
{
    return ck::Format(TEXT("\nContext: [{}]"), InObj._Context);
});
