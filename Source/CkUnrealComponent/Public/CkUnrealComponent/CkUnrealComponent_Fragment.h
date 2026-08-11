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

    // Opt-out for FProcessor_UnrealComponent_PushTransform: the USceneComponent's world transform
    // is left to whoever owns it instead (typically Unreal physics after SetSimulatePhysics +
    // K2_DetachFromComponent, which the ECS push would otherwise overwrite).
    CK_DEFINE_ECS_TAG(FTag_UnrealComponent_TransformPushDisabled);

    // Set by Request_BakeIntoJoltStaticWorld: this component's geometry lives in the Jolt static
    // world and EndPlay must remove its bodies before destroying the component.
    CK_DEFINE_ECS_TAG(FTag_UnrealComponent_BakedIntoStaticWorld);

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
        // WEAK — lifetime owned by the CkCore ObjectPooling subsystem (DestroyOnRelease)
        TWeakObjectPtr<UActorComponent> _Component;
        FCk_Handle _OwningEntity;

    public:
        CK_PROPERTY_GET(_Component);
        CK_PROPERTY_GET(_OwningEntity);

        CK_DEFINE_CONSTRUCTORS(FFragment_UnrealComponent_Current, _OwningEntity);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Lives on the component-owning TRANSFORM entity. The push's change detection compares the owner's
    // fragment against this — never against the live USceneComponent — so externally-drifted components
    // are only re-authored when the OWNER actually moves, and a fragment write is delivered no matter
    // which frame position (main pass or pump pass) drained it. Gating the push on
    // FTag_Transform_Updated instead loses pump-drained one-shots outright (the tag is cleared by
    // Transform_Cleanup before the next main-pass push slot), and comparing against the component
    // stomps external drift (TransformPropagation.DirtyOwnersOnly).
    struct CKUNREALCOMPONENT_API FFragment_UnrealComponent_LastPushedTransform
    {
    public:
        CK_GENERATED_BODY(FFragment_UnrealComponent_LastPushedTransform);

        friend class FProcessor_UnrealComponent_Setup;
        friend class FProcessor_UnrealComponent_PushTransform;

    private:
        FTransform _Transform = FTransform::Identity;

    public:
        CK_PROPERTY_GET(_Transform);

        CK_DEFINE_CONSTRUCTORS(FFragment_UnrealComponent_LastPushedTransform, _Transform);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_AND_UTILS_TRANSIENT(
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
