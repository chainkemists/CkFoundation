#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkJolt/Character/CkJoltCharacter_Fragment_Data.h"

#include <variant>

#include <CoreMinimal.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_JoltCharacter_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_JoltCharacter_Setup;
    class FProcessor_JoltCharacter_HandleRequests;
    class FProcessor_JoltCharacter_PreStep;
    class FProcessor_JoltCharacter_EndPlay;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_JoltCharacter_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_JoltCharacter_Params = FCk_JoltCharacter_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    // _Character owns the CharacterVirtual for the entity's lifetime; the FJoltWorld registry holds a
    // NON-owning raw pointer alongside. The _Mirror fields are refreshed by DoApplyCharacterPoses_GameThread;
    // the _Pending* fields are the intent inbox HandleRequests writes and PreStep drains before the step.
    struct CKJOLT_API FFragment_JoltCharacter_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_JoltCharacter_Current);

    public:
        friend class FProcessor_JoltCharacter_Setup;
        friend class FProcessor_JoltCharacter_HandleRequests;
        friend class FProcessor_JoltCharacter_PreStep;
        friend class FProcessor_JoltCharacter_EndPlay;
        friend class ::UCk_Utils_JoltCharacter_UE;

    private:
        JPH::Ref<JPH::CharacterVirtual> _Character;
        uint16                          _ObjectLayer = 0;
        ECk_JoltCharacter_GroundState   _GroundStateMirror = ECk_JoltCharacter_GroundState::InAir;
        FVector                         _GroundNormalMirror = FVector::ZeroVector;
        FVector                         _GroundVelocityMirror = FVector::ZeroVector;
        FVector                         _PendingMoveVelocity = FVector::ZeroVector;
        float                           _PendingJumpVelocity = 0.0f;
        bool                            _HasPendingJump = false;

    public:
        CK_PROPERTY_GET(_Character);
        CK_PROPERTY_GET(_ObjectLayer);
        CK_PROPERTY(_GroundStateMirror);
        CK_PROPERTY(_GroundNormalMirror);
        CK_PROPERTY(_GroundVelocityMirror);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKJOLT_API FFragment_JoltCharacter_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_JoltCharacter_Requests);

    public:
        friend class FProcessor_JoltCharacter_HandleRequests;
        friend class ::UCk_Utils_JoltCharacter_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_JoltCharacter_Move,
            FCk_Request_JoltCharacter_Jump,
            FCk_Request_JoltCharacter_Teleport>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_JoltCharacter_Requests);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKJOLT_API,
        OnJoltCharacterGroundStateChanged,
        FCk_Delegate_JoltCharacter_OnGroundStateChanged,
        FCk_Handle_JoltCharacter,
        ECk_JoltCharacter_GroundState);
}

// --------------------------------------------------------------------------------------------------------------------
