#include "CkEntityScript_Subsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/BlueprintSupport.h"
#include "Engine/Engine.h"
#include "Engine/UserDefinedStruct.h"
#include "EngineUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/StructureEditorUtils.h"

#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

auto UCk_EntityScript_Subsystem_UE::Initialize(FSubsystemCollectionBase& Collection) -> void
{
    Super::Initialize(Collection);

#if WITH_EDITOR
    if (ck::IsValid(GEditor))
    {
        _BlueprintPreCompileHandle = GEditor->OnBlueprintPreCompile().AddUObject(
            this, &UCk_EntityScript_Subsystem_UE::OnBlueprintPreCompile);

        _BlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddUObject(
            this, &UCk_EntityScript_Subsystem_UE::OnBlueprintCompiled);
    }
#endif

    UE_LOG(LogTemp, Log, TEXT("[EntityScript] Subsystem initialized with compilation safety"));
}

auto UCk_EntityScript_Subsystem_UE::Deinitialize() -> void
{
#if WITH_EDITOR
    if (ck::IsValid(GEditor))
    {
        if (_BlueprintPreCompileHandle.IsValid())
        {
            GEditor->OnBlueprintPreCompile().Remove(_BlueprintPreCompileHandle);
        }

        if (_BlueprintCompiledHandle.IsValid())
        {
            GEditor->OnBlueprintCompiled().Remove(_BlueprintCompiledHandle);
        }
    }
#endif

    _PendingSpawnParamsRequests.Empty();
    Super::Deinitialize();
}

#if WITH_EDITOR
void UCk_EntityScript_Subsystem_UE::OnBlueprintPreCompile(UBlueprint* InBlueprint)
{
    _ActiveCompilations++;
    _IsCompilationInProgress = true;

    UE_LOG(LogTemp, VeryVerbose, TEXT("[EntityScript] Blueprint pre-compile: %s (Active: %d)"),
        ck::IsValid(InBlueprint) ? *InBlueprint->GetName() : TEXT("Unknown"), _ActiveCompilations);
}

void UCk_EntityScript_Subsystem_UE::OnBlueprintCompiled()
{
    _ActiveCompilations--;

    if (_ActiveCompilations <= 0)
    {
        _ActiveCompilations = 0;
        _IsCompilationInProgress = false;

        UE_LOG(LogTemp, VeryVerbose, TEXT("[EntityScript] All compilations complete, processing %d pending requests"),
            _PendingSpawnParamsRequests.Num());

        Request_ProcessPendingSpawnParamsRequests();
    }
}
#endif

auto UCk_EntityScript_Subsystem_UE::GetOrCreate_SpawnParamsStructForEntity(
    UClass* InEntityScriptClass,
    bool InForceRecreate) -> UUserDefinedStruct*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScriptClass),
        TEXT("Invalid EntityScriptClass provided"))
    { return nullptr; }

    // Check if compilation is in progress
    const auto IsCompiling = _IsCompilationInProgress;

    if (IsCompiling)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[EntityScript] Deferring struct creation for %s (compilation in progress)"),
            *InEntityScriptClass->GetName());

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

auto UCk_EntityScript_Subsystem_UE::GetOrCreate_SpawnParamsStructForEntity_Async(
    UClass* InEntityScriptClass,
    bool InForceRecreate) -> TFuture<UUserDefinedStruct*>
{
    auto Promise = MakeShared<TPromise<UUserDefinedStruct*>>();
    auto Future = Promise->GetFuture();

    if (NOT _IsCompilationInProgress)
    {
        // Safe to process immediately
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

        if (ck::Is_NOT_Valid(ExistingRequest, ck::IsValid_Policy_NullptrOnly()))
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

auto UCk_EntityScript_Subsystem_UE::Request_ProcessPendingSpawnParamsRequests() -> void
{
    if (_PendingSpawnParamsRequests.IsEmpty())
    { return; }

    UE_LOG(LogTemp, Log, TEXT("[EntityScript] Processing %d pending spawn params requests"),
        _PendingSpawnParamsRequests.Num());

    // Process all pending requests
    for (auto& Request : _PendingSpawnParamsRequests)
    {
        if (ck::Is_NOT_Valid(Request.EntityScriptClass.Get()))
        {
            UE_LOG(LogTemp, Warning, TEXT("[EntityScript] Skipping request with invalid EntityScriptClass"));
            continue;
        }

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

auto UCk_EntityScript_Subsystem_UE::DoGetOrCreate_SpawnParamsStructForEntity_Internal(
    UClass* InEntityScriptClass,
    bool InForceRecreate) -> UUserDefinedStruct*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScriptClass),
        TEXT("Invalid EntityScriptClass provided"))
    { return nullptr; }

    // Check cache first unless forcing recreate
    if (NOT InForceRecreate)
    {
        if (auto* ExistingStruct = _EntitySpawnParams_StructsByClass.FindRef(InEntityScriptClass).Get())
        {
            return ExistingStruct;
        }
    }

    // Check if this EntityScript needs a spawn params struct
    if (NOT DoesEntityScriptNeedSpawnParamsStruct(InEntityScriptClass))
    {
        return nullptr;
    }

    const auto& ExpectedStructPath = Get_ExpectedEntitySpawnParamsStructPath();

    // Scan for existing structs in the expected path
    auto ExistingStructs = ScanForExistingEntityParamsStructInPath(ExpectedStructPath);

    const auto& ExpectedStructName = Get_SpawnParamsStructName(InEntityScriptClass);

    // Look for existing struct with matching name
    auto* ExistingStruct = ExistingStructs.FindByPredicate([&](UUserDefinedStruct* Struct)
    {
        return ck::IsValid(Struct) && Struct->GetName() == ExpectedStructName;
    });

    UUserDefinedStruct* ResultStruct = nullptr;

    if (ck::IsValid(ExistingStruct, ck::IsValid_Policy_NullptrOnly{}) && NOT InForceRecreate)
    {
        ResultStruct = *ExistingStruct;
        UE_LOG(LogTemp, Verbose, TEXT("[EntityScript] Using existing struct: %s"), *ResultStruct->GetName());
    }
    else
    {
        // Create new struct
        ResultStruct = Request_CreateNewSpawnParamsStruct(InEntityScriptClass);

        if (ck::IsValid(ResultStruct))
        {
            UE_LOG(LogTemp, Log, TEXT("[EntityScript] Created new struct: %s"), *ResultStruct->GetName());
        }
    }

    // Update caches
    if (ck::IsValid(ResultStruct))
    {
        _EntitySpawnParams_StructsByClass.Add(InEntityScriptClass, ResultStruct);
        _EntitySpawnParams_StructsByName.Add(ResultStruct->GetName(), ResultStruct);
        _EntitySpawnParams_Structs.Add(ResultStruct);
    }

    return ResultStruct;
}

auto UCk_EntityScript_Subsystem_UE::DoesEntityScriptNeedSpawnParamsStruct(UClass* InEntityScriptClass) -> bool
{
    if (ck::Is_NOT_Valid(InEntityScriptClass))
    { return false; }

    // Check if the EntityScript has any Blueprint-editable properties
    for (TFieldIterator<FProperty> PropIt(InEntityScriptClass, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
    {
        const auto* Property = *PropIt;

        if (ck::IsValid(Property) && Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
        {
            return true;
        }
    }

    return false;
}

auto UCk_EntityScript_Subsystem_UE::ScanForExistingEntityParamsStructInPath(const FString& InPathToScan) -> TArray<UUserDefinedStruct*>
{
    TArray<UUserDefinedStruct*> FoundStructs;

#if WITH_EDITOR
    if (IsRunningCookCommandlet() || GIsCookerLoadingPackage)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[EntityScript] Skipping asset scan during cooking"));
        return FoundStructs;
    }

    TArray<UObject*> AllAssets;
    EngineUtils::FindOrLoadAssetsByPath(InPathToScan, AllAssets, EngineUtils::ATL_Regular);

    for (auto* Asset : AllAssets)
    {
        if (auto* UserStruct = Cast<UUserDefinedStruct>(Asset))
        {
            if (UserStruct->GetName().Contains(TEXT("EntitySpawnParams")))
            {
                FoundStructs.Add(UserStruct);
            }
        }
    }
#endif

    return FoundStructs;
}

auto UCk_EntityScript_Subsystem_UE::Request_CreateNewSpawnParamsStruct(UClass* InEntityScriptClass) -> UUserDefinedStruct*
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(InEntityScriptClass))
    { return nullptr; }

    const auto& StructName = Get_SpawnParamsStructName(InEntityScriptClass);
    const auto& PackagePath = Get_SpawnParamsStructPackagePath(InEntityScriptClass);

    // Create package
    auto* Package = CreatePackage(*PackagePath);
    if (ck::Is_NOT_Valid(Package))
    {
        UE_LOG(LogTemp, Error, TEXT("[EntityScript] Failed to create package: %s"), *PackagePath);
        return nullptr;
    }

    // Create the struct
    auto* NewStruct = NewObject<UUserDefinedStruct>(Package, *StructName, RF_Public | RF_Standalone);
    if (ck::Is_NOT_Valid(NewStruct))
    {
        UE_LOG(LogTemp, Error, TEXT("[EntityScript] Failed to create struct: %s"), *StructName);
        return nullptr;
    }

    // Initialize struct
    NewStruct->EditorData = NewObject<UUserDefinedStructEditorData>(NewStruct, NAME_None, RF_Transactional);
    NewStruct->Guid = FGuid::NewGuid();
    NewStruct->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
    NewStruct->SetMetaData(TEXT("HasNativeMake"), TEXT("false"));
    NewStruct->SetMetaData(TEXT("HasNativeBreak"), TEXT("false"));

    // Add properties from EntityScript
    Request_AddPropertiesToStruct(NewStruct, InEntityScriptClass);

    // Compile the struct
    FStructureEditorUtils::OnStructureChanged(NewStruct, FStructureEditorUtils::EStructureEditorChangeInfo::DefaultValueChanged);

    // Save the struct
    if (Request_SaveStructAsset(NewStruct))
    {
        _EntitySpawnParams_StructsToSave.Add(NewStruct);
        return NewStruct;
    }

    return nullptr;
#else
    return nullptr;
#endif
}

auto UCk_EntityScript_Subsystem_UE::Request_AddPropertiesToStruct(UUserDefinedStruct* InStruct, UClass* InEntityScriptClass) -> void
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(InStruct) || ck::Is_NOT_Valid(InEntityScriptClass))
    { return; }

    // Add properties from EntityScript that are Blueprint-editable
    for (TFieldIterator<FProperty> PropIt(InEntityScriptClass, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
    {
        const auto* Property = *PropIt;

        if (ck::Is_NOT_Valid(Property) || NOT Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
        { continue; }

        // Set up the pin type
        FEdGraphPinType PinType;

        if (Property->IsA<FBoolProperty>())
        {
            PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        }
        else if (Property->IsA<FIntProperty>())
        {
            PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
        }
        else if (Property->IsA<FFloatProperty>())
        {
            PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
            PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
        }
        else if (Property->IsA<FStrProperty>())
        {
            PinType.PinCategory = UEdGraphSchema_K2::PC_String;
        }
        else if (const auto* ObjectProperty = CastField<FObjectProperty>(Property))
        {
            PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
            PinType.PinSubCategoryObject = ObjectProperty->PropertyClass;
        }
        else if (const auto* StructProperty = CastField<FStructProperty>(Property))
        {
            PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            PinType.PinSubCategoryObject = StructProperty->Struct;
        }

        // Add the variable to the struct
        if (FStructureEditorUtils::AddVariable(InStruct, PinType))
        {
            // Find the newly added variable and set its properties
            auto& VarDescs = FStructureEditorUtils::GetVarDesc(InStruct);
            if (NOT VarDescs.IsEmpty())
            {
                auto& LastVar = VarDescs.Last();

                // Update the variable properties
                FStructureEditorUtils::RenameVariable(InStruct, LastVar.VarGuid, Property->GetDisplayNameText().ToString());
                FStructureEditorUtils::ChangeVariableTooltip(InStruct, LastVar.VarGuid, Property->GetToolTipText().ToString());

                // Set default value if available
                FString DefaultValue;
                if (Property->ExportText_InContainer(0, DefaultValue, InEntityScriptClass->GetDefaultObject(),
                                                   InEntityScriptClass->GetDefaultObject(), nullptr, PPF_None))
                {
                    FStructureEditorUtils::ChangeVariableDefaultValue(InStruct, LastVar.VarGuid, DefaultValue);
                }

                // Set metadata
                if (const auto* CategoryMeta = Property->FindMetaData(TEXT("Category")))
                {
                    FStructureEditorUtils::SetMetaData(InStruct, LastVar.VarGuid, TEXT("Category"), *CategoryMeta);
                }
            }
        }
    }
#endif
}

auto UCk_EntityScript_Subsystem_UE::Get_SpawnParamsStructName(UClass* InEntityScriptClass) -> FString
{
    if (ck::Is_NOT_Valid(InEntityScriptClass))
    { return TEXT(""); }

    return ck::Format_UE(TEXT("EntitySpawnParams_{}_Struct"), InEntityScriptClass->GetName());
}

auto UCk_EntityScript_Subsystem_UE::Get_SpawnParamsStructPackagePath(UClass* InEntityScriptClass) -> FString
{
    if (ck::Is_NOT_Valid(InEntityScriptClass))
    { return TEXT(""); }

    const auto& StructName = Get_SpawnParamsStructName(InEntityScriptClass);
    const auto& BasePath = Get_ExpectedEntitySpawnParamsStructPath();

    return ck::Format_UE(TEXT("{}/{}.{}"), BasePath, StructName, StructName);
}

auto UCk_EntityScript_Subsystem_UE::Get_ExpectedEntitySpawnParamsStructPath() -> FString
{
    return TEXT("/Game/Generated/EntitySpawnParams");
}

auto UCk_EntityScript_Subsystem_UE::Request_SaveStructAsset(UUserDefinedStruct* InStruct) -> bool
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(InStruct))
    { return false; }

    auto* Package = InStruct->GetPackage();
    if (ck::Is_NOT_Valid(Package))
    { return false; }

    Package->SetDirtyFlag(true);

    const auto& PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(),
                                                                         FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

    return UPackage::SavePackage(Package, InStruct, *PackageFilename, SaveArgs);
#else
    return false;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
