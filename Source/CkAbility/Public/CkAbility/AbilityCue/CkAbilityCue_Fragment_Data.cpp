#include "CkAbilityCue_Fragment_Data.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/Settings/CkAbility_Settings.h"

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

#include <Engine/AssetManager.h>

#if WITH_EDITOR
#include <Editor.h>
#endif

#include <UObject/ObjectSaveContext.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AbilityCue_Aggregator_PDA::
    Request_Populate() -> void
{
    if (IsTemplate())
    { return; }

#if WITH_EDITOR
    if (NOT _AbilityCues.IsEmpty())
    {
        if (NOT GEditor->IsPlaySessionInProgress())
        {
            _AbilityCues.Empty();
            _AbilityCueConfigs.Empty();

            // for an unknown reason, we need to wait for a few frames for the above clearing of the arrays to propagate to the DataAsset
            // before we re-populate it - there is possibly a function that must be called to force the propagation to happen immediately

            const auto& TimerManager = GEditor->GetTimerManager();
            auto TempTimerHandle = FTimerHandle{};
            TimerManager->SetTimer(TempTimerHandle, [this]()
            {
                Request_Populate();
            }, 1.0f, false);

            return;
        }
    }
#endif

    const auto& ThisPath = this->GetPathName();
    const auto& ExtractedPath = UCk_Utils_IO_UE::Get_ExtractPath(ThisPath);

    {
        const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

        FARFilter Filter;
        Filter.ClassPaths.Add(FTopLevelAssetPath{ConfigType::StaticClass()});
        Filter.PackagePaths.Add(*ExtractedPath);
        Filter.bRecursiveClasses = true;
        Filter.bRecursivePaths = true;
        Filter.bIncludeOnlyOnDiskAssets = true;

        FARCompiledFilter CompiledFilter;
        IAssetRegistry::Get()->CompileFilter(Filter, CompiledFilter);

        AssetRegistryModule.Get().EnumerateAssets(CompiledFilter, [&](const FAssetData& InAssetData)
        {
            // TODO: See if we can avoid resolving the Object
            const auto ResolvedObject = InAssetData.GetSoftObjectPath().TryLoad();
            const auto Object = Cast<UCk_AbilityCue_Config_PDA>(ResolvedObject);

            CK_LOG_ERROR_NOTIFY_IF_NOT(ck::ability, ck::IsValid(Object), TEXT("Unable to Cast Cue [{}] to [{}].{}"),
                    ResolvedObject, ck::Get_RuntimeTypeToString<UCk_AbilityCue_Config_PDA>(), ck::Context(this))
            { return false; }

            _AbilityCues.Add(Object->Get_CueName(), InAssetData.GetSoftObjectPath());
            _AbilityCueConfigs.Add(Object->Get_CueName(), Object);
            return true;
        });
    }

    const auto& OtherCueTypes = UCk_Utils_Ability_Settings_UE::Get_CueTypes();

    CK_LOG_ERROR_IF_NOT(ck::ability, NOT OtherCueTypes.IsEmpty(),
        TEXT("CueTypes in Ability Settings for this Project is empty. Cues without dedicated EntityConfigs will NOT work correctly"))
    { return; }

    for (const auto& CueType : OtherCueTypes)
    {
        const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

        auto CueTypeBlueprint = UCk_Utils_Object_UE::Get_ClassGeneratedByBlueprint(CueType);

        FARFilter Filter;
        // unfortunately, there is no way to get asset data on Blueprints deriving from a particular Blueprint class
        //Filter.ClassPaths.Add(FTopLevelAssetPath{UCk_Ability_Script_PDA::StaticClass()});

#if WITH_EDITOR
        Filter.ClassPaths.Add(FTopLevelAssetPath{UBlueprint::StaticClass()});
#else
        Filter.ClassPaths.Add(FTopLevelAssetPath{UBlueprintGeneratedClass::StaticClass()});
#endif

        Filter.PackagePaths.Add(*ExtractedPath);
        Filter.bRecursiveClasses = true;
        Filter.bRecursivePaths = true;
        Filter.bIncludeOnlyOnDiskAssets = false;

        FARCompiledFilter CompiledFilter;
        IAssetRegistry::Get()->CompileFilter(Filter, CompiledFilter);

        AssetRegistryModule.Get().EnumerateAssets(CompiledFilter, [&](const FAssetData& InAssetData)
        {
            constexpr auto ContinueIterating = true;

            // TODO: See if we can avoid resolving the Object
            const auto ResolvedObject = InAssetData.GetSoftObjectPath().TryLoad();

            CK_LOG_ERROR_IF_NOT(ck::ability, ck::IsValid(ResolvedObject),
                TEXT("Could not resolve the Object from Asset [{}] when trying Populate CueAggregators"),
                InAssetData.GetSoftObjectPath().ToString())
            { return ContinueIterating; }

            const auto ResolvedObjectDefaultClassObject = [&]() -> UObject*
            {
#if WITH_EDITOR
                const auto& Blueprint = Cast<UBlueprint>(ResolvedObject);

                if (ck::IsValid(Blueprint))
                { return Blueprint->GeneratedClass->GetDefaultObject(); }
#else
                const auto BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InAssetData.GetAsset());
                if (ck::IsValid(BlueprintGeneratedClass))
                { return BlueprintGeneratedClass->GetDefaultObject(); }
#endif
                return nullptr;
            }();

            CK_LOG_ERROR_IF_NOT(ck::ability, ck::IsValid(ResolvedObjectDefaultClassObject),
                TEXT("Could not get DefaultClass from Asset [{}] when trying Populate CueAggregators"),
                InAssetData.GetSoftObjectPath().ToString())
            { return ContinueIterating; }

            if (NOT ResolvedObjectDefaultClassObject->IsA(CueType))
            { return ContinueIterating; }

            const auto Object = Cast<UCk_Ability_Script_PDA>(ResolvedObjectDefaultClassObject);

            CK_LOG_ERROR_NOTIFY_IF_NOT(ck::ability, ck::IsValid(Object), TEXT("Unable to Cast Cue [{}] to [{}].{}"),
                    ResolvedObject, ck::Get_RuntimeTypeToString<UCk_Ability_Script_PDA>(), ck::Context(this))
            { return ContinueIterating; }

            const auto AbilityCueConfig = Get_ConfigForCue(Object->GetClass());

            if (ck::Is_NOT_Valid(AbilityCueConfig))
            { return ContinueIterating; }

            ck::ability::Log(TEXT("Discovered and Adding Cue [{}] with Name [{}]"), Object, Object->Get_Data().Get_AbilityName());

            _AbilityCues.Add(Object->Get_Data().Get_AbilityName(), InAssetData.GetSoftObjectPath());
            _AbilityCueConfigs.Add(Object->Get_Data().Get_AbilityName(), AbilityCueConfig);
            return ContinueIterating;
        });
    }
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
        Rep_CustomData,

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
        if (ck::IsValid(_CustomData))
        {
            RepBits |= (1 << Rep_CustomData);
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
            if (ck::Is_NOT_Valid(_Instigator_RepObj))
            {
                ck::ability::Verbose(TEXT("Instigator RepObj was [{}] even though RepBits tell us that it WAS replicated. Unable to set "
                    "the Instigator. Note that this is always possible if the cue comes in AFTER the Instigator has been destroyed"),
                    _Instigator_RepObj);
            }
            else
            {
                _Instigator = _Instigator_RepObj->Get_AssociatedEntity();
            }
        }
    }
    if (RepBits & (1 << Rep_EffectCauser))
    {
        Ar << _EffectCauser_RepObj;
        if (Ar.IsLoading())
        {
            if (ck::Is_NOT_Valid(_EffectCauser_RepObj))
            {
                ck::ability::Verbose(TEXT("EffectCauser RepObj was [{}] even though RepBits tell us that it WAS replicated. Unable to set "
                    "the EffectCauser. Note that this is always possible if the cue comes in AFTER the EffectCauser has been destroyed"),
                    _EffectCauser_RepObj);
            }
            else
            {
                _EffectCauser = _EffectCauser_RepObj->Get_AssociatedEntity();
            }
        }
    }
    if (RepBits & (1 << Rep_CustomData))
    {
        _CustomData.NetSerialize(Ar, Map, bOutSuccess);
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
