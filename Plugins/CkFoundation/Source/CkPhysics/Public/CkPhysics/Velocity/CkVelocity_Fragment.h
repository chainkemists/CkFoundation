#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkMacros/CkMacros.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment_Params.h"

#include "CkVelocity_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Velocity_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FCk_Tag_Velocity_Setup {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_Velocity_Params
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Velocity_Params);

    public:
        using ParamsType = FCk_Fragment_Velocity_ParamsData;

    public:
        FCk_Fragment_Velocity_Params() = default;
        explicit FCk_Fragment_Velocity_Params(ParamsType InParams);

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_Velocity_Current
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Velocity_Current);

    public:
        friend class FCk_Processor_Velocity_Setup;
        friend class UCk_Utils_Velocity_UE;

    public:
        FCk_Fragment_Velocity_Current() = default;
        explicit FCk_Fragment_Velocity_Current(
            FVector InVelocity);

    private:
        FVector _CurrentVelocity = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_CurrentVelocity);
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FCk_Processor_Velocity_Replicate; }

UCLASS(Blueprintable)
class CKPHYSICS_API UCk_Fragment_Velocity_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_Velocity_Rep);

public:
    friend class ck::FCk_Processor_Velocity_Replicate;

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
