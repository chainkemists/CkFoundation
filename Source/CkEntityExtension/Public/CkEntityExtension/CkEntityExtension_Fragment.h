#pragma once

#include "CkEntityExtension_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkEntityExtension_Fragment.generated.h"

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

    // NOTE: this is defined in CkRecord_Fragment to avoid circular dependency
    // CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfEntityExtensions, FCk_Handle_EntityExtension);
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKENTITYEXTENSION_API UCk_Fragment_EntityExtension_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_EntityExtension_Rep);

public:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>&) const -> void override;

private:
    UFUNCTION()
    void
    OnRep_ExtensionOwnerChanged();

private:
    UPROPERTY(ReplicatedUsing = OnRep_ExtensionOwnerChanged)
    FCk_Handle _NewExtensionOwner;
};
