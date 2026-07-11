#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkLabel/CkLabel_Fragment.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment_Data.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <Serialization/Archive.h>

#include "CkVelocity_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UMovementComponent;
class UCk_Utils_Velocity_UE;
class UCk_Utils_BulkVelocityModifier_UE;

namespace ck { class FSnapshotContext; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Velocity_NeedsSetup);

    CK_DEFINE_ECS_TAG(FTag_VelocityChannel);
    CK_DEFINE_ECS_TAG(FTag_VelocityModifier);
    CK_DEFINE_ECS_TAG(FTag_VelocityModifier_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_BulkVelocityModifier_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_BulkVelocityModifier_GlobalScope);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Velocity_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Velocity_Params);

    public:
        using ParamsType = FCk_Fragment_Velocity_ParamsData;
        using IsSnapshotable = void;

        // Tier-C: hand-rolled — the wrapped reflected structs expose their fields only through
        // getters + the generated constructors, so round-trip through locals.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/) -> void
        {
            auto Coordinates = static_cast<uint8>(_Params.Get_Coordinates());
            auto StartingVelocity = _Params.Get_StartingVelocity();
            auto HasMinSpeed = _Params.Get_VelocityMinMax().Get_HasMinSpeed();
            auto MinSpeed    = _Params.Get_VelocityMinMax().Get_MinSpeed();
            auto HasMaxSpeed = _Params.Get_VelocityMinMax().Get_HasMaxSpeed();
            auto MaxSpeed    = _Params.Get_VelocityMinMax().Get_MaxSpeed();

            InAr << Coordinates;
            InAr << StartingVelocity;
            InAr << HasMinSpeed;
            InAr << MinSpeed;
            InAr << HasMaxSpeed;
            InAr << MaxSpeed;

            if (InAr.IsLoading())
            {
                _Params = ParamsType{static_cast<ECk_LocalWorld>(Coordinates), StartingVelocity}
                    .Set_VelocityMinMax(FCk_Fragment_Velocity_MinMax_ParamsData{HasMinSpeed, MinSpeed, HasMaxSpeed, MaxSpeed});
            }
        }

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Velocity_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_MovementComponent
    {
    public:
        CK_GENERATED_BODY(FFragment_MovementComponent);

    private:
        TWeakObjectPtr<UMovementComponent> _MovementComponent;

    public:
        CK_PROPERTY_GET(_MovementComponent);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_MovementComponent, _MovementComponent);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Velocity_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Velocity_Current);

    public:
        friend class UCk_Utils_Velocity_UE;
        friend class FProcessor_Velocity_Setup;
        friend class FProcessor_Velocity_Clamp;
        friend class FProcessor_VelocityModifier_Setup;
        friend class FProcessor_VelocityModifier_EndPlay;

    public:
        using IsSnapshotable = void;

        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/) -> void
        {
            InAr << _CurrentVelocity;
        }

    private:
        FVector _CurrentVelocity = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_CurrentVelocity);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Velocity_Current, _CurrentVelocity);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Velocity_MinMax
    {
    public:
        CK_GENERATED_BODY(FFragment_Velocity_MinMax);

    public:
        friend class UCk_Utils_Velocity_UE;

    public:
        using IsSnapshotable = void;

        // Tier-C: TOptional has no FArchive operator<< — persist a set-flag + value per speed bound.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/) -> void
        {
            auto HasMin = _MinSpeed.IsSet();
            auto Min    = _MinSpeed.Get(0.0f);
            auto HasMax = _MaxSpeed.IsSet();
            auto Max    = _MaxSpeed.Get(0.0f);

            InAr << HasMin;
            InAr << Min;
            InAr << HasMax;
            InAr << Max;

            if (InAr.IsLoading())
            {
                _MinSpeed = HasMin ? TOptional<float>{Min} : TOptional<float>{};
                _MaxSpeed = HasMax ? TOptional<float>{Max} : TOptional<float>{};
            }
        }

    private:
        TOptional<float> _MinSpeed;
        TOptional<float> _MaxSpeed;

    public:
        CK_PROPERTY_GET(_MinSpeed);
        CK_PROPERTY_GET(_MaxSpeed);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_BulkVelocityModifier_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_BulkVelocityModifier_Params);

    public:
        using ParamsType = FCk_Fragment_BulkVelocityModifier_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_BulkVelocityModifier_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_BulkVelocityModifier_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_BulkVelocityModifier_Requests);

    public:
        friend class FProcessor_BulkVelocityModifier_HandleRequests;
        friend class UCk_Utils_BulkVelocityModifier_UE;

    public:
        using RequestStartAffectingEntityType = FCk_Request_BulkVelocityModifier_AddTarget;
        using RequestStopAffectingEntityType = FCk_Request_BulkVelocityModifier_RemoveTarget;

        using RequestType = std::variant<RequestStartAffectingEntityType, RequestStopAffectingEntityType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER_TRANSIENT(FFragment_Velocity_Target, FCk_Handle_Transform);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfVelocityModifiers, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfBulkVelocityModifiers, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfVelocityChannels, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_BulkVelocityModifier_Requests);
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKPHYSICS_API FCk_RepData_Velocity
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_Velocity);

    UPROPERTY()
    FVector Value = FVector::ZeroVector;
};

namespace ck
{
    using FFragment_ContainerRef_Velocity = TFragment_ContainerEntryRef<FCk_RepData_Velocity>;
}

// --------------------------------------------------------------------------------------------------------------------
