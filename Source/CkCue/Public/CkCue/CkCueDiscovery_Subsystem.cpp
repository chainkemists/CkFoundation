// CkCueDiscovery_Subsystem.cpp
//
// UCk_CueSubsystem_Base_UE implementation:
//   - Initialize / Deinitialize
//   - Cue discovery (C++ class scan + Blueprint asset scan)
//   - Asset registry integration (add, remove, rename, update callbacks)
//   - Discovery lookup (Get_CueEntityScript, Get_DiscoveredCues)
//   - Deferred discovery system (batches rapid asset registry events)

#include "CkCueSubsystem_Base.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Debug/CkDebug_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"

#include "CkCue/CkCue_Fragment.h"
#include "CkCue/CkCue_Log.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"

#include <AssetRegistry/AssetRegistryModule.h>

#if WITH_EDITOR
#include <Editor.h>
#include "Engine/Blueprint.h"
#endif

/*─────────────────────────────────────────────────────────────────────────────┐
│                            CUE SUBSYSTEM BASE                                │
└─────────────────────────────────────────────────────────────────────────────*/

auto
    UCk_CueSubsystem_Base_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    if (GIsRunning)
    {
        DoOnEngineInitComplete();
    }
    else
    {
        FCoreDelegates::OnFEngineLoopInitComplete.AddUObject(this, &UCk_CueSubsystem_Base_UE::DoOnEngineInitComplete);
    }
}

auto
    UCk_CueSubsystem_Base_UE::
    Deinitialize()
    -> void
{
    if (_DiscoveryDeferralTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_DiscoveryDeferralTickerHandle);
        _DiscoveryDeferralTickerHandle.Reset();
    }

    Super::Deinitialize();
}

auto
    UCk_CueSubsystem_Base_UE::
    Request_PopulateAllCues()
    -> void
{
    Request_DeferredPopulateAllCues();
}

auto
    UCk_CueSubsystem_Base_UE::
    Request_PopulateBlueprintCues()
    -> void
{
    auto CueBaseClass = Get_CueBaseClass();
    if (ck::Is_NOT_Valid(CueBaseClass))
    { return; }

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry = AssetRegistryModule.Get();

    if (AssetRegistry.IsLoadingAssets())
    {
        ck::cue::Warning(TEXT("AssetRegistry is still loading assets during Blueprint discovery"));
    }

    TArray<FAssetData> AllAssets;
    AssetRegistry.GetAllAssets(AllAssets);

    int32 ProcessedAssets = 0;
    int32 ValidCueAssets = 0;

    for (const auto& InAssetData : AllAssets)
    {
#if WITH_EDITOR
        if (NOT InAssetData.IsInstanceOf<UBlueprint>())
        { continue; }

        auto IsCueAssetTag = InAssetData.GetTagValueRef<FString>("IsCueAsset");
        if (NOT IsCueAssetTag.Equals("true"))
        { continue; }
#else
        if (NOT InAssetData.IsInstanceOf<UBlueprintGeneratedClass>())
        { continue; }
#endif

        ProcessedAssets++;

        if (const auto& AssetName = InAssetData.AssetName.ToString();
            AssetName.Contains(TEXT("AssetFactory")) ||
            AssetName.Contains(TEXT("_Factory")) ||
            AssetName.Contains(TEXT("Default__")))
        { continue; }

#if WITH_EDITOR
        auto ResolvedObject = InAssetData.GetSoftObjectPath().TryLoad();
        if (ck::Is_NOT_Valid(ResolvedObject))
        { continue; }

        auto Blueprint = Cast<UBlueprint>(ResolvedObject);
        if (ck::Is_NOT_Valid(Blueprint))
        { continue; }

        if (ck::Is_NOT_Valid(Blueprint->GeneratedClass))
        { continue; }

        if (NOT Blueprint->GeneratedClass->IsChildOf(CueBaseClass))
        { continue; }

        auto DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
        if (ck::Is_NOT_Valid(DefaultObject))
        { continue; }

        auto CueObject = Cast<UCk_CueBase_EntityScript>(DefaultObject);
#else
        auto BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InAssetData.GetAsset());
        if (ck::Is_NOT_Valid(BlueprintGeneratedClass))
        { continue; }

        if (NOT BlueprintGeneratedClass->IsChildOf(CueBaseClass))
        { continue; }

        auto DefaultObject = BlueprintGeneratedClass->GetDefaultObject();
        if (ck::Is_NOT_Valid(DefaultObject))
        { continue; }

        auto CueObject = Cast<UCk_CueBase_EntityScript>(DefaultObject);
#endif

        if (ck::Is_NOT_Valid(CueObject))
        { continue; }

        auto CueName = CueObject->Get_CueName();

        if (CueName == TAG_Cue_DoNotExecute || NOT CueName.IsValid())
        { continue; }

        if (_DiscoveredCues.Contains(CueName))
        { continue; }

        _DiscoveredCues.Add(CueName, CueObject->GetClass());
        ValidCueAssets++;
    }

    ck::cue::Log(TEXT("Blueprint assets: Processed=[{}], Valid cues=[{}]"),
                 ProcessedAssets, ValidCueAssets);

    auto FallbackCueCount = 0;

    for (TObjectIterator<UBlueprintGeneratedClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        auto GeneratedClass = *ClassIterator;

        if (ck::Is_NOT_Valid(GeneratedClass))
        { continue; }

        if (GeneratedClass->HasAnyClassFlags(CLASS_Abstract))
        { continue; }

        if (UCk_Utils_IO_UE::Get_IsTemporaryAsset(GeneratedClass->GetName()))
        { continue; }

        if (NOT GeneratedClass->IsChildOf(CueBaseClass))
        { continue; }

        auto DefaultObject = Cast<UCk_CueBase_EntityScript>(GeneratedClass->GetDefaultObject());
        if (ck::Is_NOT_Valid(DefaultObject))
        { continue; }

        auto CueName = DefaultObject->Get_CueName();

        if (CueName == TAG_Cue_DoNotExecute || NOT CueName.IsValid())
        { continue; }

        if (_DiscoveredCues.Contains(CueName))
        { continue; }

        _DiscoveredCues.Add(CueName, GeneratedClass);
        FallbackCueCount++;
    }

    if (FallbackCueCount > 0)
    {
        ck::cue::Log(TEXT("Fallback class iteration found [{}] additional cues"), FallbackCueCount);
    }
}

auto
    UCk_CueSubsystem_Base_UE::
    DoOnEngineInitComplete()
    -> void
{
    Request_PopulateAllCues();

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().OnAssetAdded().AddUObject(this, &UCk_CueSubsystem_Base_UE::DoHandleAssetAddedDeleted);
    AssetRegistryModule.Get().OnAssetRemoved().AddUObject(this, &UCk_CueSubsystem_Base_UE::DoHandleAssetAddedDeleted);
    AssetRegistryModule.Get().OnAssetRenamed().AddUObject(this, &UCk_CueSubsystem_Base_UE::DoHandleRenamed);
    AssetRegistryModule.Get().OnAssetUpdated().AddUObject(this, &UCk_CueSubsystem_Base_UE::DoAssetUpdated);
    AssetRegistryModule.Get().OnAssetUpdatedOnDisk().AddUObject(this, &UCk_CueSubsystem_Base_UE::DoAssetUpdated);
}

auto
   UCk_CueSubsystem_Base_UE::
   DoHandleAssetAddedDeleted(
       const FAssetData& InAssetData)
    -> void
{
   Request_ProcessAssetUpdate(InAssetData);
}

auto
   UCk_CueSubsystem_Base_UE::
   Request_ProcessAssetUpdate(
       const FAssetData& InAssetData)
    -> void
{
   auto CueBaseClass = Get_CueBaseClass();
   if (ck::Is_NOT_Valid(CueBaseClass))
   { return; }

#if WITH_EDITOR
if (const auto IsCueAssetTag = InAssetData.GetTagValueRef<FString>("IsCueAsset");
       NOT IsCueAssetTag.Equals("true"))
   { return; }

   if (NOT InAssetData.IsInstanceOf<UBlueprint>())
   { return; }

   auto Blueprint = Cast<UBlueprint>(InAssetData.GetAsset());
   if (ck::Is_NOT_Valid(Blueprint))
   { return; }

   if (ck::Is_NOT_Valid(Blueprint->GeneratedClass))
   { return; }

   if (NOT Blueprint->GeneratedClass->IsChildOf(CueBaseClass))
   { return; }

   Request_PopulateAllCues();
#else
if (NOT InAssetData.IsInstanceOf<UBlueprintGeneratedClass>())
   { return; }

   auto GeneratedClass = Cast<UBlueprintGeneratedClass>(InAssetData.GetAsset());
   if (ck::Is_NOT_Valid(GeneratedClass))
   { return; }

   if (NOT GeneratedClass->IsChildOf(CueBaseClass))
   { return; }

   Request_PopulateAllCues();
#endif
}

auto
    UCk_CueSubsystem_Base_UE::
    DoHandleRenamed(
        const FAssetData& InAssetData,
        const FString&)
    -> void
{
    DoHandleAssetAddedDeleted(InAssetData);
}

auto
    UCk_CueSubsystem_Base_UE::
    DoAssetUpdated(
        const FAssetData& InAssetData) -> void
{
    DoHandleAssetAddedDeleted(InAssetData);
}

auto
    UCk_CueSubsystem_Base_UE::
    Get_CueEntityScript(
        const FGameplayTag& InCueName)
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
#if WITH_EDITOR
    if (_DiscoveredCues.IsEmpty())
    { Request_PopulateAllCues(); }
#endif

    const auto FoundCue = _DiscoveredCues.Find(InCueName);

    CK_ENSURE_IF_NOT(ck::IsValid(FoundCue, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Failed to find Cue with Name [{}]! Cue Subsystem [{}] has not discovered any Cue with that name, does it even exist?"),
        InCueName,
        this)
    { return {}; }

    return *FoundCue;
}

auto
    UCk_CueSubsystem_Base_UE::
    Get_DiscoveredCues() const
    -> const TMap<FGameplayTag, TSubclassOf<UCk_CueBase_EntityScript>>&
{
    return _DiscoveredCues;
}

auto
    UCk_CueSubsystem_Base_UE::
    Request_DeferredPopulateAllCues()
    -> void
{
    if (_DiscoveredCues.IsEmpty())
    {
        ck::cue::Log(TEXT("No cues discovered yet - executing discovery immediately"));
        DoExecutePopulateAllCues();
        return;
    }

    if (_DiscoveryDeferralTickerHandle.IsValid())
    {
        _DiscoveryDeferralFramesRemaining = DISCOVERY_DEFERRAL_FRAMES;
        ck::cue::Log(TEXT("Cue discovery deferred - resetting {} frame counter"), DISCOVERY_DEFERRAL_FRAMES);
    }
    else
    {
        // Asset registry fires rapidly during editor operations (save, rename, delete).
        // We defer re-discovery by 60 frames to batch these events into a single scan.
        // UGameInstanceSubsystem doesn't have Tick(), so a ticker callback is used.
        _DiscoveryDeferralFramesRemaining = DISCOVERY_DEFERRAL_FRAMES;
        _DiscoveryDeferralTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &UCk_CueSubsystem_Base_UE::DoTickDeferredDiscovery)
        );
        ck::cue::Log(TEXT("Cue discovery deferred - starting {} frame counter"), DISCOVERY_DEFERRAL_FRAMES);
    }
}

auto
    UCk_CueSubsystem_Base_UE::
    DoTickDeferredDiscovery(
        float InDeltaTime)
    -> bool
{
    _DiscoveryDeferralFramesRemaining--;

    if (_DiscoveryDeferralFramesRemaining <= 0)
    {
        DoExecutePopulateAllCues();
        _DiscoveryDeferralTickerHandle.Reset();
        return false;
    }

    return true;
}

auto
    UCk_CueSubsystem_Base_UE::
    DoExecutePopulateAllCues()
    -> void
{
    const auto StartTime = FPlatformTime::Seconds();

    _DiscoveredCues.Empty();

    auto CueBaseClass = Get_CueBaseClass();
    CK_ENSURE_IF_NOT(ck::IsValid(CueBaseClass),
        TEXT("CueBaseClass is INVALID. Derived subsystem must implement Get_CueBaseClass"))
    { return; }

    ck::cue::Log(TEXT("=== CUE DISCOVERY START ==="));
    ck::cue::Log(TEXT("CueBaseClass: [{}]"), CueBaseClass->GetName());

    auto LoadedClassCount = 0;
    auto CueChildClasses = 0;
    auto ValidCueClassCount = 0;

    const auto ClassScanStartTime = FPlatformTime::Seconds();

    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        auto Class = *ClassIterator;
        LoadedClassCount++;

        if (ck::Is_NOT_Valid(Class) || Class->HasAnyClassFlags(CLASS_Abstract))
        { continue; }

        if (UCk_Utils_IO_UE::Get_IsTemporaryAsset(Class->GetName()))
        { continue; }

        if (NOT Class->IsChildOf(CueBaseClass))
        { continue; }

        CueChildClasses++;

        auto DefaultObject = Cast<UCk_CueBase_EntityScript>(Class->GetDefaultObject());
        if (ck::Is_NOT_Valid(DefaultObject))
        { continue; }

        auto CueName = DefaultObject->Get_CueName();

        if (NOT ck::IsValid(CueName))
        { continue; }

        if (CueName == TAG_Cue_DoNotExecute)
        { continue; }

        if (_DiscoveredCues.Contains(CueName))
        {
            auto ExistingClassName = _DiscoveredCues[CueName]->GetName();
            auto NewClassName = Class->GetName();

            if (NewClassName.Contains(TEXT("REINST_")) || ExistingClassName.Contains(TEXT("REINST_")))
            {
                if (NOT NewClassName.Contains(TEXT("REINST_")))
                {
                    _DiscoveredCues[CueName] = Class;
                }
                continue;
            }

            CK_TRIGGER_ENSURE(TEXT("Duplicate CueName [{}] found! Existing: [{}], New: [{}]"),
                CueName, _DiscoveredCues[CueName], Class);
            continue;
        }

        _DiscoveredCues.Add(CueName, Class);
        ValidCueClassCount++;
    }

    const auto ClassScanDuration = (FPlatformTime::Seconds() - ClassScanStartTime) * 1000.0;
    ck::cue::Log(TEXT("C++ class scan: Total=[{}], CueChildren=[{}], Valid=[{}], Time=[{:.2f}ms]"),
                 LoadedClassCount, CueChildClasses, ValidCueClassCount, ClassScanDuration);

    const auto BlueprintScanStartTime = FPlatformTime::Seconds();
    Request_PopulateBlueprintCues();
    const auto BlueprintScanDuration = (FPlatformTime::Seconds() - BlueprintScanStartTime) * 1000.0;

    ck::cue::Log(TEXT("Blueprint scan: Time=[{:.2f}ms]"), BlueprintScanDuration);

    const auto TotalDuration = (FPlatformTime::Seconds() - StartTime) * 1000.0;

    ck::cue::Log(TEXT("=== FINAL CUE DISCOVERY RESULTS ==="));
    ck::cue::Log(TEXT("Total cues discovered: [{}], Total time: [{:.2f}ms]"),
                 _DiscoveredCues.Num(), TotalDuration);
}
