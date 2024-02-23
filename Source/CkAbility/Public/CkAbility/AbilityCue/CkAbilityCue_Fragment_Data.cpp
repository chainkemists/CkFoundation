#include "CkAbilityCue_Fragment_Data.h"

#include "CkAbility/CkAbility_Log.h"

#include "CkCore/IO/CkIO_Utils.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

#include "CkEntityBridge/CkEntityBridge_Fragment_Data.h"

#include "Engine/AssetManager.h"
#include "Engine/ObjectLibrary.h"

#include "UObject/ObjectSaveContext.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AbilityCue_Aggregator_PDA::
    Request_Populate() -> void
{
    _AbilityCues.Empty();

    const auto& ThisPath = this->GetPathName();
    const auto& ExtractedPath = UCk_Utils_IO_UE::Get_ExtractPath(ThisPath);

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    FARFilter Filter;
    Filter.ClassPaths.Add(FTopLevelAssetPath{ConfigType::StaticClass()});
    Filter.PackagePaths.Add(*ExtractedPath);
    Filter.bRecursiveClasses = true;

    FARCompiledFilter CompiledFilter;
    IAssetRegistry::Get()->CompileFilter(Filter, CompiledFilter);

    AssetRegistryModule.Get().EnumerateAssets(CompiledFilter, [&](const FAssetData& InAssetData)
    {
        // TODO: See if we can avoid resolving the Object
        const auto ResolvedObject = InAssetData.GetSoftObjectPath().TryLoad();
        const auto Object = Cast<UCk_AbilityCue_Config_PDA>(ResolvedObject);

        if (ck::Is_NOT_Valid(Object))
        {
            ck::ability::Error(TEXT("Unable to Cast Cue [{}] to [{}].{}"),
                ResolvedObject, ck::Get_RuntimeTypeToString<UCk_AbilityCue_Config_PDA>(), ck::Context(this));
            return false;
        }

        _AbilityCues.Add(Object->Get_CueName(), InAssetData.GetSoftObjectPath());
        return true;
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AbilityCue_Aggregator_PDA::
    PreSave(
        FObjectPreSaveContext ObjectSaveContext)
    -> void
{
    Request_Populate();

    Super::PreSave(ObjectSaveContext);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AbilityCue_Params::
    NetSerialize(
        FArchive& Ar,
        UPackageMap* Map,
        bool& bOutSuccess)
    -> bool
{
    enum ERepFlag
    {
        Rep_Location = 0,
        Rep_Normal,
        Rep_Instigator,
        Rep_EffectCauser,

        Rep_Max
    };

    auto RepBits = uint16{0};
    if (Ar.IsSaving())
    {
        if (NOT _Location.IsNearlyZero())
        {
            _QuantizedLocation = _Location;
            RepBits |= (1 << Rep_Location);
        }
        if (NOT _Normal.IsNearlyZero())
        {
            _QuantizedNormal = _Normal;
            RepBits |= (1 << Rep_Normal);
        }
        if (ck::IsValid(_Instigator))
        {
            if (_Instigator.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>())
            {
                _Instigator_RepObj = _Instigator.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
                RepBits |= (1 << Rep_Instigator);
            }
        }
        if (ck::IsValid(_EffectCauser))
        {
            if (_EffectCauser.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>())
            {
                _EffectCauser_RepObj = _EffectCauser.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
                RepBits |= (1 << Rep_EffectCauser);
            }
        }
    }

    Ar.SerializeBits(&RepBits, Rep_Max);

    if (RepBits & (1 << Rep_Location))
    {
        Ar << _QuantizedLocation;
        if (Ar.IsLoading())
        { _Location = _QuantizedLocation; }
    }
    if (RepBits & (1 << Rep_Normal))
    {
        Ar << _QuantizedNormal;
        if (Ar.IsLoading())
        { _Normal = _QuantizedNormal; }
    }
    if (RepBits & (1 << Rep_Instigator))
    {
        Ar << _Instigator_RepObj;
        if (Ar.IsLoading())
        {
            CK_ENSURE_IF_NOT(ck::IsValid(_Instigator_RepObj),
                TEXT("Instigator RepObj was [{}] even though RepBits tell us that it WAS replicated. Unable to set the Instigator."),
                _Instigator_RepObj)
            { }
            else
            { _Instigator = _Instigator_RepObj->Get_AssociatedEntity(); }
        }
    }
    if (RepBits & (1 << Rep_EffectCauser))
    {
        Ar << _EffectCauser_RepObj;
        if (Ar.IsLoading())
        {
            CK_ENSURE_IF_NOT(ck::IsValid(_EffectCauser_RepObj),
                TEXT("EffectCauser RepObj was [{}] even though RepBits tell us that it WAS replicated. Unable to set the EffectCauser."),
                _EffectCauser_RepObj)
            { }

            _EffectCauser = _EffectCauser_RepObj->Get_AssociatedEntity();
        }
    }

    bOutSuccess = true;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityCue_Spawn::
    FCk_Request_AbilityCue_Spawn(
        const FGameplayTag& InAbilityCueName,
        UObject* InWorldContextObject)
    : _AbilityCueName(InAbilityCueName)
    , _WorldContextObject(InWorldContextObject)
{
}

// --------------------------------------------------------------------------------------------------------------------
