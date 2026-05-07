#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "Components/ActorComponent.h"

#include "CkUnrealComponent_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKUNREALCOMPONENT_API FCk_Handle_UnrealComponent : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_UnrealComponent); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_UnrealComponent);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UnrealComponent_TickPolicy : uint8
{
    DoNotTick,
    TickViaProcessor
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UnrealComponent_TickPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKUNREALCOMPONENT_API FCk_Fragment_UnrealComponent_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_UnrealComponent_ParamsData);

public:
    FCk_Fragment_UnrealComponent_ParamsData() = default;

    explicit FCk_Fragment_UnrealComponent_ParamsData(TSubclassOf<UActorComponent> InComponentClass);

    explicit FCk_Fragment_UnrealComponent_ParamsData(UActorComponent* InComponentArchetype);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UActorComponent> _ComponentClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UActorComponent> _ComponentArchetype;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_UnrealComponent_TickPolicy _TickPolicy = ECk_UnrealComponent_TickPolicy::DoNotTick;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _DebugName;

public:
    CK_PROPERTY_GET(_ComponentClass);
    CK_PROPERTY_GET(_ComponentArchetype);
    CK_PROPERTY(_TickPolicy);
    CK_PROPERTY(_DebugName);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_UnrealComponent_OnAdded,
    FCk_Handle_UnrealComponent, InUnrealComponent);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_UnrealComponent_OnRemoved,
    FCk_Handle_UnrealComponent, InUnrealComponent);

// --------------------------------------------------------------------------------------------------------------------
