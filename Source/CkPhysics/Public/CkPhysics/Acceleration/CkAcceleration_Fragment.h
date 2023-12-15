#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkLabel/CkLabel_Fragment.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment_Data.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkAcceleration_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Acceleration_UE;
class UCk_Utils_BulkAccelerationModifier_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Acceleration_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_AccelerationChannel);
    CK_DEFINE_ECS_TAG(FTag_AccelerationModifier);
    CK_DEFINE_ECS_TAG(FTag_AccelerationModifier_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_BulkAccelerationModifier_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_BulkAccelerationModifier_GlobalScope);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Acceleration_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Acceleration_Params);

    public:
        using ParamsType = FCk_Fragment_Acceleration_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Acceleration_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Acceleration_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Acceleration_Current);

    public:
        friend class UCk_Utils_Acceleration_UE;
        friend class FProcessor_Acceleration_Setup;
        friend class FProcessor_AccelerationModifier_Setup;
        friend class FProcessor_AccelerationModifier_Teardown;

    private:
        FVector _CurrentAcceleration = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_CurrentAcceleration);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Acceleration_Current, _CurrentAcceleration);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_BulkAccelerationModifier_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_BulkAccelerationModifier_Params);

    public:
        using ParamsType = FCk_Fragment_BulkAccelerationModifier_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_BulkAccelerationModifier_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_BulkAccelerationModifier_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_BulkAccelerationModifier_Requests);

    public:
        friend class FProcessor_BulkAccelerationModifier_HandleRequests;
        friend class UCk_Utils_BulkAccelerationModifier_UE;

    public:
        using RequestStartAffectingEntityType = FCk_Request_BulkAccelerationModifier_AddTarget;
        using RequestStopAffectingEntityType = FCk_Request_BulkAccelerationModifier_RemoveTarget;

        using RequestType = std::variant<RequestStartAffectingEntityType, RequestStopAffectingEntityType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_Acceleration_Target : public FFragment_EntityHolder
    {
        using FFragment_EntityHolder::FFragment_EntityHolder;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfAccelerationModifiers : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfBulkAccelerationModifiers : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfAccelerationChannels : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_Acceleration_Replicate; }

UCLASS(Blueprintable)
class CKPHYSICS_API UCk_Fragment_Acceleration_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_Acceleration_Rep);

public:
    friend class ck::FProcessor_Acceleration_Replicate;

public:
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UFUNCTION()
    void OnRep_Acceleration();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Acceleration)
    FVector _Acceleration = FVector::ZeroVector;
};

// --------------------------------------------------------------------------------------------------------------------
