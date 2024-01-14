#include "CkEditorAssetLoader_Subsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"

#include "CkResourceLoader/Public/CkResourceLoader/CkResourceLoader_Processor.h"
#include "CkResourceLoader/Public/CkResourceLoader/CkResourceLoader_Utils.h"

#include "CkResourceLoaderEditor/Settings/CkResourceLoaderEditor_Settings.h"

#include "Engine/AssetManager.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EditorAssetLoader_SubSystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    _World.Add<ck::FProcessor_ResourceLoader_HandleRequests>(_World.Get_Registry());
    _AssetLoaderEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_World.Get_Registry());

#if WITH_EDITOR
    if (GIsRunning)
    {
        // Engine init already completed
        DoOnEngineInitComplete();
    }
    else
    {
        FCoreDelegates::OnFEngineLoopInitComplete.AddUObject(this, &UCk_EditorAssetLoader_SubSystem_UE::DoOnEngineInitComplete);
    }
#endif
}

auto
    UCk_EditorAssetLoader_SubSystem_UE::
    DoOnEngineInitComplete()
    -> void
{
    Request_RefreshLoadingAssets();
}

auto
    UCk_EditorAssetLoader_SubSystem_UE::
    Request_RefreshLoadingAssets()
    -> void
{
    UCk_Utils_ResourceLoaderEditor_ProjectSettings_UE::Request_ClearAllLoadedObjects();

    const auto& ClassesToLoad = UCk_Utils_ResourceLoaderEditor_ProjectSettings_UE::Get_ClassesToLoad();

    if (ClassesToLoad.IsEmpty())
    { return; }

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry       = AssetRegistryModule.Get();

    const auto& ValidClassesToLoad = ck::algo::Filter(ClassesToLoad, [&](const FSoftClassPath& InClass)
    {
        return ck::IsValid(InClass);
    });

    const auto& ClassPathsToLoad = ck::algo::Transform<TArray<FTopLevelAssetPath>>(ValidClassesToLoad, [&](const FSoftClassPath& InClass)
    {
        return InClass.GetAssetPath();
    });

    FARFilter Filter;

    Filter.ClassPaths.Append(ClassPathsToLoad);
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;

    FARCompiledFilter CompiledFilter;
    AssetRegistry.CompileFilter(Filter, CompiledFilter);

    auto SoftObjectReferences = TArray<FCk_ResourceLoader_ObjectReference_Soft>{};

    for (const auto Path : CompiledFilter.ClassPaths)
    {
        const auto& SoftObjPath = FSoftObjectPath{Path};
        UCk_Utils_ResourceLoaderEditor_ProjectSettings_UE::Request_AddLoadedObject(SoftObjPath);
        SoftObjectReferences.Emplace(FCk_ResourceLoader_ObjectReference_Soft{SoftObjPath});
    }

    UCk_Utils_ResourceLoader_UE::Request_LoadObjectBatch
    (
        _AssetLoaderEntity,
        FCk_Request_ResourceLoader_LoadObjectBatch{SoftObjectReferences}.Set_LoadingPolicy(ECk_ResourceLoader_LoadingPolicy::Async),
        {}
    );

    _World.Tick(FCk_Time::ZeroSecond());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EditorAssetLoader_SubSystem_UE::
    Request_RefreshLoadingAssets()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(GEngine, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("GEngine is INVALID. This function is being called too early."))
    { return; }

    GEngine->GetEngineSubsystem<UCk_EditorAssetLoader_SubSystem_UE>()->Request_RefreshLoadingAssets();
}

// --------------------------------------------------------------------------------------------------------------------
