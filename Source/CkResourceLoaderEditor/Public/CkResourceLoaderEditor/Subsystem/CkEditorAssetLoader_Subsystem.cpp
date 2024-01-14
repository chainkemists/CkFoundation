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

    _World.Add<ck::FProcessor_ResourceLoader_HandleRequests>(_World.Get_Registry());
    _AssetLoaderEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_World.Get_Registry());
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
    const auto& ClassesToLoad = UCk_Utils_ResourceLoaderEditor_ProjectSettings_UE::Get_ClassesToLoad();

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry       = AssetRegistryModule.Get();

    FARFilter Filter;
    for (auto Class : ClassesToLoad)
    {
        if (Class.ResolveClass())
        {
            const auto ResolvedObject = Class.ResolveObject();
            Filter.ClassPaths.Add(FTopLevelAssetPath{ResolvedObject->GetPathName()});
        }
    }
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

    UCk_Utils_ResourceLoader_UE::Request_LoadObjectBatch(_AssetLoaderEntity, FCk_Request_ResourceLoader_LoadObjectBatch{SoftObjectReferences}, {});
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

