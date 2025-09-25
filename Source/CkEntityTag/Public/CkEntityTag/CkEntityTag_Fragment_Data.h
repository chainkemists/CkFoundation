#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameplayTagContainer.h>

#include "CkEntityTag_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Not yet useful for this feature
//USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
//struct CKENTITYTAG_API FCk_Handle_EntityTag : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_EntityTag); };
//CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_EntityTag);

//--------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EntityTagUpdate : uint8
{
    Added,
    Removed,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityTagUpdate);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_Fragment_EntityTag_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_EntityTag_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories="EntityTag"))
    FName _Tag;

public:
    CK_PROPERTY_GET(_Tag);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_EntityTag_ParamsData, _Tag);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_EntityTag_OnTagUpdated,
    FCk_Handle, InOwner,
    FName, InTagName,
    ECk_EntityTagUpdate, InUpdateType);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_EntityTag_OnTagUpdated_MC,
    FCk_Handle, InOwner,
    FName, InTagName,
    ECk_EntityTagUpdate, InUpdateType);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_EntityTag_OnGameplayTagUpdated,
    FCk_Handle, InOwner,
    FGameplayTag, InTagName,
    ECk_EntityTagUpdate, InUpdateType);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_EntityTag_OnGameplayTagUpdated_MC,
    FCk_Handle, InOwner,
    FGameplayTag, InTagName,
    ECk_EntityTagUpdate, InUpdateType);

// --------------------------------------------------------------------------------------------------------------------