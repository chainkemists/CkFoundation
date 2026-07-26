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

// NOTE: FCk_Handle_EntityExtension is declared in CkRecord_Fragment_Data.h (circular dependency),
// and FCk_Delegate_EntityExtension_OnExtensionAdded in CkEntityExtension_Fragment.h (it needs that handle).

// --------------------------------------------------------------------------------------------------------------------
