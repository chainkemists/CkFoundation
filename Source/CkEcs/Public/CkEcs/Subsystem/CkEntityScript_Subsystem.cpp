#include "CkEntityScript_Subsystem.h"

#include "CkCore/Reflection/CkReflection_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/CkEcsLog.h"

#include <Interfaces/IPluginManager.h>
#include <UObject/ObjectSaveContext.h>
#include <UObject/SavePackage.h>
#include <EngineUtils.h>
#include <AssetRegistry/IAssetRegistry.h>

#if WITH_EDITOR
#include <Subsystems/EditorAssetSubsystem.h>
#include <BlueprintActionDatabase.h>
#include <ContentBrowserModule.h>
#include <ISourceControlModule.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <Kismet2/StructureEditorUtils.h>
#include <Editor/EditorEngine.h>
#include <UserDefinedStructure/UserDefinedStructEditorData.h>
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

    auto StructObjects = TArray<UObject*>{};
    FindOrLoadAssetsByPath(_EntitySpawnParams_StructFolderName, StructObjects, EngineUtils::ATL_Regular);

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

    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
        _OnAssetAdded_DelegateHandle = AssetRegistry->OnAssetAdded().AddUObject(this, &UCk_EntityScript_Subsystem_UE::OnAssetAdded);
        _OnAssetRemoved_DelegateHandle = AssetRegistry->OnAssetRemoved().AddUObject(this, &UCk_EntityScript_Subsystem_UE::OnAssetRemoved);
        _OnAssetRenamed_DelegateHandle = AssetRegistry->OnAssetRenamed().AddUObject(this, &UCk_EntityScript_Subsystem_UE::OnAssetRenamed);
    }

    RegisterForBlueprintChanges();
}

auto
    UCk_EntityScript_Subsystem_UE::
    Deinitialize()
    -> void
{
    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
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

    Super::Deinitialize();
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
           InAssetName.Contains(TEXT("_INST_"));
}

auto
    UCk_EntityScript_Subsystem_UE::
    GetOrCreate_SpawnParamsStructForEntity(
        UClass* InEntityScriptClass,
        bool InForceRecreate)
    -> UUserDefinedStruct*
{
    if (ck::Is_NOT_Valid(InEntityScriptClass))
    { return {}; }

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

    if (const auto& FoundExistingStruct = _EntitySpawnParams_StructsByName.Find(StructName);
        ck::IsValid(FoundExistingStruct, ck::IsValid_Policy_NullptrOnly{}))
    {
        SpawnParamsStructForEntity = *FoundExistingStruct;

        auto ExistingProperties = TArray<FProperty*>{};
        for (TFieldIterator<FProperty> PropIt(SpawnParamsStructForEntity); PropIt; ++PropIt)
        {
            ExistingProperties.Add(*PropIt);
        }

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
        else
        {
            SaveStruct(SpawnParamsStructForEntity);
        }
    }
#endif

    return SpawnParamsStructForEntity;
}

auto
    UCk_EntityScript_Subsystem_UE::
    UpdateStructProperties(
        UUserDefinedStruct* InStruct,
        const TArray<FProperty*>& InNewProperties) const
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

    const auto TryUpdateAllEntitySpawnParamStructs = [this]()
    {
        for (TObjectIterator<UClass> It; It; ++It)
        {
            UClass* Class = *It;

            if (NOT Class->IsChildOf(UCk_EntityScript_UE::StaticClass()) ||
                Class == UCk_EntityScript_UE::StaticClass() ||
                IsTemporaryAsset(Class->GetName()))
            { continue; }

            // Only process blueprint generated classes - these are the ones that could have changed
            if (Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
            {
                constexpr auto ForceRecreate = true;
                std::ignore = GetOrCreate_SpawnParamsStructForEntity(Class, ForceRecreate);
            }
        }
    };

    _OnBlueprintCompiled_DelegateHandle = GEditor->OnBlueprintCompiled().AddLambda(TryUpdateAllEntitySpawnParamStructs);
    _OnBlueprintReinstanced_DelegateHandle = GEditor->OnBlueprintReinstanced().AddLambda(TryUpdateAllEntitySpawnParamStructs);
#endif
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
        TSet<UUserDefinedStruct*>& StructsToSave;
        TSet<UObject*> SerializedObjects;
        FProperty* SerializedProperty;

    public:
        explicit FStructSaver(TSet<UUserDefinedStruct*>& InStructsToSave)
            : StructsToSave(InStructsToSave)
            , SerializedProperty(nullptr)
        {}

        auto FindReferences(const UObject* Object, const UObject* InReferencingObject = nullptr) -> void
        {
            check(ck::IsValid(Object));

            if (NOT Object->GetClass()->IsChildOf(UClass::StaticClass()))
            {
                FVerySlowReferenceCollectorArchiveScope CollectorScope(GetVerySlowReferenceCollectorArchive(), InReferencingObject, SerializedProperty);
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

            if (NOT SerializedObjects.Contains(InObject))
            {
                SerializedObjects.Add(InObject);
                FindReferences(InObject, InReferencingObject);
            }
        }

        auto IsIgnoringArchetypeRef() const -> bool override { return true; }
        auto IsIgnoringTransient() const -> bool override { return true; }

        auto SetSerializedProperty(FProperty* InProperty) -> void override
        {
            SerializedProperty = InProperty;
        }
        auto GetSerializedProperty() const -> FProperty* override
        {
            return SerializedProperty;
        }

    private:
        auto TrySaveStruct(UObject* InObject) const -> bool
        {
            if (auto* Struct = static_cast<UUserDefinedStruct*>(InObject);
                StructsToSave.Contains(Struct))
            {
                StructsToSave.Remove(Struct);
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
