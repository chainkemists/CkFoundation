#pragma once

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkSignal/CkSignal_Macros.h"

#include "CkEntityExtension_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// NOTE: see Data.h file for why this is defined here
DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_EntityExtension_OnExtensionAdded,
    FCk_Handle, InExtensionOwner,
    FCk_Handle_EntityExtension, InEntityAsExtension);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_EntityExtension_OnExtensionAdded_MC,
    FCk_Handle, InExtensionOwner,
    FCk_Handle_EntityExtension, InEntityAsExtension);

// --------------------------------------------------------------------------------------------------------------------

// NOTE: see Data.h file for why this is defined here
DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_EntityExtension_OnExtensionRemoved,
    FCk_Handle, InExtensionOwner,
    FCk_Handle_EntityExtension, InEntityAsExtension);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_EntityExtension_OnExtensionRemoved_MC,
    FCk_Handle, InExtensionOwner,
    FCk_Handle_EntityExtension, InEntityAsExtension);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_DummyStructToCompileInTheDelegates
{
    GENERATED_BODY()
};

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityExtension_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_EntityExtension_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityExtension_Params);

    private:
        FCk_Handle _ExtensionOwner;

    public:
        CK_PROPERTY_GET(_ExtensionOwner);

        CK_DEFINE_CONSTRUCTORS(FFragment_EntityExtension_Params, _ExtensionOwner);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKENTITYEXTENSION_API, OnEntityExtensionAdded,
        FCk_Delegate_EntityExtension_OnExtensionAdded_MC, FCk_Handle, FCk_Handle_EntityExtension);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKENTITYEXTENSION_API, OnEntityExtensionRemoved,
        FCk_Delegate_EntityExtension_OnExtensionRemoved_MC, FCk_Handle, FCk_Handle_EntityExtension);

    // --------------------------------------------------------------------------------------------------------------------

    // NOTE: this is defined in CkRecord_Fragment to avoid circular dependency
    // CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfEntityExtensions, FCk_Handle_EntityExtension);
}

// --------------------------------------------------------------------------------------------------------------------
