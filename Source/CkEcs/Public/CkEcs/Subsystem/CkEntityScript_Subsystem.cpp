#include "CkEntityScript_Subsystem.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/CkEcsLog.h"

#include <UObject/ObjectSaveContext.h>
#include <EngineUtils.h>
#include <AssetRegistry/IAssetRegistry.h>

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
    const auto& StructFolderPath_Game = ck::Format_UE(TEXT("/Game/{}"), _EntitySpawnParams_StructFolderName);

    ScanForExistingEntityParamsStructInPath(StructFolderPath_Game);

    // Scan all possible paths for EntityScript structs using asset registry
#if WITH_EDITOR
    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
        TArray<FAssetData> StructAssets;
        FARFilter Filter;
        Filter.ClassPaths.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());

        AssetRegistry->GetAssets(Filter, StructAssets);

        for (const auto& Asset : StructAssets)
        {
            if (Asset.AssetName.ToString().StartsWith(_SpawnParamsStructName_Prefix))
            {
                if (auto* Struct = Cast<UUserDefinedStruct>(Asset.GetAsset()))
                {
                    _EntitySpawnParams_Structs.Add(Struct);
                    _EntitySpawnParams_StructsByName.Add(Asset.AssetName, Struct);
                }
            }
        }
    }
#endif

    // Process any EntityScripts that are already loaded at startup
    for (auto It = TObjectIterator<UClass>{}; It; ++It)
    {
        UClass* Class = *It;

        if (Class->IsChildOf(UCk_EntityScript_UE::StaticClass()) &&
            NOT IsTemporaryAsset(Class->GetName()) &&
            Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
        {
            // Create structs for existing EntityScripts
            std::ignore = DoGetOrCreate_SpawnParamsStructForEntity_Internal(Class, false);
        }
    }

    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
        _OnFilesLoaded_DelegateHandle = AssetRegistry->OnFilesLoaded().AddUObject(this, &ThisType::OnFilesLoaded);
    }

#if WITH_EDITOR
    // Add compilation safety delegates - hook PreCompile and Compiled for deferred updates
    if (ck::IsValid(GEditor))
    {
        // Instead of updating immediately, defer the updates to avoid compilation conflicts
        const auto RequestDeferredUpdate = [this]()
        {
            _bHasPendingStructUpdates = true;
            ScheduleDeferredStructUpdate();
        };

        _OnBlueprintCompiled_DelegateHandle = GEditor->OnBlueprintPreCompile().AddLambda([this](UBlueprint* InBlueprint)
        {
            // Track compilation for safety
            _ActiveCompilations++;
            _IsCompilationInProgress = true;
            if (_ActiveCompilations == 1)
            {
                Request_StartCompilationTicker();
            }

            // Process EntityScript structs immediately during startup
            if (ck::IsValid(InBlueprint) && ck::IsValid(InBlueprint->GeneratedClass))
            {
                if (InBlueprint->GeneratedClass->IsChildOf(UCk_EntityScript_UE::StaticClass()) &&
                    NOT IsTemporaryAsset(InBlueprint->GeneratedClass->GetName()))
                {
                    std::ignore = DoGetOrCreate_SpawnParamsStructForEntity_Internal(InBlueprint->GeneratedClass, false);
                }
            }
        });

        _OnBlueprintReinstanced_DelegateHandle = GEditor->OnBlueprintCompiled().AddLambda([this, RequestDeferredUpdate]()
        {
            // Track compilation end and schedule deferred update
            _ActiveCompilations = FMath::Max(0, _ActiveCompilations - 1);
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
    _PendingSpawnParamsRequests.Empty();

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
    if (_ActiveCompilations <= 0)
    {
        _ActiveCompilations = 0;
        _IsCompilationInProgress = false;

        Request_ProcessPendingSpawnParamsRequests();
        Request_StopCompilationTicker();
        return false; // Stop ticking
    }

    return true; // Continue ticking
}

auto
    UCk_EntityScript_Subsystem_UE::
    Request_ProcessPendingSpawnParamsRequests() -> void
{
    if (_PendingSpawnParamsRequests.IsEmpty())
    { return; }

    // Process all pending requests
    for (auto& Request : _PendingSpawnParamsRequests)
    {
        if (ck::Is_NOT_Valid(Request.EntityScriptClass.Get()))
        { continue; }

        auto* Result = DoGetOrCreate_SpawnParamsStructForEntity_Internal(
            Request.EntityScriptClass.Get(),
            Request.ForceRecreate);

        // Fulfill any promises
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

    // Check cache first unless forcing recreate
    if (NOT InForceRecreate)
    {
        const auto& StructName = GenerateEntitySpawnParamsStructName(InEntityScriptClass);
        if (const auto& FoundExistingStruct = _EntitySpawnParams_StructsByName.Find(StructName);
            ck::IsValid(FoundExistingStruct, ck::IsValid_Policy_NullptrOnly{}))
        {
            return *FoundExistingStruct;
        }
    }

    // Check if compilation is in progress
    if (_IsCompilationInProgress)
    {
        // Find existing pending request or create new one
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

    if (NOT _IsCompilationInProgress)
    {
        auto* Result = DoGetOrCreate_SpawnParamsStructForEntity_Internal(InEntityScriptClass, InForceRecreate);
        Promise->SetValue(Result);
    }
    else
    {
        // Defer the request
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

    if (IsTemporaryAsset(InEntityScriptClass->GetName()))
    { return {}; }

    const auto& StructName = GenerateEntitySpawnParamsStructName(InEntityScriptClass);

    if (NOT InForceRecreate)
    {
        if (const auto& FoundExistingStruct = _EntitySpawnParams_StructsByName.Find(StructName);
            ck::IsValid(FoundExistingStruct, ck::IsValid_Policy_NullptrOnly{}))
        {
            return *FoundExistingStruct;
        }
    }

    UUserDefinedStruct* SpawnParamsStructForEntity = nullptr;

#if WITH_EDITOR
    const auto& ExposedProperties = UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass(InEntityScriptClass);

    {
        // load assets by path that follow the convention of EntityScript Spawn Params structs
        const auto StructPackageName = Get_StructPathForEntityScriptPath(InEntityScriptClass->GetPackage()->GetName());
        ScanForExistingEntityParamsStructInPath(StructPackageName);
    }

    if (const auto& FoundExistingStruct = _EntitySpawnParams_StructsByName.Find(StructName);
        ck::IsValid(FoundExistingStruct, ck::IsValid_Policy_NullptrOnly{}))
    {
        SpawnParamsStructForEntity = *FoundExistingStruct;

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
            _EntitySpawnParams_StructsToSave.Add(SpawnParamsStructForEntity);
        }
    }

    if (ck::Is_NOT_Valid(SpawnParamsStructForEntity))
    {
        const auto StructPackageName = Get_StructPathForEntityScriptPath(InEntityScriptClass->GetPackage()->GetName()) / StructName.ToString();
        auto* StructPackage = CreatePackage(*StructPackageName);

        if (NOT StructPackage->IsFullyLoaded())
        { return {}; }

        // without checking for this, we eventually experience a crash (although, we don't crash _all_ the time)
        // in UObjectGlobals:3465 because the dependent struct has not yet loaded
        if (auto Obj = StaticFindObjectFastInternal(nullptr, StructPackage, StructName, true);
            ck::IsValid(Obj) && (Obj->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad | RF_ClassDefaultObject) || Obj->GetClass()->bLayoutChanging))
        { return {}; }

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
                _EntitySpawnParams_StructsToSave.Add(SpawnParamsStructForEntity);
            }
        }
    }
#endif

    return SpawnParamsStructForEntity;
}

auto
    UCk_EntityScript_Subsystem_UE::
    IsTemporaryAsset(
        const FString& InAssetName)
    -> bool
{
    return InAssetName.StartsWith(TEXT("REINST_")) ||
           InAssetName.StartsWith(TEXT("SKEL_")) ||
           InAssetName.StartsWith(TEXT("TRASHCLASS_")) ||
           InAssetName.StartsWith(TEXT("DEADCLASS_")) ||
           InAssetName.StartsWith(TEXT("LIVECODING_")) ||
           InAssetName.Contains(TEXT("_INST_")) ||
           InAssetName.Contains(TEXT("_REPLACED_"));
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

    FStructureEditorUtils::BroadcastPreChange(InStruct);
    FStructureEditorUtils::ModifyStructData(InStruct);

    for (const auto* NewProperty : InNewProperties)
    {
        const auto& PropertyName = NewProperty->GetFName();
        const auto& NewPinType = DecodePropertyAsPinType(NewProperty);

        if (const auto& FoundExistingGuid = ExistingPropertiesMap.Find(PropertyName);
            ck::IsValid(FoundExistingGuid, ck::IsValid_Policy_NullptrOnly{}))
        {
            const auto& ExistingGuid = *FoundExistingGuid;

            if (const auto* ExistingProperty = FStructureEditorUtils::GetPropertyByGuid(InStruct, ExistingGuid);
                ck::IsValid(ExistingProperty, ck::IsValid_Policy_NullptrOnly{}) && NOT UCk_Utils_Reflection_UE::Get_ArePropertiesCompatible(ExistingProperty, NewProperty))
            {
                FStructureEditorUtils::ChangeVariableType(InStruct, ExistingGuid, NewPinType);
            }

            ExistingPropertiesMap.Remove(PropertyName);
        }
        else
        {
            FStructureEditorUtils::AddVariable(InStruct, NewPinType);

            if (const auto& UpdatedVars = FStructureEditorUtils::GetVarDesc(InStruct);
                UpdatedVars.Num() > 0)
            {
                const auto& NewVarGuid = UpdatedVars.Last().VarGuid;
                FStructureEditorUtils::RenameVariable(InStruct, NewVarGuid, PropertyName.ToString());
            }
        }
    }

    // Any remaining in the map are properties that no longer exist - remove them
    for (const auto& Pair : ExistingPropertiesMap)
    {
        FStructureEditorUtils::RemoveVariable(InStruct, Pair.Value);
    }

    FStructureEditorUtils::OnStructureChanged(InStruct);
    FStructureEditorUtils::BroadcastPostChange(InStruct);
    FStructureEditorUtils::CompileStructure(InStruct);
    std::ignore = InStruct->MarkPackageDirty();

    SaveStruct(InStruct);

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

    // Note: Blueprint compilation delegates are already hooked up in Initialize()
    // This function now just handles the object save delegate
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

    // If we already have a pending update, don't schedule another one
    if (_DeferredUpdateTimerHandle.IsValid())
    { return; }

    // Get any valid world to use for the timer manager
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

    // Schedule the update for the next tick to ensure compilation has finished
    World->GetTimerManager().SetTimer(_DeferredUpdateTimerHandle,
        [this]()
        {
            ProcessDeferredStructUpdates();
            _DeferredUpdateTimerHandle.Invalidate();
        },
        0.1f, // Small delay to ensure compilation is complete
        false);
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

    // Process all EntityScript classes that need struct updates
    for (auto It = TObjectIterator<UClass>{}; It; ++It)
    {
        UClass* Class = *It;

        if (NOT Class->IsChildOf(UCk_EntityScript_UE::StaticClass()) ||
            IsTemporaryAsset(Class->GetName()))
        { continue; }

        // Only process blueprint generated classes - these are the ones that could have changed
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
            if (IsTemporaryAsset(Struct->GetName()))
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
        TSet<UUserDefinedStruct*>& _StructsToSave;
        TSet<UObject*> _SerializedObjects;
        FProperty* _SerializedProperty;

    public:
        explicit FStructSaver(TSet<UUserDefinedStruct*>& InStructsToSave)
            : _StructsToSave(InStructsToSave)
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
            if (auto* Struct = static_cast<UUserDefinedStruct*>(InObject);
                _StructsToSave.Contains(Struct))
            {
                _StructsToSave.Remove(Struct);
                SaveStruct(Struct);
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
            FStructSaver{_EntitySpawnParams_StructsToSave}.FindReferences(Blueprint);
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
    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
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

    auto ParentClassPath = FString{};
    if (NOT InAssetData.GetTagValue(FBlueprintTags::ParentClassPath, ParentClassPath))
    { return; }

    const auto& ParentClassName = FPackageName::ExportTextPathToObjectPath(ParentClassPath);
    const auto& ParentClass = FindObject<UClass>(nullptr, *ParentClassName);

    if (ck::Is_NOT_Valid(ParentClass))
    { return; }

    if (NOT ParentClass->IsChildOf(UCk_EntityScript_UE::StaticClass()) && ParentClass != UCk_EntityScript_UE::StaticClass())
    { return; }

    const auto& Blueprint = Cast<UBlueprint>(InAssetData.GetAsset());

    if (ck::Is_NOT_Valid(Blueprint))
    { return; }

    const auto& BlueprintGeneratedClass = Blueprint->GeneratedClass;

    if (ck::Is_NOT_Valid(BlueprintGeneratedClass))
    { return; }

    if (IsTemporaryAsset(BlueprintGeneratedClass->GetName()))
    { return; }

    const auto& NewObjectPath = InAssetData.GetObjectPathString();

    ck::ecs::Display(TEXT("EntityScript blueprint renamed from [{}] to [{}] - Updating its associated Spawn Params struct..."), InOldObjectPath, NewObjectPath);

    const auto& OldAssetShortName = FPaths::GetBaseFilename(InOldObjectPath);
    const auto& OldStructName = FName{ck::Format_UE(TEXT("{}{}"), _SpawnParamsStructName_Prefix, OldAssetShortName)};
    const auto& NewStructName = GenerateEntitySpawnParamsStructName(BlueprintGeneratedClass);

    UUserDefinedStruct* SpawnParamsStructForOldName = nullptr;

    if (auto* FoundStruct = _EntitySpawnParams_StructsByName.Find(OldStructName);
        ck::IsValid(FoundStruct, ck::IsValid_Policy_NullptrOnly{}))
    {
        SpawnParamsStructForOldName = *FoundStruct;
    }

    if (ck::Is_NOT_Valid(SpawnParamsStructForOldName))
    {
        ck::ecs::Display(TEXT("Could not find existing struct for renamed EntityScript [{}] (old path: [{}]). Creating new one."), InOldObjectPath, InAssetData);

        constexpr auto ForceRecreate = true;
        std::ignore = GetOrCreate_SpawnParamsStructForEntity(BlueprintGeneratedClass, ForceRecreate);
        return;
    }

    _EntitySpawnParams_StructsByName.Remove(OldStructName);
    _EntitySpawnParams_StructsByName.Add(NewStructName, SpawnParamsStructForOldName);

    const auto& RenamedAssetSpawnParamsStructPath = Get_StructPathForEntityScriptPath(NewObjectPath);

    const auto NewPackagePath = RenamedAssetSpawnParamsStructPath / NewStructName.ToString();
    const auto OldPackagePath = RenamedAssetSpawnParamsStructPath / OldStructName.ToString();

    const auto& EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    EditorAssetSubsystem->RenameAsset(OldPackagePath, NewPackagePath);
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
        const auto* Removed = Cast<UUserDefinedStruct>(InAssetData.GetAsset());
        _EntitySpawnParams_Structs.Remove(Removed);
        _EntitySpawnParams_Structs.Remove(nullptr);
        _EntitySpawnParams_StructsByName.Remove(InAssetData.AssetName);
        _EntitySpawnParams_StructsToSave.Remove(Removed);
        return;
    }

    if (const auto& BlueprintClassPath = UBlueprint::StaticClass()->GetClassPathName();
        InAssetData.AssetClassPath != BlueprintClassPath)
    { return; }

    auto ParentClassPath = FString{};
    if (NOT InAssetData.GetTagValue(FBlueprintTags::ParentClassPath, ParentClassPath))
    { return; }

    const auto& ParentClassName = FPackageName::ExportTextPathToObjectPath(ParentClassPath);
    const auto& ParentClass = FindObject<UClass>(nullptr, *ParentClassName);

    if (ck::Is_NOT_Valid(ParentClass))
    { return; }

    if (NOT ParentClass->IsChildOf(UCk_EntityScript_UE::StaticClass()) && ParentClass != UCk_EntityScript_UE::StaticClass())
    { return; }

    const auto& Blueprint = Cast<UBlueprint>(InAssetData.GetAsset());

    if (ck::Is_NOT_Valid(Blueprint))
    { return; }

    if (const auto& BlueprintGeneratedClass = Blueprint->GeneratedClass;
        ck::IsValid(BlueprintGeneratedClass))
    {
        if (IsTemporaryAsset(BlueprintGeneratedClass->GetName()))
        { return; }
    }

    const auto& DeletedObjectPath = InAssetData.GetObjectPathString();
    const auto& DeletedAssetSpawnParamsStructPath = Get_StructPathForEntityScriptPath(DeletedObjectPath);

    ck::ecs::Display(TEXT("EntityScript blueprint [{}] has been deleted - Removing its associated Spawn Params struct..."), DeletedObjectPath);

    const auto& DeletedAssetShortName = FPaths::GetBaseFilename(DeletedObjectPath);
    const auto& DeletedAssetStructName = FName{ck::Format_UE(TEXT("{}{}"), _SpawnParamsStructName_Prefix, DeletedAssetShortName)};
    const auto DeletedAssetStructPackagePath = DeletedAssetSpawnParamsStructPath / DeletedAssetStructName.ToString();

    const auto& EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    EditorAssetSubsystem->DeleteAsset(DeletedAssetStructPackagePath);
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    SaveStruct(
        UUserDefinedStruct* InStructToSave)
    -> void
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(InStructToSave))
    { return; }

    const auto& StructToSavePackage = InStructToSave->GetPackage();

    if (ck::Is_NOT_Valid(StructToSavePackage))
    { return; }

    const auto& PackageName = StructToSavePackage->GetName();

    const auto& EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    EditorAssetSubsystem->SaveAsset(PackageName);
#endif
}

auto
    UCk_EntityScript_Subsystem_UE::
    Get_StructPathForEntityScriptPath(
        const FString& InEntityScriptFullPath)
    -> FString
{
    auto DefaultPath = ck::Format_UE(TEXT("/Game/{}"), _EntitySpawnParams_StructFolderName);

    switch (const auto& AssetLocalRoot = UCk_Utils_IO_UE::Get_AssetLocalRoot(InEntityScriptFullPath))
    {
        case ECk_AssetLocalRootType::Project:
        {
            return ck::Format_UE(TEXT("/Game/{}"), _EntitySpawnParams_StructFolderName);
        }
        case ECk_AssetLocalRootType::Engine:
        {
            return ck::Format_UE(TEXT("/Engine/{}"), _EntitySpawnParams_StructFolderName);
        }
        case ECk_AssetLocalRootType::ProjectPlugin:
        case ECk_AssetLocalRootType::EnginePlugin:
        {
            auto PackageRoot = FString{};
            auto PackagePath = FString{};
            auto PackageName = FString{};

            constexpr auto StripRootLeadingSlash = false;
            FPackageName::SplitLongPackageName(InEntityScriptFullPath, PackageRoot, PackagePath, PackageName, StripRootLeadingSlash);

            return  ck::Format_UE(TEXT("{}{}"), PackageRoot, _EntitySpawnParams_StructFolderName);
        }
        case ECk_AssetLocalRootType::Invalid:
        {
            return DefaultPath;
        }
        default:
        {
            CK_INVALID_ENUM(AssetLocalRoot);
            return {};
        }
    }
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
