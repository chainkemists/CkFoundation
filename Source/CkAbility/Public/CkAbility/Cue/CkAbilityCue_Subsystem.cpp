#include "CkAbilityCue_Subsystem.h"

#include "GameplayCueManager.h"

#include "AssetRegistry/AssetRegistryModule.h"

#include "CkAbility/Cue/CkAbilityCue_Fragment_Data.h"

#include "CkCore/IO/CkIO_Utils.h"

#include "CkUnreal/EntityBridge/CkEntityBridge_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AbilityCue_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
#if WITH_EDITOR
	if (GIsRunning)
	{
		// Engine init already completed
		DoOnEngineInitComplete();
	}
	else
	{
		FCoreDelegates::OnFEngineLoopInitComplete.AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoOnEngineInitComplete);
	}
#endif
}

auto
    UCk_AbilityCue_Subsystem_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoOnEngineInitComplete()
    -> void
{
#if WITH_EDITOR
    DoPopulateAllAggregators();
    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().OnAssetAdded().AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoHandleAssetAddedDeleted);
	AssetRegistryModule.Get().OnAssetRemoved().AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoHandleAssetAddedDeleted);
	AssetRegistryModule.Get().OnAssetRenamed().AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoHandleRenamed);
#endif
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoHandleAssetAddedDeleted(
        const FAssetData& InAssetData)
    -> void
{
    const auto Object = InAssetData.GetSoftObjectPath().ResolveObject();

    if (ck::Is_NOT_Valid(Object))
    { return; }

    if (Object->IsA<UCk_AbilityCue_Aggregator_DA>())
    {
        DoAddAggregator(InAssetData);
    }
    else if (Object->IsA<UCk_EntityBridge_Config_Base_PDA>())
    {
        const auto& PathOnly = UCk_Utils_IO_UE::Get_ExtractPath(Object->GetPathName());

        const auto Found = _Aggregators.Find(FName{PathOnly});

        if (NOT ck::IsValid(Found, ck::IsValid_Policy_NullptrOnly{}))
        {
            UCk_Utils_EditorOnly_UE::Request_PushNewEditorMessage
            (
                FCk_Utils_EditorOnly_PushNewEditorMessage_Params
                {
                    TEXT("Ability"),
                    FCk_MessageSegments
                    {
                        {
                            FCk_TokenizedMessage{TEXT("Ability Cue added to a folder WITHOUT an Aggregator")}.Set_TargetObject(this)
                        }
                    }
                }
                .Set_MessageSeverity(ECk_EditorMessage_Severity::Error)
                .Set_ToastNotificationDisplayPolicy(ECk_EditorMessage_ToastNotification_DisplayPolicy::Display)
                .Set_MessageLogDisplayPolicy(ECk_EditorMessage_MessageLog_DisplayPolicy::Focus)
            );

            return;
        }

        Found->LoadSynchronous()->Request_Populate();
    }
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoHandleRenamed(
        const FAssetData& InAssetData,
        const FString& InRename)
    -> void
{
    const auto Object = InAssetData.GetSoftObjectPath().ResolveObject();

    if (ck::Is_NOT_Valid(Object))
    { return; }

    if (Object->IsA<UCk_EntityBridge_Config_Base_PDA>())
    {
        const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

        FARFilter Filter;
        Filter.ClassPaths.Add(FTopLevelAssetPath{UCk_AbilityCue_Aggregator_DA::StaticClass()});
        Filter.bRecursiveClasses = true;

        FARCompiledFilter CompiledFilter;
        IAssetRegistry::Get()->CompileFilter(Filter, CompiledFilter);

        TArray<TSoftObjectPtr<UCk_AbilityCue_Aggregator_DA>> Assets;
        AssetRegistryModule.Get().EnumerateAssets(CompiledFilter, [&](const FAssetData& InEnumeratedAssetData)
        {
            Assets.Emplace(TSoftObjectPtr<UCk_AbilityCue_Aggregator_DA>{InEnumeratedAssetData.GetSoftObjectPath()});
            return true;
        });

        for (auto& Asset : Assets)
        {
            const auto Aggregator = Asset.LoadSynchronous();
            Aggregator->Request_Populate();
        }
    }
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoPopulateAllAggregators()
    -> void
{
    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    FARFilter Filter;
    Filter.ClassPaths.Add(FTopLevelAssetPath{UCk_AbilityCue_Aggregator_DA::StaticClass()});
    Filter.bRecursiveClasses = true;

    FARCompiledFilter CompiledFilter;
    IAssetRegistry::Get()->CompileFilter(Filter, CompiledFilter);

    TArray<TSoftObjectPtr<UCk_AbilityCue_Aggregator_DA>> Assets;
    AssetRegistryModule.Get().EnumerateAssets(CompiledFilter, [&](const FAssetData& InEnumeratedAssetData)
    {
        Assets.Emplace(TSoftObjectPtr<UCk_AbilityCue_Aggregator_DA>{InEnumeratedAssetData.GetSoftObjectPath()});
        return true;
    });

    for (auto& Asset : Assets)
    {
        const auto Aggregator = Asset.LoadSynchronous();
        Aggregator->Request_Populate();
    }
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoAddAggregator(
        const FAssetData& InAggregatorData)
    -> void
{
    const auto Aggregator = TSoftObjectPtr<UCk_AbilityCue_Aggregator_DA>{InAggregatorData.GetSoftObjectPath()};

    CK_ENSURE_IF_NOT(ck::IsValid(Aggregator),
        TEXT("Could not load Aggregator using AssetData [{}]. Either the AssetData is incorrect OR there is a redirector involved."),
        InAggregatorData)
    { return; }

    _Aggregators.Add(InAggregatorData.PackagePath, Aggregator);
}

// --------------------------------------------------------------------------------------------------------------------
