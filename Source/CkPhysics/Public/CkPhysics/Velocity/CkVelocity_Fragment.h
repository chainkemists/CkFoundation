#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkLabel/CkLabel_Fragment.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment_Data.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkVelocity_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UMovementComponent;
class UCk_Utils_Velocity_UE;
class UCk_Utils_BulkVelocityModifier_UE;

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
        friend class FProcessor_VelocityModifier_Teardown;

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

    struct FFragment_Velocity_Target : public FFragment_EntityHolder
    {
        using FFragment_EntityHolder::FFragment_EntityHolder;
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfVelocityModifiers, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfBulkVelocityModifiers, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfVelocityChannels, FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID(ck::FFragment_MovementComponent, ck::IsValid_Policy_Default,
[&](const ck::FFragment_MovementComponent& InMovementComponent)
{
    return ck::IsValid(InMovementComponent.Get_MovementComponent());
});

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_Velocity_Replicate; }

UCLASS(Blueprintable)
class CKPHYSICS_API UCk_Fragment_Velocity_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_Velocity_Rep);

public:
    friend class ck::FProcessor_Velocity_Replicate;

public:
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UFUNCTION()
    void OnRep_Velocity();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Velocity)
    FVector _Velocity = FVector::ZeroVector;
};

// --------------------------------------------------------------------------------------------------------------------
