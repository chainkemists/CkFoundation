#include "CkEntityBridge_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkEntityBridge/CkEntityBridge_ConstructionScript.h"

#include <Misc/DataValidation.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityBridge_Config_Base_PDA::
    Get_EntityConstructionScript() const
    -> UCk_Entity_ConstructionScript_PDA*
{
    return DoGet_EntityConstructionScript();
}

auto
    UCk_EntityBridge_Config_Base_PDA::
    DoGet_EntityConstructionScript() const
    -> UCk_Entity_ConstructionScript_PDA*
{
    CK_TRIGGER_ENSURE(TEXT("DoGet_EntityConstructionScript was not Overriden in [{}]"), this);
    return {};
}

auto
    UCk_EntityBridge_Config_PDA::
    DoGet_EntityConstructionScript() const
    -> UCk_Entity_ConstructionScript_PDA*
{
    return _EntityConstructionScript;
}

#if WITH_EDITOR
auto
    UCk_EntityBridge_Config_PDA::
    IsDataValid(
        FDataValidationContext& Context) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(Context);

    if (IsTemplate())
    { return Result; }

    if (ck::Is_NOT_Valid(_EntityConstructionScript))
    {
        Context.AddError(FText::FromString(ck::Format_UE(TEXT("Entity Bridge Config [{}] is missing an Entity Construction Script"), this)));

        Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
    }

    return Result;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityBridge_Config_WithActor_PDA::
    DoGet_EntityConstructionScript() const
    -> UCk_Entity_ConstructionScript_PDA*
{
    return _EntityConstructionScript;
}

#if WITH_EDITOR
auto
    UCk_EntityBridge_Config_WithActor_PDA::
    IsDataValid(
        FDataValidationContext& Context) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(Context);

    if (IsTemplate())
    { return Result; }

    if (ck::Is_NOT_Valid(_EntityConstructionScript))
    {
        Context.AddError(FText::FromString(ck::Format_UE(TEXT("Entity Bridge Config (With Actor) [{}] is missing an Entity Construction Script"), this)));

        Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
    }

    return Result;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityBridge_Config_Base_PDA::
    Build(
        const FCk_Handle& InEntity,
        const FInstancedStruct& InOptionalParams,
        const UObject* InOptionalObjectConstructionScript) const
    -> void
{
    DoBuild(InEntity, InOptionalParams, InOptionalObjectConstructionScript);
}

auto
    UCk_EntityBridge_Config_Base_PDA::
    DoBuild(
        FCk_Handle InHandle,
        const FInstancedStruct& InOptionalParams,
        const UObject* InOptionalObjectConstructionScript) const
    -> void
{
    const auto& EntityConstructionScript = Get_EntityConstructionScript();
    CK_ENSURE_IF_NOT(ck::IsValid(EntityConstructionScript), TEXT("INVALID ConstructionScript in EntityConfig [{}]"), GetPathName())
    { return; }

    EntityConstructionScript->Construct(InHandle, InOptionalParams, InOptionalObjectConstructionScript);
}

// --------------------------------------------------------------------------------------------------------------------
