#pragma once

#include "CkUnrealComponent_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "Components/ActorComponent.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_UnrealComponent_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_UnrealComponent_Params = FCk_Fragment_UnrealComponent_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_UnrealComponent_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_UnrealComponent_TickViaProcessor);
    CK_DEFINE_ECS_TAG(FTag_UnrealComponent_IsScene);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKUNREALCOMPONENT_API FFragment_UnrealComponent_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_UnrealComponent_Current);

        friend class FProcessor_UnrealComponent_Setup;
        friend class FProcessor_UnrealComponent_PushTransform;
        friend class FProcessor_UnrealComponent_Tick;
        friend class FProcessor_UnrealComponent_EndPlay;
        friend class ::UCk_Utils_UnrealComponent_UE;

    private:
        TStrongObjectPtr<UActorComponent> _Component;
        FCk_Handle _OwningEntity;

    public:
        CK_PROPERTY_GET(_Component);
        CK_PROPERTY_GET(_OwningEntity);

        CK_DEFINE_CONSTRUCTORS(FFragment_UnrealComponent_Current, _OwningEntity);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_AND_UTILS(
        RecordOfUnrealComponents_Utils,
        FFragment_RecordOfUnrealComponents,
        FCk_Handle_UnrealComponent);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKUNREALCOMPONENT_API,
        UnrealComponent_OnAdded,
        FCk_Delegate_UnrealComponent_OnAdded,
        FCk_Handle_UnrealComponent);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKUNREALCOMPONENT_API,
        UnrealComponent_OnRemoved,
        FCk_Delegate_UnrealComponent_OnRemoved,
        FCk_Handle_UnrealComponent);
}

// --------------------------------------------------------------------------------------------------------------------
