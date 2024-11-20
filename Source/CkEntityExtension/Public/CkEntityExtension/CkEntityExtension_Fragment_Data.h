#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include <GameplayTagContainer.h>

#include "CkEntityExtension_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ExtensionAwareness : uint8
{
    // If not found, an ensure is triggered
    None,
    // Wait for and bind to the first available Entity
    WaitAndBind_OnFirstAvailable,
    // Wait for and bind to the first available Entity, repeatedly rebinding as extensions are added or removed
    WaitAndBind_Repeatedly,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ExtensionAwareness);

// --------------------------------------------------------------------------------------------------------------------

// NOTE: this is defined in CkRecord_Fragment_Data to avoid circular dependency
//USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
//struct CKENTITYEXTENSION_API FCk_Handle_EntityExtension : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_EntityExtension); };
//CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_EntityExtension);

// --------------------------------------------------------------------------------------------------------------------

// NOTE: this is defined in Fragment.h due to the dependency on FCk_Handle_EntityExtension
//DECLARE_DYNAMIC_DELEGATE_TwoParams(
//    FCk_Delegate_EntityExtension_OnExtensionAdded,
//    FCk_Handle_EntityExtension, InExtensionOwner,
//    FCk_Handle, InEntityAsExtension);
//
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
//    FCk_Delegate_EntityExtension_OnExtensionAdded_MC,
//    FCk_Handle_EntityExtension, InExtensionOwner,
//    FCk_Handle, InEntityAsExtension);

// --------------------------------------------------------------------------------------------------------------------
