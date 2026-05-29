#include "CkItemQuery_Subsystem.h"

#include "CkInventory/Item/CkItem_Definition.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

#include "Engine/AssetManager.h"
#include "Engine/Engine.h"

#include "UObject/UObjectGlobals.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemQuery_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

#if WITH_EDITOR
    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    auto& AssetRegistry = AssetRegistryModule.Get();

    _OnAssetAddedHandle   = AssetRegistry.OnAssetAdded()  .AddUObject(this, &UCk_ItemQuery_Subsystem_UE::DoOnAssetChanged);
    _OnAssetRemovedHandle = AssetRegistry.OnAssetRemoved().AddUObject(this, &UCk_ItemQuery_Subsystem_UE::DoOnAssetChanged);
    _OnAssetUpdatedHandle = AssetRegistry.OnAssetUpdated().AddUObject(this, &UCk_ItemQuery_Subsystem_UE::DoOnAssetChanged);
    _OnAssetRenamedHandle = AssetRegistry.OnAssetRenamed().AddUObject(this, &UCk_ItemQuery_Subsystem_UE::DoOnAssetRenamed);

    _OnObjectPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(
        this, &UCk_ItemQuery_Subsystem_UE::DoOnObjectPropertyChanged);
#endif
}

auto
    UCk_ItemQuery_Subsystem_UE::
    Deinitialize()
    -> void
{
#if WITH_EDITOR
    if (auto* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry")))
    {
        auto& AssetRegistry = AssetRegistryModule->Get();
        AssetRegistry.OnAssetAdded()  .Remove(_OnAssetAddedHandle);
        AssetRegistry.OnAssetRemoved().Remove(_OnAssetRemovedHandle);
        AssetRegistry.OnAssetUpdated().Remove(_OnAssetUpdatedHandle);
        AssetRegistry.OnAssetRenamed().Remove(_OnAssetRenamedHandle);
    }

    FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(_OnObjectPropertyChangedHandle);
#endif

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemQuery_Subsystem_UE::
    Get()
    -> UCk_ItemQuery_Subsystem_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(GEngine), TEXT("GEngine is invalid — cannot resolve ItemQuery subsystem"))
    { return nullptr; }

    return GEngine->GetEngineSubsystem<UCk_ItemQuery_Subsystem_UE>();
}

auto
    UCk_ItemQuery_Subsystem_UE::
    Get_IsIndexReady() const
    -> bool
{
    return _IsIndexReady;
}

auto
    UCk_ItemQuery_Subsystem_UE::
    Request_BuildIndex()
    -> void
{
    if (_IsIndexReady || _BuildInFlight)
    { return; }

    const auto Paths = DoGatherDefinitionPaths();

    if (Paths.IsEmpty())
    {
        _IsIndexReady = true;
        return;
    }

    _BuildInFlight = true;
    _LoadHandle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
        Paths, FStreamableDelegate::CreateUObject(this, &UCk_ItemQuery_Subsystem_UE::DoOnIndexLoaded));
}

auto
    UCk_ItemQuery_Subsystem_UE::
    Query_Definitions(
        const FCk_ItemQuery_Filter& InFilter) const
    -> TArray<TObjectPtr<UCk_InventoryItem_Definition>>
{
    const auto& RequiredAll   = InFilter.Get_RequiredAll();
    const auto& RequiredAny    = InFilter.Get_RequiredAny();
    const auto& Excluded       = InFilter.Get_Excluded();
    const auto& NativePredicate  = InFilter.Get_CustomFilter();
    const auto& DynamicPredicate = InFilter.Get_CustomFilterDynamic();

    auto Out = TArray<TObjectPtr<UCk_InventoryItem_Definition>>{};

    for (const auto& DefPtr : _AllDefinitions)
    {
        auto* Def = DefPtr.Get();
        if (ck::Is_NOT_Valid(Def))
        { continue; }

        if (NOT RequiredAll.IsEmpty() && NOT Def->Has_AllItemTraitsByClass(RequiredAll))
        { continue; }

        if (NOT RequiredAny.IsEmpty() && NOT Def->Has_AnyItemTraitByClass(RequiredAny))
        { continue; }

        if (NOT Excluded.IsEmpty() && Def->Has_AnyItemTraitByClass(Excluded))
        { continue; }

        if (NativePredicate.IsBound() && NOT NativePredicate.Execute(Def))
        { continue; }

        if (DynamicPredicate.IsBound())
        {
            auto Passes = true;
            DynamicPredicate.ExecuteIfBound(Def, Passes);

            if (NOT Passes)
            { continue; }
        }

        Out.Add(Def);
    }

    return Out;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemQuery_Subsystem_UE::
    DoGatherDefinitionPaths() const
    -> TArray<FSoftObjectPath>
{
    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

    auto Assets = TArray<FAssetData>{};
    AssetRegistryModule.Get().GetAssetsByClass(
        UCk_InventoryItem_Definition::StaticClass()->GetClassPathName(), Assets, true);

    auto Paths = TArray<FSoftObjectPath>{};
    Paths.Reserve(Assets.Num());
    for (const auto& Asset : Assets)
    {
        Paths.Add(Asset.ToSoftObjectPath());
    }

    return Paths;
}

auto
    UCk_ItemQuery_Subsystem_UE::
    DoOnIndexLoaded()
    -> void
{
    if (NOT _IsIndexReady && _LoadHandle.IsValid())
    {
        auto Loaded = TArray<UObject*>{};
        _LoadHandle->GetLoadedAssets(Loaded);

        for (auto* Obj : Loaded)
        {
            auto* Def = Cast<UCk_InventoryItem_Definition>(Obj);
            if (ck::IsValid(Def))
            { _AllDefinitions.Add(Def); }
        }

        _IsIndexReady = true;
    }

    _LoadHandle.Reset();
    _BuildInFlight = false;
}

auto
    UCk_ItemQuery_Subsystem_UE::
    DoInvalidateIndex()
    -> void
{
    if (_LoadHandle.IsValid())
    {
        _LoadHandle->CancelHandle();
        _LoadHandle.Reset();
    }

    _BuildInFlight = false;
    _IsIndexReady = false;
    _AllDefinitions.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR

auto
    UCk_ItemQuery_Subsystem_UE::
    DoOnAssetChanged(
        const FAssetData& InAssetData)
    -> void
{
    if (NOT InAssetData.IsInstanceOf(UCk_InventoryItem_Definition::StaticClass()))
    { return; }

    DoInvalidateIndex();
}

auto
    UCk_ItemQuery_Subsystem_UE::
    DoOnAssetRenamed(
        const FAssetData& InAssetData,
        const FString& InOldObjectPath)
    -> void
{
    DoOnAssetChanged(InAssetData);
}

auto
    UCk_ItemQuery_Subsystem_UE::
    DoOnObjectPropertyChanged(
        UObject* InObject,
        FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    if (ck::Is_NOT_Valid(InObject))
    { return; }

    if (NOT InObject->IsA<UCk_InventoryItem_Definition>())
    { return; }

    DoInvalidateIndex();
}

#endif

// --------------------------------------------------------------------------------------------------------------------
