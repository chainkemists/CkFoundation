#include "CkEntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

// -----------------------------------------------------------------------------------------------------------

auto
    UCk_EntityScript_UE::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    return DoConstruct(InHandle);
}

auto
    UCk_EntityScript_UE::
    ContinueConstruction(
        FCk_Handle InHandle)
    -> void
{
    DoContinueConstruction(InHandle);
}

auto
    UCk_EntityScript_UE::
    BeginPlay()
    -> void
{
    const auto ScriptEntity = DoGet_ScriptEntity();
    DoBeginPlay(ScriptEntity);
}

auto
    UCk_EntityScript_UE::
    EndPlay()
    -> void
{
    const auto ScriptEntity = DoGet_ScriptEntity();
    DoEndPlay(ScriptEntity);
}

auto
    UCk_EntityScript_UE::
    GetPrimaryAssetId() const
        -> FPrimaryAssetId
{
    return FPrimaryAssetId{FPrimaryAssetType{_AssetRegistryCategory}, GetFName()};
}

auto
    UCk_EntityScript_UE::
    DoGet_ScriptEntity() const
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(_AssociatedEntity),
        TEXT("ScriptEntity is [{}]. {}\n{}"),
        _AssociatedEntity,
        _AssociatedEntity.Has<ck::FTag_EntityScript_HasEndedPlay>()
        ? TEXT("The Script has ALREADY EndedPlay. Ensure that no lingering Latent Actions are still being performed")
        : TEXT("It's possible that this was not set correctly by the Processor that spawned the entity script"),
        ck::Context(this))
    { return _AssociatedEntity; }

    return _AssociatedEntity;
}

auto
    UCk_EntityScript_UE::
    DoFinishConstruction()
    -> void
{
    if (ck::Is_NOT_Valid(_AssociatedEntity))
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has<ck::FTag_EntityScript_BeginPlay>()),
        TEXT("Called Finish Construction on EntityScript [{}] that has been ASKED to BeginPlay"), _AssociatedEntity)
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has<ck::FTag_EntityScript_HasBegunPlay>()),
        TEXT("Called Finish Construction on EntityScript [{}] that has ALREADY BegunPlay"), _AssociatedEntity)
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has<ck::FTag_EntityScript_ContinueConstruction>()),
        TEXT("Called Finish Construction on EntityScript [{}] that was NOT ONGOING Construction"), _AssociatedEntity)
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has<ck::FTag_EntityScript_FinishConstruction>()),
        TEXT("Called Finish Construction on EntityScript [{}] that has already FINISHED Construction"), _AssociatedEntity)
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has<ck::FTag_EntityScript_HasEndedPlay>()),
        TEXT("Called Finish Construction on EntityScript [{}] that has ALREADY EndedPlay"), _AssociatedEntity)
    { return; }

    _AssociatedEntity.Add<ck::FTag_EntityScript_FinishConstruction>();
}

auto
    UCk_EntityScript_UE::
    ShowReplicationInEditor() const
    -> bool
{
    return true;
}

auto
    UCk_EntityScript_UE::
    Get_AreSpawnParamsMatching(
        const FInstancedStruct& InClientSpawnParams,
        const UCk_EntityScript_UE* InConstructedScript) const
    -> bool
{
    if (ck::Is_NOT_Valid(InClientSpawnParams)
        || ck::Is_NOT_Valid(InConstructedScript, ck::IsValid_Policy_NullptrOnly{}))
    { return true; }

    const auto* SpawnParamsStruct = InClientSpawnParams.GetScriptStruct();
    if (ck::Is_NOT_Valid(SpawnParamsStruct, ck::IsValid_Policy_NullptrOnly{}))
    { return true; }

    const auto* SpawnParamsData = InClientSpawnParams.GetMemory();

    for (TFieldIterator<FProperty> PropIt(SpawnParamsStruct, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
    {
        const auto* SpawnParamsProp = *PropIt;

        const auto& PropertyName =
            UCk_Utils_Reflection_UE::Get_SanitizedUserDefinedPropertyName(SpawnParamsProp);

        const auto* ScriptProp = UCk_Utils_Reflection_UE::Get_PropertyBySanitizedName(
            const_cast<UCk_EntityScript_UE*>(InConstructedScript), PropertyName);

        if (ck::Is_NOT_Valid(ScriptProp, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        const auto* SpawnParamAddr =
            SpawnParamsProp->ContainerPtrToValuePtr<uint8>(SpawnParamsData);

        const auto* ScriptPropAddr =
            ScriptProp->ContainerPtrToValuePtr<uint8>(InConstructedScript);

        if (NOT SpawnParamsProp->Identical(SpawnParamAddr, ScriptPropAddr))
        { return false; }
    }

    return true;
}

auto
    UCk_EntityScript_UE::
    Get_EffectiveReplication(
        const FInstancedStruct& InSpawnParams) const
    -> ECk_Replication
{
    return Get_Replication();
}

// -----------------------------------------------------------------------------------------------------------
