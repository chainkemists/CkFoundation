#include "CkEntityScript_Subsystem.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/CkEcsLog.h"

#include <UObject/ObjectSaveContext.h>
#include <EngineUtils.h>
#include <AssetRegistry/IAssetRegistry.h>
#include <Misc/ScopeExit.h>
#include <UObject/StrongObjectPtr.h>

#if WITH_EDITOR
#include <Subsystems/EditorAssetSubsystem.h>
#include <ISourceControlModule.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <Kismet2/StructureEditorUtils.h>
#include <Editor/EditorEngine.h>
#include <UserDefinedStructure/UserDefinedStructEditorData.h>
#include <Engine/Engine.h>
#endif

// -----------------------------------------------------------------------------------------------------------

auto
    UCk_EntityScript_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _EntitySpawnParams_StructFolderName = UCk_Utils_Ecs_Settings_UE::Get_EntityScriptSpawnParamsFolderName();

    // FindObject (memory-only), never GetAsset()/LoadObject: package loading during subsystem init
    // cascades into Blueprint regeneration and re-entrant compilation.
#if WITH_EDITOR
    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
        auto StructAssets = TArray<FAssetData>{};
        auto Filter = FARFilter{};
        Filter.ClassPaths.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());

        AssetRegistry->GetAssets(Filter, StructAssets);

        for (const auto& Asset : StructAssets)
        {
            if (Asset.AssetName.ToString().StartsWith(_SpawnParamsStructName_Prefix))
            {
                if (auto* Struct = FindObject<UUserDefinedStruct>(nullptr, *Asset.GetObjectPathString());
                    ck::IsValid(Struct))
                {
                    _EntitySpawnParams_Structs.Add(Struct);
                    _EntitySpawnParams_StructsByName.Add(Asset.AssetName, Struct);
                }
            }
        }
    }
#endif

    for (auto It = TObjectIterator<UClass>{}; It; ++It)
    {
        UClass* Class = *It;

        if (Class->IsChildOf(UCk_EntityScript_UE::StaticClass()) &&
            NOT UCk_Utils_IO_UE::Get_IsTemporaryAsset(Class->GetName()) &&
            Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
        {
            constexpr auto ForceRecreate = false;
            std::ignore = DoGetOrCreate_SpawnParamsStructForEntity_Internal(Class, ForceRecreate);
        }
    }

    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
        _OnFilesLoaded_DelegateHandle = AssetRegistry->OnFilesLoaded().AddUObject(this, &ThisType::OnFilesLoaded);
    }

#if WITH_EDITOR
    if (ck::IsValid(GEditor))
    {
        const auto RequestDeferredUpdate = [this]()
        {
            _bHasPendingStructUpdates = true;
            ScheduleDeferredStructUpdate();
        };

        _OnBlueprintCompiled_DelegateHandle = GEditor->OnBlueprintPreCompile().AddLambda([this](UBlueprint* InBlueprint)
        {
            if (ck::Is_NOT_Valid(InBlueprint) || ck::Is_NOT_Valid(InBlueprint->GeneratedClass))
            { return; }

            _ActiveCompilation = InBlueprint;
            Request_StartCompilationTicker();

            if (InBlueprint->GeneratedClass->IsChildOf(UCk_EntityScript_UE::StaticClass()) &&
                NOT UCk_Utils_IO_UE::Get_IsTemporaryAsset(InBlueprint->GeneratedClass->GetName()))
            {
                std::ignore = DoGetOrCreate_SpawnParamsStructForEntity_Internal(InBlueprint->GeneratedClass, true);
            }
        });

        _OnBlueprintReinstanced_DelegateHandle = GEditor->OnBlueprintCompiled().AddLambda([this, RequestDeferredUpdate]()
        {
            _ActiveCompilation.Reset();
            RequestDeferredUpdate();
        });
    }
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    Deinitialize()
    -> void
{
    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
        AssetRegistry->OnFilesLoaded().Remove(_OnFilesLoaded_DelegateHandle);
        AssetRegistry->OnAssetAdded().Remove(_OnAssetAdded_DelegateHandle);
        AssetRegistry->OnAssetRemoved().Remove(_OnAssetRemoved_DelegateHandle);
        AssetRegistry->OnAssetRenamed().Remove(_OnAssetRenamed_DelegateHandle);
    }

#if WITH_EDITOR
    FCoreUObjectDelegates::OnObjectPreSave.Remove(_OnObjectPreSave_DelegateHandle);

    if (ck::IsValid(GEditor))
    {
        GEditor->OnBlueprintCompiled().Remove(_OnBlueprintCompiled_DelegateHandle);
        GEditor->OnBlueprintReinstanced().Remove(_OnBlueprintReinstanced_DelegateHandle);
    }
#endif

    Request_StopCompilationTicker();
    if (NOT _PendingStructAssetOperations.IsEmpty() || NOT _EntitySpawnParams_StructsToSave.IsEmpty())
    {
        ck::ecs::Warning(TEXT("EntityScript shutdown with [{}] unresolved struct asset operations and [{}] pending struct saves"),
            _PendingStructAssetOperations.Num(), _EntitySpawnParams_StructsToSave.Num());
    }
    _PendingSpawnParamsRequests.Empty();
    _PendingStructAssetOperations.Empty();

    Super::Deinitialize();
}

auto
    UCk_EntityScript_Subsystem_UE::
    Request_StartCompilationTicker() -> void
{
    if (_CompilationCheckTickerHandle.IsValid())
    { return; }

    _CompilationCheckTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UCk_EntityScript_Subsystem_UE::Request_CheckCompilationStatus),
        0.5f
    );
}

auto
    UCk_EntityScript_Subsystem_UE::
    Request_StopCompilationTicker() -> void
{
    if (_CompilationCheckTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_CompilationCheckTickerHandle);
        _CompilationCheckTickerHandle.Reset();
    }
}

bool
    UCk_EntityScript_Subsystem_UE::
    Request_CheckCompilationStatus(
        float InDeltaTime)
{
    if (ck::IsValid(_ActiveCompilation))
    { return true; }

    Request_ProcessPendingSpawnParamsRequests();
    ProcessPendingStructAssetOperations();
#if WITH_EDITOR
    if (ck::IsValid(GEditor) && ck::IsValid(GEngine) && GEngine->bIsInitialized)
    {
        auto PendingSaves = TArray<TObjectPtr<UUserDefinedStruct>>{};
        PendingSaves.Reserve(_EntitySpawnParams_StructsToSave.Num());
        for (const auto Pending : _EntitySpawnParams_StructsToSave)
        { PendingSaves.Add(Pending); }
        for (const auto Struct : PendingSaves)
        { std::ignore = TrySavePendingStruct(Struct); }
    }
#endif
    if (NOT _PendingStructAssetOperations.IsEmpty() || NOT _EntitySpawnParams_StructsToSave.IsEmpty())
    { return true; }

    Request_StopCompilationTicker();
    return false;
}

auto
    UCk_EntityScript_Subsystem_UE::
    TrySavePendingStruct(TObjectPtr<UUserDefinedStruct> InStruct) -> bool
{
#if WITH_EDITOR
    if (NOT _EntitySpawnParams_StructsToSave.Contains(InStruct))
    { return false; }
    if (_StructsBeingSaved.Contains(InStruct))
    { return true; }

    _EntitySpawnParams_StructsToSave.Remove(InStruct);
    _StructsBeingSaved.Add(InStruct);
    const auto bSaved = SaveStruct(InStruct.Get());
    _StructsBeingSaved.Remove(InStruct);
    const auto bDirtyDuringSave = _StructsDirtyDuringSave.Remove(InStruct) > 0;
    if (NOT bSaved || bDirtyDuringSave)
    {
        _EntitySpawnParams_StructsToSave.Add(InStruct);
        Request_StartCompilationTicker();
    }
    return bSaved;
#else
    return false;
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    QueueStructAssetOperation(FPendingStructAssetOperation&& InOperation) -> void
{
#if WITH_EDITOR
    // The source of a deferred rename remains at its original package until it succeeds.
    // Retarget it when subsequent Blueprint events refer to the unsaved intermediate name.
    for (auto& Pending : _PendingStructAssetOperations)
    {
        if (Pending.bDelete && NOT InOperation.bDelete &&
            (Pending.SourcePath == InOperation.SourcePath ||
                Pending.RemovedRenameSourcePath == InOperation.SourcePath))
        {
            Pending.TargetPath = MoveTemp(InOperation.TargetPath);
            Pending.TargetName = InOperation.TargetName;
            Pending.bDelete = false;
            Pending.RemovedRenameSourcePath.Empty();
            ++Pending.Revision;
            Request_StartCompilationTicker();
            return;
        }

        if (NOT Pending.bDelete && Pending.TargetPath == InOperation.SourcePath)
        {
            if (InOperation.bDelete)
            { Pending.RemovedRenameSourcePath = InOperation.SourcePath; }
            else
            { Pending.RemovedRenameSourcePath.Empty(); }
            Pending.TargetPath = MoveTemp(InOperation.TargetPath);
            Pending.TargetName = InOperation.TargetName;
            Pending.bDelete = InOperation.bDelete;
            ++Pending.Revision;
            Request_StartCompilationTicker();
            return;
        }
    }

    _PendingStructAssetOperations.Add(MoveTemp(InOperation));
    Request_StartCompilationTicker();
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    ProcessPendingStructAssetOperations() -> void
{
#if WITH_EDITOR
    if (_bProcessingStructAssetOperations || _PendingStructAssetOperations.IsEmpty() ||
        ck::Is_NOT_Valid(GEditor) || ck::Is_NOT_Valid(GEngine) || NOT GEngine->bIsInitialized ||
        ck::IsValid(_ActiveCompilation) || GCompilingBlueprint)
    { return; }

    const auto EditorAssets = TObjectPtr<UEditorAssetSubsystem>{GEditor->GetEditorSubsystem<UEditorAssetSubsystem>()};
    if (ck::Is_NOT_Valid(EditorAssets.Get()))
    { return; }

    _bProcessingStructAssetOperations = true;
    ON_SCOPE_EXIT { _bProcessingStructAssetOperations = false; };

    for (auto Index = int32{0}; Index < _PendingStructAssetOperations.Num();)
    {
        const auto Pending = _PendingStructAssetOperations[Index];
        const auto bSourceExists = EditorAssets->DoesAssetExist(Pending.SourcePath);
        const auto SourceObjectPath = Pending.SourcePath + TEXT(".") + Pending.SourceName.ToString();
        const auto Cached = TObjectPtr<UUserDefinedStruct>{
            _EntitySpawnParams_StructsByName.FindRef(Pending.SourceName)};
        const auto bCachedSourceMatches = ck::IsValid(Cached.Get()) && Cached->GetPathName() == SourceObjectPath;

        if (NOT bSourceExists && NOT Pending.bDelete &&
            NOT EditorAssets->DoesAssetExist(Pending.TargetPath))
        {
            // A registered source may arrive later; an absent physical source has nothing to move.
            if (ck::IsValid(FindObject<UUserDefinedStruct>(nullptr, *SourceObjectPath)) ||
                FPackageName::DoesPackageExist(Pending.SourcePath))
            {
                ++Index;
                continue;
            }
            ck::ecs::Warning(TEXT("SpawnParams rename source [{}] no longer exists; abandoning deferred rename to [{}]"),
                Pending.SourcePath, Pending.TargetPath);
            _PendingStructAssetOperations.RemoveAt(Index);
            continue;
        }

        if (NOT bSourceExists && NOT Pending.bDelete)
        {
            if (ck::IsValid(FindObject<UUserDefinedStruct>(nullptr, *SourceObjectPath)) ||
                FPackageName::DoesPackageExist(Pending.SourcePath))
            {
                ++Index;
                continue;
            }
            ck::ecs::Warning(TEXT("SpawnParams destination [{}] conflicts with missing source [{}]; discarding deferred rename without adopting the destination"),
                Pending.TargetPath, Pending.SourcePath);
            _PendingStructAssetOperations.RemoveAt(Index);
            continue;
        }

        auto SourceStruct = TStrongObjectPtr<UUserDefinedStruct>{};
        if (bSourceExists)
        { SourceStruct.Reset(Cast<UUserDefinedStruct>(EditorAssets->LoadAsset(Pending.SourcePath))); }

        const auto bCompleted = Pending.bDelete
            ? (NOT bSourceExists || EditorAssets->DeleteAsset(Pending.SourcePath))
            : (ck::IsValid(SourceStruct.Get()) && EditorAssets->RenameAsset(Pending.SourcePath, Pending.TargetPath));
        if (NOT bCompleted)
        {
            ++Index;
            continue;
        }

        if (bCachedSourceMatches)
        {
            _EntitySpawnParams_StructsByName.Remove(Pending.SourceName);
            if (Pending.bDelete)
            {
                _EntitySpawnParams_Structs.Remove(Cached.Get());
                _EntitySpawnParams_StructsToSave.Remove(Cached.Get());
            }
        }

        if (NOT Pending.bDelete)
        {
            const auto TargetStruct = TObjectPtr<UUserDefinedStruct>{SourceStruct.Get()};
            if (ck::IsValid(TargetStruct.Get()) &&
                NOT _EntitySpawnParams_StructsByName.Contains(Pending.TargetName))
            {
                _EntitySpawnParams_Structs.Add(TargetStruct.Get());
                _EntitySpawnParams_StructsByName.Add(Pending.TargetName, TargetStruct.Get());
            }
        }

        if (_PendingStructAssetOperations[Index].Revision != Pending.Revision && NOT Pending.bDelete)
        {
            // A synchronous callback retargeted this operation while the source was moving.
            // The next attempt must start from the package that just became physical.
            _PendingStructAssetOperations[Index].SourcePath = Pending.TargetPath;
            _PendingStructAssetOperations[Index].SourceName = Pending.TargetName;
            ++Index;
        }
        else
        { _PendingStructAssetOperations.RemoveAt(Index); }
    }
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    Request_ProcessPendingSpawnParamsRequests() -> void
{
    if (_PendingSpawnParamsRequests.IsEmpty())
    { return; }

    for (auto& Request : _PendingSpawnParamsRequests)
    {
        if (ck::Is_NOT_Valid(Request.EntityScriptClass.Get()))
        { continue; }

        auto* Result = DoGetOrCreate_SpawnParamsStructForEntity_Internal(
            Request.EntityScriptClass.Get(),
            Request.ForceRecreate);

        for (auto& WeakPromise : Request.Promises)
        {
            if (auto Promise = WeakPromise.Pin())
            {
                Promise->SetValue(Result);
            }
        }
    }

    _PendingSpawnParamsRequests.Empty();
}

auto
    UCk_EntityScript_Subsystem_UE::
    GetOrCreate_SpawnParamsStructForEntity(
        UClass* InEntityScriptClass,
        bool InForceRecreate) -> UUserDefinedStruct*
{
    if (ck::Is_NOT_Valid(InEntityScriptClass))
    { return {}; }

    if (NOT InForceRecreate && _PendingStructAssetOperations.IsEmpty())
    {
        const auto& StructName = GenerateEntitySpawnParamsStructName(InEntityScriptClass);
        if (const auto& FoundExistingStruct = _EntitySpawnParams_StructsByName.Find(StructName);
            ck::IsValid(FoundExistingStruct, ck::IsValid_Policy_NullptrOnly{}))
        {
#if WITH_EDITOR
            const auto ExpectedPath = Get_StructPathForEntityScriptPath(InEntityScriptClass->GetPackage()->GetName()) /
                StructName.ToString() + TEXT(".") + StructName.ToString();
            if ((*FoundExistingStruct)->GetPathName() == ExpectedPath)
            { return *FoundExistingStruct; }
#else
            return *FoundExistingStruct;
#endif
        }
    }

    if (ck::IsValid(_ActiveCompilation))
    {
        auto* ExistingRequest = _PendingSpawnParamsRequests.FindByPredicate(
            [InEntityScriptClass](const FPendingSpawnParamsRequest& Request)
            {
                return Request.EntityScriptClass == InEntityScriptClass;
            });

        if (ck::Is_NOT_Valid(ExistingRequest, ck::IsValid_Policy_NullptrOnly{}))
        {
            auto& NewRequest = _PendingSpawnParamsRequests.Emplace_GetRef();
            NewRequest.EntityScriptClass = InEntityScriptClass;
            NewRequest.ForceRecreate = InForceRecreate;
        }
        else if (InForceRecreate)
        {
            ExistingRequest->ForceRecreate = true;
        }

        return nullptr;
    }

    return DoGetOrCreate_SpawnParamsStructForEntity_Internal(InEntityScriptClass, InForceRecreate);
}

auto
    UCk_EntityScript_Subsystem_UE::
    GetOrCreate_SpawnParamsStructForEntity_Async(
        UClass* InEntityScriptClass,
        bool InForceRecreate) -> TFuture<UUserDefinedStruct*>
{
    auto Promise = MakeShared<TPromise<UUserDefinedStruct*>>();
    auto Future = Promise->GetFuture();

    if (ck::Is_NOT_Valid(_ActiveCompilation))
    {
        auto* Result = DoGetOrCreate_SpawnParamsStructForEntity_Internal(InEntityScriptClass, InForceRecreate);
        Promise->SetValue(Result);
    }
    else
    {
        auto* ExistingRequest = _PendingSpawnParamsRequests.FindByPredicate(
            [InEntityScriptClass](const FPendingSpawnParamsRequest& Request)
            {
                return Request.EntityScriptClass == InEntityScriptClass;
            });

        if (ck::Is_NOT_Valid(ExistingRequest, ck::IsValid_Policy_NullptrOnly{}))
        {
            auto& NewRequest = _PendingSpawnParamsRequests.Emplace_GetRef();
            NewRequest.EntityScriptClass = InEntityScriptClass;
            NewRequest.ForceRecreate = InForceRecreate;
            NewRequest.Promises.Add(Promise);
        }
        else
        {
            if (InForceRecreate)
            {
                ExistingRequest->ForceRecreate = true;
            }
            ExistingRequest->Promises.Add(Promise);
        }
    }

    return Future;
}

auto
    UCk_EntityScript_Subsystem_UE::
    DoGetOrCreate_SpawnParamsStructForEntity_Internal(
        UClass* InEntityScriptClass,
        bool InForceRecreate) -> UUserDefinedStruct*
{
    if (NOT InEntityScriptClass->IsChildOf(UCk_EntityScript_UE::StaticClass()))
    { return {}; }

    if (UCk_Utils_IO_UE::Get_IsTemporaryAsset(InEntityScriptClass->GetName()))
    { return {}; }

    // Only Blueprint EntityScripts need a SpawnParams struct (K2Node pins). Script classes live in
    // /Script/Angelscript, with no way to resolve back to the owning plugin's content root.
#if WITH_ANGELSCRIPT_CK
    ck::ecs::Verbose(TEXT("[SpawnParams] DoGetOrCreate called for [{}] | bIsScriptClass=[{}] | CompiledFromBP=[{}] | Package=[{}]"),
        InEntityScriptClass->GetName(),
        InEntityScriptClass->bIsScriptClass,
        InEntityScriptClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint),
        InEntityScriptClass->GetPackage()->GetName());

    if (InEntityScriptClass->bIsScriptClass)
    {
        ck::ecs::Verbose(TEXT("[SpawnParams] Skipping [{}] — bIsScriptClass is true"), InEntityScriptClass->GetName());
        return {};
    }
#endif

    const auto& StructName = GenerateEntitySpawnParamsStructName(InEntityScriptClass);

#if WITH_EDITOR
    const auto DesiredPath = Get_StructPathForEntityScriptPath(InEntityScriptClass->GetPackage()->GetName()) /
        StructName.ToString();
    for (const auto& Pending : _PendingStructAssetOperations)
    {
        if (NOT Pending.bDelete && Pending.TargetPath == DesiredPath)
        {
            const auto SourceObjectPath = Pending.SourcePath + TEXT(".") + Pending.SourceName.ToString();
            const auto CachedSource = TObjectPtr<UUserDefinedStruct>{
                _EntitySpawnParams_StructsByName.FindRef(Pending.SourceName)};
            if (ck::IsValid(CachedSource.Get()) && CachedSource->GetPathName() == SourceObjectPath)
            { return CachedSource.Get(); }

            const auto LoadedSource = TObjectPtr<UUserDefinedStruct>{
                FindObject<UUserDefinedStruct>(nullptr, *SourceObjectPath)};
            return LoadedSource.Get();
        }
    }
#endif

    if (NOT InForceRecreate)
    {
        if (const auto& FoundExistingStruct = _EntitySpawnParams_StructsByName.Find(StructName);
            ck::IsValid(FoundExistingStruct, ck::IsValid_Policy_NullptrOnly{}))
        {
#if WITH_EDITOR
            if ((*FoundExistingStruct)->GetPathName() != DesiredPath + TEXT(".") + StructName.ToString())
            { /* A same-name struct in another content root does not own this Blueprint. */ }
            else
#endif
            return *FoundExistingStruct;
        }
    }

    UUserDefinedStruct* SpawnParamsStructForEntity = nullptr;

#if WITH_EDITOR
    // FindObject (memory-only), never LoadObject: loading here cascades into Blueprint compilation and
    // re-entrant QueueForCompilation crashes. A struct not in memory is created fresh below.
    const auto StructPackagePath = Get_StructPathForEntityScriptPath(InEntityScriptClass->GetPackage()->GetName());
    const auto StructFullPath = StructPackagePath / StructName.ToString();

    if (const auto ExistingStruct = TObjectPtr<UUserDefinedStruct>{
            FindObject<UUserDefinedStruct>(nullptr, *(StructFullPath + TEXT(".") + StructName.ToString()))};
        ck::IsValid(ExistingStruct.Get()))
    {
        if (NOT _EntitySpawnParams_StructsByName.Contains(StructName))
        {
            _EntitySpawnParams_Structs.Add(ExistingStruct.Get());
            _EntitySpawnParams_StructsByName.Add(StructName, ExistingStruct.Get());
        }
        SpawnParamsStructForEntity = ExistingStruct.Get();
    }

    if (const auto& FoundExistingStruct = _EntitySpawnParams_StructsByName.Find(StructName);
        ck::IsValid(FoundExistingStruct, ck::IsValid_Policy_NullptrOnly{}) &&
        (*FoundExistingStruct)->GetPathName() == StructFullPath + TEXT(".") + StructName.ToString())
    {
        SpawnParamsStructForEntity = *FoundExistingStruct;
    }

    if (ck::IsValid(SpawnParamsStructForEntity))
    {

        // Mid-compilation the cached struct must be returned as-is: UpdateStructProperties would
        // re-enter compilation of dependent Blueprints. The ticker re-runs the update afterwards.
        if (GCompilingBlueprint)
        {
            ck::ecs::Display(TEXT("[SpawnParams] GCompilingBlueprint — returning cached struct for [{}] without update"), InEntityScriptClass->GetName());
            return SpawnParamsStructForEntity;
        }

        const auto& ExposedProperties = UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass(InEntityScriptClass);

        auto ExistingProperties = TArray<FProperty*>{};
        for (auto PropIt = TFieldIterator<FProperty>(SpawnParamsStructForEntity); PropIt; ++PropIt)
        {
            ExistingProperties.Add(*PropIt);
        }

        if (ExposedProperties.IsEmpty() && ExistingProperties.Num() == 1)
        { return SpawnParamsStructForEntity; }

        if (NOT UCk_Utils_Reflection_UE::Get_ArePropertiesDifferent(ExistingProperties, ExposedProperties))
        { return SpawnParamsStructForEntity; }

        ck::ecs::Display(TEXT("EntityScript [{}] properties changed - updating associated Spawn Params struct..."), InEntityScriptClass);

        if (UpdateStructProperties(SpawnParamsStructForEntity, ExposedProperties))
        {
            if (_StructsBeingSaved.Contains(SpawnParamsStructForEntity))
            { _StructsDirtyDuringSave.Add(SpawnParamsStructForEntity); }
            _EntitySpawnParams_StructsToSave.Add(SpawnParamsStructForEntity);
            Request_StartCompilationTicker();
        }
    }

    // No new structs mid-compilation — CreateUserDefinedStruct fires OnStructureChanged, which
    // re-enters compilation.
    if (ck::Is_NOT_Valid(SpawnParamsStructForEntity) && GCompilingBlueprint)
    {
        ck::ecs::Display(TEXT("[SpawnParams] GCompilingBlueprint — deferring new struct creation for [{}]"), InEntityScriptClass->GetName());
    }

    if (ck::Is_NOT_Valid(SpawnParamsStructForEntity) && NOT GCompilingBlueprint)
    {
        const auto& ExposedProperties = UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass(InEntityScriptClass);

        const auto StructPackageName = Get_StructPathForEntityScriptPath(InEntityScriptClass->GetPackage()->GetName()) / StructName.ToString();
        auto* StructPackage = CreatePackage(*StructPackageName);

        // without checking for this, we eventually experience a crash (although, we don't crash _all_ the time)
        // in UObjectGlobals:3465 because the dependent struct has not yet loaded
        if (auto Obj = StaticFindObjectFastInternal(nullptr, StructPackage, StructName, EFindObjectFlags::ExactClass);
            ck::IsValid(Obj) && (Obj->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad | RF_ClassDefaultObject) || Obj->GetClass()->bLayoutChanging))
        { return {}; }

        // An asset on disk but not yet in memory must NOT be recreated — a fresh struct gets fresh
        // variable GUIDs, invalidating FInstancedStruct data in Blueprints that hold the old ones.
        // Mid-async-load it arrives as a Blueprint dependency and OnFilesLoaded discovers it.
        if (FPackageName::DoesPackageExist(StructPackageName))
        {
            if (IsAsyncLoading())
            { return {}; }

            SpawnParamsStructForEntity = LoadObject<UUserDefinedStruct>(nullptr, *(StructPackageName + TEXT(".") + StructName.ToString()));

            if (ck::IsValid(SpawnParamsStructForEntity))
            {
                _EntitySpawnParams_Structs.Add(SpawnParamsStructForEntity);
                _EntitySpawnParams_StructsByName.Add(StructName, SpawnParamsStructForEntity);
                return SpawnParamsStructForEntity;
            }
        }

        ck::ecs::Display(TEXT("[SpawnParams] Creating new struct [{}] at [{}]"), StructName, StructPackageName);

        SpawnParamsStructForEntity = FStructureEditorUtils::CreateUserDefinedStruct(
            StructPackage,
            StructName,
            RF_Public | RF_Standalone);

        if (ck::Is_NOT_Valid(SpawnParamsStructForEntity))
        {
            ck::ecs::Error(TEXT("Failed to create Spawn Params struct for EntityScript [{}]"), InEntityScriptClass);
            return {};
        }

        _EntitySpawnParams_Structs.Add(SpawnParamsStructForEntity);
        _EntitySpawnParams_StructsByName.Add(StructName, SpawnParamsStructForEntity);

        if (NOT ExposedProperties.IsEmpty())
        {
            if (UpdateStructProperties(SpawnParamsStructForEntity, ExposedProperties))
            {
                if (_StructsBeingSaved.Contains(SpawnParamsStructForEntity))
                { _StructsDirtyDuringSave.Add(SpawnParamsStructForEntity); }
                _EntitySpawnParams_StructsToSave.Add(SpawnParamsStructForEntity);
                Request_StartCompilationTicker();
            }
        }
    }
#endif

    return SpawnParamsStructForEntity;
}

auto
    UCk_EntityScript_Subsystem_UE::
    UpdateStructProperties(
        UUserDefinedStruct* InStruct,
        const TArray<FProperty*>& InNewProperties)
    -> bool
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(InStruct))
    { return false; }

    auto ExistingPropertiesMap = TMap<FName, FGuid>{};

    for (const auto& CurrentVars = FStructureEditorUtils::GetVarDesc(InStruct);
        const auto& VarDesc : CurrentVars)
    {
        ExistingPropertiesMap.Add(*VarDesc.FriendlyName, VarDesc.VarGuid);
    }

    // UserDefinedStructs cannot be empty, if it already had 1 property and it were to attempt to reduce it to 0,
    // abort and do not modify the structure. If we don't do that, containing BP will fail to compile
    if (InNewProperties.IsEmpty() && ExistingPropertiesMap.Num() == 1)
    { return true; }

    // Determine what changes are needed before calling ModifyStructData,
    // which marks the struct as modified and triggers dirty propagation in UE 5.7+
    auto PropertiesToChangeType = TArray<TPair<FGuid, FEdGraphPinType>>{};
    auto PropertiesToAdd = TArray<TPair<FName, FEdGraphPinType>>{};
    auto PropertiesToRemove = TArray<FGuid>{};
    auto RemainingExistingMap = ExistingPropertiesMap;

    for (const auto* NewProperty : InNewProperties)
    {
        // UUserDefinedStruct has no delegate properties — AddVariable silently falls back to Boolean
        // and would overwrite correctly-typed AngelScript/C++ spawn-params properties.
        if (UCk_Utils_Reflection_UE::Get_IsDelegateProperty(NewProperty))
        { continue; }

        const auto& PropertyName = NewProperty->GetFName();
        const auto& NewPinType = DecodePropertyAsPinType(NewProperty);

        if (const auto& FoundExistingGuid = RemainingExistingMap.Find(PropertyName);
            ck::IsValid(FoundExistingGuid, ck::IsValid_Policy_NullptrOnly{}))
        {
            const auto& ExistingGuid = *FoundExistingGuid;

            if (const auto* ExistingProperty = FStructureEditorUtils::GetPropertyByGuid(InStruct, ExistingGuid);
                ck::IsValid(ExistingProperty) && NOT UCk_Utils_Reflection_UE::Get_ArePropertiesCompatible(ExistingProperty, NewProperty))
            {
                PropertiesToChangeType.Emplace(ExistingGuid, NewPinType);
            }

            RemainingExistingMap.Remove(PropertyName);
        }
        else
        {
            PropertiesToAdd.Emplace(PropertyName, NewPinType);
        }
    }

    for (const auto& Pair : RemainingExistingMap)
    {
        PropertiesToRemove.Add(Pair.Value);
    }

    if (PropertiesToChangeType.IsEmpty() && PropertiesToAdd.IsEmpty() && PropertiesToRemove.IsEmpty())
    { return false; }

    FStructureEditorUtils::BroadcastPreChange(InStruct);
    FStructureEditorUtils::ModifyStructData(InStruct);

    for (const auto& [Guid, PinType] : PropertiesToChangeType)
    {
        FStructureEditorUtils::ChangeVariableType(InStruct, Guid, PinType);
    }

    for (const auto& [Name, PinType] : PropertiesToAdd)
    {
        FStructureEditorUtils::AddVariable(InStruct, PinType);

        if (const auto& UpdatedVars = FStructureEditorUtils::GetVarDesc(InStruct);
            UpdatedVars.Num() > 0)
        {
            const auto& NewVarGuid = UpdatedVars.Last().VarGuid;
            FStructureEditorUtils::RenameVariable(InStruct, NewVarGuid, Name.ToString());
        }
    }

    for (const auto& Guid : PropertiesToRemove)
    {
        FStructureEditorUtils::RemoveVariable(InStruct, Guid);
    }

    FStructureEditorUtils::OnStructureChanged(InStruct);
    FStructureEditorUtils::BroadcastPostChange(InStruct);
    FStructureEditorUtils::CompileStructure(InStruct);
    std::ignore = InStruct->MarkPackageDirty();

    return true;
#else
    return false;
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    RegisterForBlueprintChanges()
    -> void
{
#if WITH_EDITOR
    _OnObjectPreSave_DelegateHandle = FCoreUObjectDelegates::OnObjectPreSave.AddUObject(this, &UCk_EntityScript_Subsystem_UE::OnObjectSaved);

    if (ck::Is_NOT_Valid(GEditor))
    { return; }

    if (UCk_Utils_EditorOnly_UE::Get_IsCommandletOrCooking())
    { return; }
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    ScheduleDeferredStructUpdate()
    -> void
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(GEditor))
    { return; }

    if (_DeferredUpdateTimerHandle.IsValid())
    { return; }

    UWorld* World = nullptr;
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        if (Context.World() != nullptr)
        {
            World = Context.World();
            break;
        }
    }

    if (ck::Is_NOT_Valid(World))
    { return; }

    constexpr auto DelayForCompilationToFinish = 0.1f;
    constexpr auto Looping = false;

    auto WeakThis = TWeakObjectPtr(this);
    World->GetTimerManager().SetTimer(_DeferredUpdateTimerHandle,
        [WeakThis]()
        {
            if (ck::IsValid(WeakThis))
            {
                WeakThis->ProcessDeferredStructUpdates();
                WeakThis->_DeferredUpdateTimerHandle.Invalidate();
            }
        },
        DelayForCompilationToFinish,
        Looping);
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    ProcessDeferredStructUpdates()
    -> void
{
#if WITH_EDITOR
    if (NOT _bHasPendingStructUpdates)
    { return; }

    _bHasPendingStructUpdates = false;

    for (auto It = TObjectIterator<UClass>{}; It; ++It)
    {
        UClass* Class = *It;

        if (NOT Class->IsChildOf(UCk_EntityScript_UE::StaticClass()) ||
            UCk_Utils_IO_UE::Get_IsTemporaryAsset(Class->GetName()))
        { continue; }

        if (Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
        {
            constexpr auto ForceRecreate = true;
            std::ignore = GetOrCreate_SpawnParamsStructForEntity(Class, ForceRecreate);
        }
    }
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    ScanForExistingEntityParamsStructInPath(
        const FString& InPathToScan)
    -> void
{
    auto StructObjects = TArray<UObject*>{};
    FindOrLoadAssetsByPath(InPathToScan, StructObjects, EngineUtils::ATL_Regular);

    _EntitySpawnParams_Structs.Reserve(StructObjects.Num());
    for (auto* StructObject : StructObjects)
    {
        if (auto* Struct = Cast<UUserDefinedStruct>(StructObject);
            ck::IsValid(Struct))
        {
            if (UCk_Utils_IO_UE::Get_IsTemporaryAsset(Struct->GetName()))
            { continue; }

            _EntitySpawnParams_Structs.Add(Struct);
            _EntitySpawnParams_StructsByName.Add(Struct->GetFName(), Struct);
        }
    }
}

auto
    UCk_EntityScript_Subsystem_UE::
    OnObjectSaved(
        UObject* Object,
        FObjectPreSaveContext Context)
    -> void
{
#if WITH_EDITOR
    class FStructSaver : public FReferenceCollector
    {
        UCk_EntityScript_Subsystem_UE& _Owner;
        TSet<UObject*> _SerializedObjects;
        FProperty* _SerializedProperty;

    public:
        explicit FStructSaver(UCk_EntityScript_Subsystem_UE& InOwner)
            : _Owner(InOwner)
            , _SerializedProperty(nullptr)
        {}

        auto FindReferences(const UObject* Object, const UObject* InReferencingObject = nullptr) -> void
        {
            check(ck::IsValid(Object));

            if (NOT Object->GetClass()->IsChildOf(UClass::StaticClass()))
            {
                FVerySlowReferenceCollectorArchiveScope CollectorScope(GetVerySlowReferenceCollectorArchive(), InReferencingObject, _SerializedProperty);
                Object->SerializeScriptProperties(CollectorScope.GetArchive());
            }
        }

        auto HandleObjectReference(UObject*& InObject, const UObject* InReferencingObject, const FProperty* InReferencingProperty) -> void override
        {
            if (ck::Is_NOT_Valid(InObject))
            { return; }

            if (NOT TrySaveStruct(InObject))
            {
                if (const auto* Node = Cast<UEdGraphNode>(InObject);
                    ck::IsValid(Node))
                {
                    for (const auto* Pin : Node->Pins)
                    {
                        if (ck::Is_NOT_Valid(Pin, ck::IsValid_Policy_NullptrOnly{}))
                        { continue; }

                        if (Pin->PinType.PinSubCategoryObject.IsValid())
                        {
                            TrySaveStruct(Pin->PinType.PinSubCategoryObject.Get());
                        }
                        if (Pin->PinType.PinValueType.TerminalSubCategoryObject.IsValid())
                        {
                            TrySaveStruct(Pin->PinType.PinValueType.TerminalSubCategoryObject.Get());
                        }
                    }
                }
            }

            if (NOT _SerializedObjects.Contains(InObject))
            {
                _SerializedObjects.Add(InObject);
                FindReferences(InObject, InReferencingObject);
            }
        }

        auto IsIgnoringArchetypeRef() const -> bool override { return true; }
        auto IsIgnoringTransient() const -> bool override { return true; }

        auto SetSerializedProperty(FProperty* InProperty) -> void override
        {
            _SerializedProperty = InProperty;
        }
        auto GetSerializedProperty() const -> FProperty* override
        {
            return _SerializedProperty;
        }

    private:
        auto TrySaveStruct(UObject* InObject) const -> bool
        {
            if (const auto Struct = TObjectPtr<UUserDefinedStruct>{Cast<UUserDefinedStruct>(InObject)};
                _Owner._EntitySpawnParams_StructsToSave.Contains(Struct.Get()))
            {
                if (_Owner._StructsBeingSaved.Contains(Struct))
                { return true; }

                std::ignore = _Owner.TrySavePendingStruct(Struct);
                return true;
            }

            return false;
        }
    };

    if (NOT _EntitySpawnParams_StructsToSave.IsEmpty())
    {
        if (const auto& Blueprint = Cast<UBlueprint>(Object);
            ck::IsValid(Blueprint))
        {
            FStructSaver{*this}.FindReferences(Blueprint);
        }
    }
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    OnFilesLoaded()
    -> void
{
#if WITH_EDITOR
    // Asset-registry query + FindObject, never FindOrLoadAssetsByPath: that one triggers sync package
    // loading, which re-enters Blueprint compilation. FindObject never loads.
    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
        // ALL UUserDefinedStruct assets, not just /Game/, so plugin spawn-params structs are found too.
        auto StructAssets = TArray<FAssetData>{};
        auto Filter = FARFilter{};
        Filter.ClassPaths.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());

        AssetRegistry->GetAssets(Filter, StructAssets);

        for (const auto& Asset : StructAssets)
        {
            if (NOT Asset.AssetName.ToString().StartsWith(_SpawnParamsStructName_Prefix))
            { continue; }

            if (_EntitySpawnParams_StructsByName.Contains(Asset.AssetName))
            { continue; }

            if (auto* Struct = FindObject<UUserDefinedStruct>(nullptr, *Asset.GetObjectPathString());
                ck::IsValid(Struct))
            {
                _EntitySpawnParams_Structs.Add(Struct);
                _EntitySpawnParams_StructsByName.Add(Asset.AssetName, Struct);
            }
        }

        _OnAssetAdded_DelegateHandle = AssetRegistry->OnAssetAdded().AddUObject(this, &ThisType::OnAssetAdded);
        _OnAssetRemoved_DelegateHandle = AssetRegistry->OnAssetRemoved().AddUObject(this, &ThisType::OnAssetRemoved);
        _OnAssetRenamed_DelegateHandle = AssetRegistry->OnAssetRenamed().AddUObject(this, &ThisType::OnAssetRenamed);
    }

    RegisterForBlueprintChanges();
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    IsEntityScriptStructData(
        const FAssetData& AssetData)
    -> bool
{
    return AssetData.GetClass() == UUserDefinedStruct::StaticClass() && AssetData.AssetName.ToString().StartsWith(_SpawnParamsStructName_Prefix);
}

auto
    UCk_EntityScript_Subsystem_UE::
    OnAssetAdded(
        const FAssetData& InAssetData)
    -> void
{
#if WITH_EDITOR
    if (UCk_Utils_EditorOnly_UE::Get_IsCommandletOrCooking())
    { return; }

    if (IsEntityScriptStructData(InAssetData) && NOT _EntitySpawnParams_StructsByName.Contains(InAssetData.AssetName))
    {
        if (auto* Added = Cast<UUserDefinedStruct>(InAssetData.GetAsset());
            ck::IsValid(Added))
        {
            _EntitySpawnParams_Structs.Add(Added);
            _EntitySpawnParams_StructsByName.Add(InAssetData.AssetName, Added);
        }
        return;
    }

    if (const auto& BlueprintClassPath = UBlueprint::StaticClass()->GetClassPathName();
        InAssetData.AssetClassPath != BlueprintClassPath)
    { return; }

    FString ParentClassPath;
    if (NOT InAssetData.GetTagValue(FBlueprintTags::ParentClassPath, ParentClassPath))
    { return; }

    const auto& ParentClassName = FPackageName::ExportTextPathToObjectPath(ParentClassPath);
    const auto& ParentClass = FindObject<UClass>(nullptr, *ParentClassName);

    if (ck::Is_NOT_Valid(ParentClass))
    { return; }

    if (NOT ParentClass->IsChildOf(UCk_EntityScript_UE::StaticClass()) && ParentClass != UCk_EntityScript_UE::StaticClass())
    { return; }

    ck::ecs::Display(TEXT("New EntityScript blueprint detected: {} - Creating config struct..."), InAssetData);

    const auto& Blueprint = Cast<UBlueprint>(InAssetData.GetAsset());

    if (ck::Is_NOT_Valid(Blueprint))
    { return; }

    const auto& BlueprintGeneratedClass = Blueprint->GeneratedClass;

    if (ck::Is_NOT_Valid(BlueprintGeneratedClass))
    { return; }

    constexpr auto ForceRecreate = true;
    std::ignore = GetOrCreate_SpawnParamsStructForEntity(BlueprintGeneratedClass, ForceRecreate);
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    OnAssetRenamed(
        const FAssetData& InAssetData,
        const FString& InOldObjectPath)
    -> void
{
#if WITH_EDITOR
    if (IsEntityScriptStructData(InAssetData))
    {
        ck::ecs::Display(TEXT("Entity Spawn Params struct renamed from [{}] to [{}]"), InOldObjectPath, InAssetData);
        return;
    }

    if (const auto& BlueprintClassPath = UBlueprint::StaticClass()->GetClassPathName();
        InAssetData.AssetClassPath != BlueprintClassPath)
    { return; }

    auto NativeParentClassPath = FString{};
    if (NOT InAssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, NativeParentClassPath))
    { return; }

    const auto& ParentClassName = FPackageName::ExportTextPathToObjectPath(NativeParentClassPath);
    const auto& ParentClass = FindObject<UClass>(nullptr, *ParentClassName);

    if (ck::Is_NOT_Valid(ParentClass))
    { return; }

    if (NOT ParentClass->IsChildOf(UCk_EntityScript_UE::StaticClass()) && ParentClass != UCk_EntityScript_UE::StaticClass())
    { return; }

    const auto& NewObjectPath = InAssetData.GetObjectPathString();

    ck::ecs::Display(TEXT("EntityScript blueprint renamed from [{}] to [{}] - Updating its associated Spawn Params struct..."), InOldObjectPath, NewObjectPath);

    auto OldAssetShortName = FPaths::GetBaseFilename(InOldObjectPath);
    auto NewAssetShortName = InAssetData.AssetName.ToString();
    if (UCk_Utils_IO_UE::Get_IsTemporaryAsset(OldAssetShortName) ||
        UCk_Utils_IO_UE::Get_IsTemporaryAsset(NewAssetShortName))
    { return; }
    OldAssetShortName.RemoveFromStart(TEXT("BP_"));
    NewAssetShortName.RemoveFromStart(TEXT("BP_"));
    const auto& OldStructName = FName{ck::Format_UE(TEXT("{}{}"), _SpawnParamsStructName_Prefix, OldAssetShortName)};
    const auto& NewStructName = FName{ck::Format_UE(TEXT("{}{}"), _SpawnParamsStructName_Prefix, NewAssetShortName)};

    const auto OldPackagePath = Get_StructPathForEntityScriptPath(InOldObjectPath) / OldStructName.ToString();
    const auto NewPackagePath = Get_StructPathForEntityScriptPath(NewObjectPath) / NewStructName.ToString();
    if (OldPackagePath == NewPackagePath)
    { return; }

    QueueStructAssetOperation(FPendingStructAssetOperation{
        OldPackagePath, NewPackagePath, OldStructName, NewStructName, false});
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    OnAssetRemoved(
        const FAssetData& InAssetData)
    -> void
{
#if WITH_EDITOR
    if (IsEntityScriptStructData(InAssetData))
    {
        const auto Cached = TObjectPtr<UUserDefinedStruct>{
            _EntitySpawnParams_StructsByName.FindRef(InAssetData.AssetName)};
        if (ck::IsValid(Cached.Get()) && Cached->GetPathName() == InAssetData.GetObjectPathString())
        {
            _EntitySpawnParams_Structs.Remove(Cached.Get());
            _EntitySpawnParams_StructsByName.Remove(InAssetData.AssetName);
            _EntitySpawnParams_StructsToSave.Remove(Cached.Get());
        }
        return;
    }

    if (const auto& BlueprintClassPath = UBlueprint::StaticClass()->GetClassPathName();
        InAssetData.AssetClassPath != BlueprintClassPath)
    { return; }

    auto NativeParentClassPath = FString{};
    if (NOT InAssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, NativeParentClassPath))
    { return; }

    const auto& ParentClassName = FPackageName::ExportTextPathToObjectPath(NativeParentClassPath);
    const auto& ParentClass = FindObject<UClass>(nullptr, *ParentClassName);

    if (ck::Is_NOT_Valid(ParentClass))
    { return; }

    if (NOT ParentClass->IsChildOf(UCk_EntityScript_UE::StaticClass()) && ParentClass != UCk_EntityScript_UE::StaticClass())
    { return; }

    const auto& DeletedObjectPath = InAssetData.GetObjectPathString();
    const auto& DeletedAssetSpawnParamsStructPath = Get_StructPathForEntityScriptPath(DeletedObjectPath);

    ck::ecs::Display(TEXT("EntityScript blueprint [{}] has been deleted - Removing its associated Spawn Params struct..."), DeletedObjectPath);

    auto DeletedAssetShortName = FPaths::GetBaseFilename(DeletedObjectPath);
    if (UCk_Utils_IO_UE::Get_IsTemporaryAsset(DeletedAssetShortName))
    { return; }
    DeletedAssetShortName.RemoveFromStart(TEXT("BP_"));
    const auto& DeletedAssetStructName = FName{ck::Format_UE(TEXT("{}{}"), _SpawnParamsStructName_Prefix, DeletedAssetShortName)};
    const auto DeletedAssetStructPackagePath = DeletedAssetSpawnParamsStructPath / DeletedAssetStructName.ToString();
    QueueStructAssetOperation(FPendingStructAssetOperation{
        DeletedAssetStructPackagePath, {}, DeletedAssetStructName, {}, true});
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    SaveStruct(
        UUserDefinedStruct* InStructToSave)
    -> bool
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(GEngine) || NOT GEngine->bIsInitialized)
    { return false; }

    if (ck::Is_NOT_Valid(GEditor))
    { return false; }

    if (ck::Is_NOT_Valid(InStructToSave))
    { return false; }

    const auto& StructToSavePackage = InStructToSave->GetPackage();

    if (ck::Is_NOT_Valid(StructToSavePackage))
    { return false; }

    const auto& PackageName = StructToSavePackage->GetName();

    const auto& EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    if (ck::Is_NOT_Valid(EditorAssetSubsystem))
    { return false; }
    return EditorAssetSubsystem->SaveAsset(PackageName);
#else
    return false;
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    Get_StructPathForEntityScriptPath(
        const FString& InEntityScriptFullPath)
    -> FString
{
    auto DefaultPath = ck::Format_UE(TEXT("/Game/{}"), _EntitySpawnParams_StructFolderName);

    if (InEntityScriptFullPath.Len() < 2 || InEntityScriptFullPath[0] != TEXT('/'))
    { return DefaultPath; }

    // Extract the content root directly from the path (e.g. "CkFoundation" from "/CkFoundation/Path/Asset")
    // to avoid dependency on SplitLongPackageName mount point registration timing.
    auto PathAfterLeadingSlash = InEntityScriptFullPath.Mid(1);
    auto SlashIdx = int32{INDEX_NONE};

    if (NOT PathAfterLeadingSlash.FindChar(TEXT('/'), SlashIdx))
    { return DefaultPath; }

    auto ContentRoot = PathAfterLeadingSlash.Left(SlashIdx);

    if (ContentRoot.IsEmpty() || ContentRoot == TEXT("Script"))
    { return DefaultPath; }

    return ck::Format_UE(TEXT("/{}/{}"), ContentRoot, _EntitySpawnParams_StructFolderName);
}

#if WITH_EDITOR
auto
    UCk_EntityScript_Subsystem_UE::
    DecodePropertyAsPinType(
        const FProperty* InProperty)
    -> FEdGraphPinType
{
    auto PinType = FEdGraphPinType{};
    const auto* GraphSchema = GetDefault<UEdGraphSchema_K2>();
    GraphSchema->ConvertPropertyToPinType(InProperty, PinType);
    return PinType;
}
#endif

auto
    UCk_EntityScript_Subsystem_UE::
    GenerateEntitySpawnParamsStructName(
        const UClass* InEntityScriptClass)
    -> FName
{
    auto ClassName = InEntityScriptClass->GetName();

    if (ClassName.StartsWith(TEXT("BP_")))
    {
        ClassName.RightChopInline(3);
    }

    if (ClassName.EndsWith(TEXT("_C")))
    {
        ClassName.LeftChopInline(2);
    }

    return *ck::Format_UE(TEXT("{}{}"), _SpawnParamsStructName_Prefix, ClassName);
}

// -----------------------------------------------------------------------------------------------------------
