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
#include "Engine/Blueprint.h"
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
                if (ck::Is_NOT_Valid(Blueprint))
                { return {}; }

                if (const auto BlueprintGeneratedClass = Blueprint->GeneratedClass;
                    ck::IsValid(BlueprintGeneratedClass))
                { return BlueprintGeneratedClass->GetDefaultObject(); }
#else
                const auto BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InAssetData.GetAsset());
                if (ck::IsValid(BlueprintGeneratedClass))
                { return BlueprintGeneratedClass->GetDefaultObject(); }
#endif
                return {};
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

            const auto& AbilityName = Object->Get_Data().Get_AbilityName();

            ck::ability::Log(TEXT("Discovered and Adding Cue [{}] with Name [{}]"), Object, AbilityName);

            _AbilityCues.Add(AbilityName, InAssetData.GetSoftObjectPath());
            _AbilityCueConfigs.Add(AbilityName, AbilityCueConfig);

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

FCk_Request_AbilityCue_Spawn::
    FCk_Request_AbilityCue_Spawn(
        const FGameplayTag& InAbilityCueName,
        UObject* InWorldContextObject)
    : _AbilityCueName(InAbilityCueName)
    , _WorldContextObject(InWorldContextObject)
{
}

// --------------------------------------------------------------------------------------------------------------------
