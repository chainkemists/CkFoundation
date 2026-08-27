#include "CkDynamicHandleSubsystem.h"

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"

#include "CkDynamic/Settings/CkDynamic_Settings.h"
#include "CkDynamic/CkDynamic_AngelScript.h"
#include "CkDynamic/CkDynamic_HandleDefinition.h"

#include "CkCore/Format/CkFormat.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <HAL/FileManager.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptManager.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamicHandleSubsystem::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);
    ck::angelscriptgenerator::Log(TEXT("[DynamicHandleSubsystem] Initialized"));

    auto& AssetRegistry = FAssetRegistryModule::GetRegistry();
    if (AssetRegistry.IsLoadingAssets())
    {
        _AssetRegistryFilesLoadedHandle = AssetRegistry.OnFilesLoaded().AddUObject(
            this, &UCkDynamicHandleSubsystem::ReconcileRegistryIfNeeded);
    }
    else
    {
        ReconcileRegistryIfNeeded();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamicHandleSubsystem::
    Deinitialize()
    -> void
{
    if (_AssetRegistryFilesLoadedHandle.IsValid())
    {
        if (auto* Module = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry")))
        {
            Module->Get().OnFilesLoaded().Remove(_AssetRegistryFilesLoadedHandle);
        }
        _AssetRegistryFilesLoadedHandle.Reset();
    }

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamicHandleSubsystem::
    ReconcileRegistryIfNeeded()
    -> void
{
    if (_AssetRegistryFilesLoadedHandle.IsValid())
    {
        FAssetRegistryModule::GetRegistry().OnFilesLoaded().Remove(_AssetRegistryFilesLoadedHandle);
        _AssetRegistryFilesLoadedHandle.Reset();
    }

    if (NOT IsRegenerationNeeded())
    { return; }

    ck::angelscriptgenerator::Log(
        TEXT("[DynamicHandleSubsystem] Live handle definitions differ from the canonical registry; regenerating."));
    GenerateHandleTypeRegistry();
    FCkDynamic_HandleTypeRegistry::DiscoverAndRegisterAllDefinitions();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamicHandleSubsystem::
    Get_RegistryFilePath()
    -> FString
{
    return UCk_Utils_Dynamic_Settings_UE::Get_DynamicHandleRegistryFilePath();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamicHandleSubsystem::
    GenerateHandleTypeRegistry()
    -> void
{
    // Also a CallInEditor button, so a secondary must Warn rather than skip silently, and must
    // still broadcast completion or editor listeners hang.
    if (NOT FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(
            TEXT("DynamicHandleSubsystem.GenerateHandleTypeRegistry")))
    {
        ck::angelscriptgenerator::Warning(
            TEXT("[DynamicHandleSubsystem] Skipped registry generation - this editor instance is a ")
            TEXT("SECONDARY (another instance owns Script/Generated regen). Close the other instance ")
            TEXT("or run this from the owning one."));
        OnGenerationComplete.Broadcast(0, false);
        return;
    }

    auto Definitions = DiscoverAllDefinitions();

    Definitions.Sort([](const UCkDynamic_HandleDefinition& A, const UCkDynamic_HandleDefinition& B)
    {
        return A.TypeName < B.TypeName;
    });

    auto JsonContent = BuildJsonContent(Definitions);

    auto OutputPath = Get_RegistryFilePath();
    auto OutputDir = FPaths::GetPath(OutputPath);
    IFileManager::Get().MakeDirectory(*OutputDir, true);

    if (FFileHelper::SaveStringToFile(JsonContent, *OutputPath))
    {
        LastGeneratedHash = ComputeDefinitionsHash(Definitions);

        ck::angelscriptgenerator::Log(TEXT("[DynamicHandleSubsystem] Generated registry: {} ({} types)"),
            OutputPath, Definitions.Num());

        OnGenerationComplete.Broadcast(Definitions.Num(), true);
    }
    else
    {
        ck::angelscriptgenerator::Log(TEXT("[DynamicHandleSubsystem] Failed to write registry: {}"), OutputPath);
        OnGenerationComplete.Broadcast(0, false);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamicHandleSubsystem::
    IsRegenerationNeeded() const
    -> bool
{
    auto Definitions = DiscoverAllDefinitions();
    Definitions.Sort([](const UCkDynamic_HandleDefinition& A, const UCkDynamic_HandleDefinition& B)
    {
        return A.TypeName < B.TypeName;
    });

    const auto ExpectedJson = BuildJsonContent(Definitions);
    auto CurrentJson = FString{};
    if (NOT FFileHelper::LoadFileToString(CurrentJson, *Get_RegistryFilePath()))
    { return true; }

    return CurrentJson != ExpectedJson;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamicHandleSubsystem::
    Get_DiscoveredDefinitionCount() const
    -> int32
{
    return DiscoverAllDefinitions().Num();
}

auto
    UCkDynamicHandleSubsystem::
    ForceRefreshDynamicHandleBindings()
    -> int32
{
#if WITH_ANGELSCRIPT_CK
    auto TotalNewTypes = int32{ 0 };

    GenerateHandleTypeRegistry();

    FCkDynamic_HandleTypeRegistry::ResetJsonRegistryLoadedFlag();
    FCkAngelScript_HandleRegistry::ResetBindingsCompleteFlag();

    FCkDynamic_HandleTypeRegistry::LoadFromJsonRegistry();

    TotalNewTypes += FCkDynamic_HandleTypeRegistry::DiscoverAndRegisterNewDefinitionsIncremental();
    TotalNewTypes += FCkAngelScript_HandleRegistry::RegisterNewTypesIncremental();

    if (TotalNewTypes > 0)
    {
        ck::angelscriptgenerator::Log(
            TEXT("[DynamicHandleSubsystem] Registered {} new handle types at runtime"),
            TotalNewTypes);
    }
    else
    {
        ck::angelscriptgenerator::Log(
            TEXT("[DynamicHandleSubsystem] No new handle types to register"));
    }

    return TotalNewTypes;
#else
    return 0;
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamicHandleSubsystem::
    DiscoverAllDefinitions() const
    -> TArray<UCkDynamic_HandleDefinition*>
{
    auto Result = TArray<UCkDynamic_HandleDefinition*>{};

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry = AssetRegistryModule.Get();

    auto DefinitionAssets = TArray<FAssetData>{};
    AssetRegistry.GetAssetsByClass(UCkDynamic_HandleDefinition::StaticClass()->GetClassPathName(), DefinitionAssets);

    for (const auto& AssetData : DefinitionAssets)
    {
        if (auto* Definition = Cast<UCkDynamic_HandleDefinition>(AssetData.GetAsset()))
        {
            if (Definition->IsValidDefinition())
            {
                Result.Add(Definition);
            }
        }
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamicHandleSubsystem::
    BuildJsonContent(
        const TArray<UCkDynamic_HandleDefinition*>& InDefinitions) const
    -> FString
{
    auto RootObject = MakeShared<FJsonObject>();

    auto HandleTypesArray = TArray<TSharedPtr<FJsonValue>>{};

    for (const auto* Definition : InDefinitions)
    {
        if (ck::Is_NOT_Valid(Definition))
        {
            continue;
        }

        auto HandleTypeObject = MakeShared<FJsonObject>();

        HandleTypeObject->SetStringField(TEXT("TypeName"), Definition->TypeName);
        HandleTypeObject->SetStringField(TEXT("ShortName"), Definition->GetShortName());
        HandleTypeObject->SetStringField(TEXT("Description"), Definition->Description);
        HandleTypeObject->SetStringField(TEXT("SourceAsset"), Definition->GetPathName());

        auto FragmentsArray = TArray<TSharedPtr<FJsonValue>>{};
        for (const auto* Fragment : Definition->RequiredFragments)
        {
            if (Fragment != nullptr)
            {
                FragmentsArray.Add(MakeShared<FJsonValueString>(Fragment->GetName()));
            }
        }
        HandleTypeObject->SetArrayField(TEXT("RequiredFragments"), FragmentsArray);

        HandleTypesArray.Add(MakeShared<FJsonValueObject>(HandleTypeObject));
    }

    RootObject->SetArrayField(TEXT("HandleTypes"), HandleTypesArray);
    RootObject->SetNumberField(TEXT("Version"), 1);

    auto OutputString = FString{};
    auto Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject, Writer);

    return OutputString;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamicHandleSubsystem::
    ComputeDefinitionsHash(
        const TArray<UCkDynamic_HandleDefinition*>& InDefinitions) const
    -> uint32
{
    auto Hash = uint32{ 0 };

    for (const auto* Definition : InDefinitions)
    {
        if (ck::Is_NOT_Valid(Definition))
        {
            continue;
        }

        Hash = HashCombine(Hash, GetTypeHash(Definition->TypeName));
        Hash = HashCombine(Hash, GetTypeHash(Definition->GetShortName()));

        for (const auto* Fragment : Definition->RequiredFragments)
        {
            if (Fragment != nullptr)
            {
                Hash = HashCombine(Hash, GetTypeHash(Fragment->GetName()));
            }
        }
    }

    return Hash;
}

// --------------------------------------------------------------------------------------------------------------------
