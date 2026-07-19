#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkJolt/StaticWorld/CkJoltStaticActor_Fragment.h"
#include "CkJolt/StaticWorld/CkJoltStaticActor_Fragment_Data.h"

#include "CkJoltStaticActor_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_JoltStaticActor"))
class CKJOLT_API UCk_Utils_JoltStaticActor_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_JoltStaticActor_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_JoltStaticActor);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|JoltStaticActor",
        DisplayName="[Ck][JoltStaticActor] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_JoltStaticActor
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|JoltStaticActor",
        DisplayName="[Ck][JoltStaticActor] Handle -> JoltStaticActor Handle",
        meta = (CompactNodeTitle = "<AsJoltStaticActor>", BlueprintAutocast))
    static FCk_Handle_JoltStaticActor
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid JoltStaticActor Handle",
        Category = "Ck|Utils|JoltStaticActor",
        meta = (CompactNodeTitle = "INVALID_JoltStaticActorHandle", Keywords = "make"))
    static FCk_Handle_JoltStaticActor
    Get_InvalidHandle() { return {}; };

public:
    // The source actor that contributed this entity's baked bodies. May be null once the actor has been
    // destroyed (attribution survives the actor's death via Get_SourceActorName).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltStaticActor",
              DisplayName="[Ck][JoltStaticActor] Get Source Actor")
    static AActor*
    Get_SourceActor(
        const FCk_Handle_JoltStaticActor& InJoltStaticActor);

    // The FName of the source actor, cached at bake time so it survives the actor's death.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltStaticActor",
              DisplayName="[Ck][JoltStaticActor] Get Source Actor Name")
    static FName
    Get_SourceActorName(
        const FCk_Handle_JoltStaticActor& InJoltStaticActor);

    // The number of baked static-world bodies currently attributed to this source actor.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltStaticActor",
              DisplayName="[Ck][JoltStaticActor] Get Num Bodies")
    static int32
    Get_NumBodies(
        const FCk_Handle_JoltStaticActor& InJoltStaticActor);
};

// --------------------------------------------------------------------------------------------------------------------
