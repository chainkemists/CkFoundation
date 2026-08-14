#include "CkDynamic_AngelScript.h"

#if WITH_ANGELSCRIPT_CK

#include "CkCore/Ensure/CkEnsure_Utils.h"
#include "CkCore/Format/CkFormat.h"

#include "CkDynamic/CkDynamic_Utils.h"
#include "CkDynamic/CkDynamic_HandleDefinition.h"
#include "CkDynamic/Settings/CkDynamic_Settings.h"
#include "CkDynamic/CkDynamic_Log.h"

#include <AngelscriptManager.h>
#include <AngelscriptType.h>
#include "AngelscriptCodeModule.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    GetRegistryFilePath()
    -> FString
{
    return UCk_Utils_Dynamic_Settings_UE::Get_DynamicHandleRegistryFilePath();
}

auto
    FCkDynamic_HandleTypeRegistry::
    ExtractShortNameFromTypeName(
        const FString& InTypeName)
    -> FString
{
    static const TArray<FString> KnownPrefixes =
    {
        TEXT("FCk_Handle_"),
        TEXT("Handle_"),
    };

    for (const auto& Prefix : KnownPrefixes)
    {
        if (InTypeName.StartsWith(Prefix))
        {
            return InTypeName.RightChop(Prefix.Len());
        }
    }

    return InTypeName;
}

namespace ck_dynamic_angelscript
{
    // Weak, not raw: a file-static map is invisible to the GC, so a collected struct must go STALE
    // rather than dangle. Also cleared on AS pre-compile — see InvalidateScriptStructCache.
    static TMap<FString, TWeakObjectPtr<const UScriptStruct>> ResolvedScriptStructCache;
}

auto
    FCkDynamic_HandleTypeRegistry::
    ResolveScriptStructByName(
        const FString& InStructName)
    -> const UScriptStruct*
{
    auto* FoundStruct = FindObject<UScriptStruct>(nullptr, *InStructName);
    if (FoundStruct != nullptr)
    {
        return FoundStruct;
    }

    // The registry stores the F-STRIPPED UScriptStruct name while the AngelScript type database is keyed on
    // the F-PREFIXED script name, so the bare lookup misses by construction for every AS-declared fragment.
    const FString CandidateNames[] = { InStructName, FString{TEXT("F")} + InStructName };
    for (const auto& CandidateName : CandidateNames)
    {
        const auto AngelscriptType = FAngelscriptType::GetByAngelscriptTypeName(CandidateName);
        if (NOT AngelscriptType.IsValid())
        {
            continue;
        }

        const auto TypeUsage = FAngelscriptTypeUsage(AngelscriptType);
        const auto* UnrealStruct = TypeUsage.GetUnrealStruct();
        if (UnrealStruct != nullptr)
        {
            return Cast<UScriptStruct>(UnrealStruct);
        }
    }

    for (TObjectIterator<UScriptStruct> It; It; ++It)
    {
        if (It->GetName() == InStructName)
        {
            return *It;
        }
    }

    return nullptr;
}

auto
    FCkDynamic_HandleTypeRegistry::
    FindScriptStructByName(
        const FString& InStructName)
    -> const UScriptStruct*
{
    if (const auto* const CachedStruct = ck_dynamic_angelscript::ResolvedScriptStructCache.Find(InStructName))
    {
        if (const auto* const ResolvedStruct = CachedStruct->Get())
        {
            return ResolvedStruct;
        }

        ck_dynamic_angelscript::ResolvedScriptStructCache.Remove(InStructName);
    }

    const auto* const StructType = ResolveScriptStructByName(InStructName);
    if (StructType != nullptr)
    {
        ck_dynamic_angelscript::ResolvedScriptStructCache.Add(InStructName, StructType);
    }

    return StructType;
}

auto
    FCkDynamic_HandleTypeRegistry::
    InvalidateScriptStructCache()
    -> void
{
    ck_dynamic_angelscript::ResolvedScriptStructCache.Reset();
}

auto
    FCkDynamic_HandleTypeRegistry::
    CreateMultiFragmentValidator(
        const TArray<FString>& InFragmentNames)
    -> TFunction<bool(const FCk_Handle&)>
{
    if (InFragmentNames.IsEmpty())
    {
        return [](const FCk_Handle& InHandle) -> bool
        {
            return ck::IsValid(InHandle);
        };
    }

    return [FragmentNames = InFragmentNames](const FCk_Handle& InHandle) -> bool
    {
        if (ck::Is_NOT_Valid(InHandle))
        {
            return false;
        }

        for (const auto& FragmentName : FragmentNames)
        {
            const auto* StructType = FCkDynamic_HandleTypeRegistry::FindScriptStructByName(FragmentName);
            if (StructType == nullptr)
            {
                return false;
            }

            if (NOT UCk_Utils_DynamicFragment_UE::Has_Fragment(InHandle, StructType))
            {
                return false;
            }
        }

        return true;
    };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    LoadFromJsonRegistry()
    -> bool
{
    if (_JsonRegistryLoaded)
    { return true; }

    const auto FilePath = GetRegistryFilePath();

    // A missing/unparsable canonical registry must NOT abort the load — the merges below are independent sources.
    auto HandleTypesArray = TArray<TSharedPtr<FJsonValue>>{};

    if (auto JsonString = FString{};
        FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        auto JsonReader = TJsonReaderFactory<>::Create(JsonString);
        auto RootObject = TSharedPtr<FJsonObject>{};

        if (FJsonSerializer::Deserialize(JsonReader, RootObject) && RootObject.IsValid())
        {
            HandleTypesArray = RootObject->GetArrayField(TEXT("HandleTypes"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[DynamicHandleTypes] Failed to parse registry file: %s — continuing with stub/test-only registries only"), *FilePath);
        }
    }
    else
    {
        ck::dynamic::Log(TEXT("[DynamicHandleTypes] No registry file found at: {} — continuing with stub/test-only registries only"), FilePath);
    }

    {
        const auto StubFilePath = FPaths::GetPath(FilePath) /
            (FString{TEXT("_StubRecovery_")} + FPaths::GetCleanFilename(FilePath));

        auto StubJsonString = FString{};
        if (FFileHelper::LoadFileToString(StubJsonString, *StubFilePath))
        {
            auto StubReader = TJsonReaderFactory<>::Create(StubJsonString);
            auto StubRoot   = TSharedPtr<FJsonObject>{};
            if (FJsonSerializer::Deserialize(StubReader, StubRoot) && StubRoot.IsValid()
                && StubRoot->HasField(TEXT("HandleTypes")))
            {
                const auto StubEntries = StubRoot->GetArrayField(TEXT("HandleTypes"));

                auto CanonicalTypeNames = TSet<FString>{};
                for (const auto& Entry : HandleTypesArray)
                {
                    const auto Obj = Entry->AsObject();
                    if (Obj.IsValid())
                    {
                        auto Name = FString{};
                        Obj->TryGetStringField(TEXT("TypeName"), Name);
                        if (NOT Name.IsEmpty())
                        { CanonicalTypeNames.Add(Name); }
                    }
                }

                auto MergedCount = 0;
                for (const auto& StubEntry : StubEntries)
                {
                    const auto Obj = StubEntry->AsObject();
                    if (NOT Obj.IsValid())
                    { continue; }

                    auto Name = FString{};
                    Obj->TryGetStringField(TEXT("TypeName"), Name);
                    if (Name.IsEmpty() || CanonicalTypeNames.Contains(Name))
                    { continue; }

                    HandleTypesArray.Add(StubEntry);
                    ++MergedCount;
                }

                if (MergedCount > 0)
                {
                    ck::dynamic::Log(
                        TEXT("[DynamicHandleTypes] Merged {} stub entry/entries from sibling recovery file: {}"),
                        MergedCount, StubFilePath);
                }
            }
        }
    }

    // The gate is load-bearing: test plugins are disabled in Shipping/Test, so their TESTONLY types have no
    // C++ backing and registering them yields asINVALID_TYPE across every handle mixin.
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
    {
        auto TestRegistryFiles = TArray<FString>{};

        const auto CollectIfPresent = [&TestRegistryFiles](const FString& InBaseDir) -> void
        {
            const auto Candidate = InBaseDir / TEXT("Script") / TEXT("Generated") / TEXT("DynamicHandleTypes.TESTONLY.json");
            if (FPaths::FileExists(Candidate))
            { TestRegistryFiles.AddUnique(FPaths::ConvertRelativePathToFull(Candidate)); }
        };

        CollectIfPresent(FPaths::ProjectDir());

        const auto PluginsRoot = FPaths::ProjectPluginsDir();
        auto PluginDirNames = TArray<FString>{};
        constexpr auto FindFiles = false;
        constexpr auto FindDirectories = true;
        IFileManager::Get().FindFiles(PluginDirNames, *(PluginsRoot / TEXT("*")), FindFiles, FindDirectories);
        for (const auto& PluginDirName : PluginDirNames)
        {
            CollectIfPresent(PluginsRoot / PluginDirName);
        }

        for (const auto& TestFilePath : TestRegistryFiles)
        {
            auto TestJsonString = FString{};
            if (NOT FFileHelper::LoadFileToString(TestJsonString, *TestFilePath))
            { continue; }

            auto TestReader = TJsonReaderFactory<>::Create(TestJsonString);
            auto TestRoot   = TSharedPtr<FJsonObject>{};
            if (NOT (FJsonSerializer::Deserialize(TestReader, TestRoot) && TestRoot.IsValid()
                && TestRoot->HasField(TEXT("HandleTypes"))))
            { continue; }

            const auto TestEntries = TestRoot->GetArrayField(TEXT("HandleTypes"));

            auto ExistingTypeNames = TSet<FString>{};
            for (const auto& Entry : HandleTypesArray)
            {
                const auto Obj = Entry->AsObject();
                if (Obj.IsValid())
                {
                    auto Name = FString{};
                    Obj->TryGetStringField(TEXT("TypeName"), Name);
                    if (NOT Name.IsEmpty())
                    { ExistingTypeNames.Add(Name); }
                }
            }

            auto MergedCount = 0;
            for (const auto& TestEntry : TestEntries)
            {
                const auto Obj = TestEntry->AsObject();
                if (NOT Obj.IsValid())
                { continue; }

                auto Name = FString{};
                Obj->TryGetStringField(TEXT("TypeName"), Name);
                if (Name.IsEmpty() || ExistingTypeNames.Contains(Name))
                { continue; }

                HandleTypesArray.Add(TestEntry);
                ExistingTypeNames.Add(Name);
                ++MergedCount;
            }

            if (MergedCount > 0)
            {
                ck::dynamic::Log(
                    TEXT("[DynamicHandleTypes] Merged {} TEST-ONLY handle type(s) from: {}"),
                    MergedCount, TestFilePath);
            }
        }
    }
#endif // !UE_BUILD_SHIPPING && !UE_BUILD_TEST

    if (HandleTypesArray.Num() == 0)
    {
        ck::dynamic::Log(TEXT("[DynamicHandleTypes] Registry file is empty"));
        _JsonRegistryLoaded = true;
        return true;
    }

    auto RegisteredCount = 0;

    for (const auto& HandleTypeValue : HandleTypesArray)
    {
        const auto HandleTypeObject = HandleTypeValue->AsObject();
        if (HandleTypeObject == nullptr)
        { continue; }

        auto TypeName = FString{};
        HandleTypeObject->TryGetStringField(TEXT("TypeName"), TypeName);

        if (TypeName.IsEmpty())
        { continue; }

        auto ShortName = FString{};
        HandleTypeObject->TryGetStringField(TEXT("ShortName"), ShortName);

        if (ShortName.IsEmpty())
        {
            ShortName = ExtractShortNameFromTypeName(TypeName);
        }

        auto RequiredFragments = TArray<FString>{};
        const auto FragmentsArray = HandleTypeObject->GetArrayField(TEXT("RequiredFragments"));
        for (const auto& FragmentValue : FragmentsArray)
        {
            auto FragmentName = FString{};
            if (FragmentValue->TryGetString(FragmentName))
            {
                RequiredFragments.Add(FragmentName);
            }
        }

        auto Description = FString{};
        HandleTypeObject->TryGetStringField(TEXT("Description"), Description);

        auto SourceAsset = FString{};
        HandleTypeObject->TryGetStringField(TEXT("SourceAsset"), SourceAsset);

        if (RegisterHandleType(TypeName, ShortName, RequiredFragments, Description, SourceAsset))
        {
            RegisteredCount++;
        }
    }

    _JsonRegistryLoaded = true;

    ck::dynamic::Log(TEXT("[DynamicHandleTypes] Loaded {} handle types from registry"), RegisteredCount);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    RegisterHandleType(
        const FString& InTypeName,
        const FString& InShortName,
        const TArray<FString>& InRequiredFragments,
        const FString& InDescription,
        const FString& InSourceAsset)
    -> bool
{
    if (InTypeName.IsEmpty())
    { return false; }

    auto ResolvedShortName = InShortName;
    if (ResolvedShortName.IsEmpty())
    {
        ResolvedShortName = ExtractShortNameFromTypeName(InTypeName);
    }

    auto Validator = CreateMultiFragmentValidator(InRequiredFragments);

    if (FCkAngelScript_HandleRegistry::IsHandleTypeRegistered(InTypeName))
    {
        return FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle(
            InTypeName,
            MoveTemp(Validator),
            InRequiredFragments,
            InDescription,
            InSourceAsset);
    }

    return FCkAngelScript_HandleRegistry::RegisterDynamicHandle(
        InTypeName,
        ResolvedShortName,
        MoveTemp(Validator),
        InRequiredFragments,
        InDescription,
        InSourceAsset);
}

auto
    FCkDynamic_HandleTypeRegistry::
    RegisterHandleTypeFromDefinition(
        const UCkDynamic_HandleDefinition* InDefinition)
    -> bool
{
    if (ck::Is_NOT_Valid(InDefinition))
    {
        return false;
    }

    if (InDefinition->TypeName.IsEmpty())
    {
        return false;
    }

    return RegisterHandleType(
        InDefinition->TypeName,
        InDefinition->GetShortName(),
        InDefinition->GetRequiredFragmentNames(),
        InDefinition->Description,
        InDefinition->GetPathName());
}

auto
    FCkDynamic_HandleTypeRegistry::
    DiscoverAndRegisterAllDefinitions()
    -> void
{
    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry = AssetRegistryModule.Get();

    auto DefinitionAssets = TArray<FAssetData>{};
    AssetRegistry.GetAssetsByClass(UCkDynamic_HandleDefinition::StaticClass()->GetClassPathName(), DefinitionAssets);

    for (const auto& AssetData : DefinitionAssets)
    {
        if (auto* Definition = Cast<UCkDynamic_HandleDefinition>(AssetData.GetAsset()))
        {
            RegisterHandleTypeFromDefinition(Definition);
        }
    }
}

auto
    FCkDynamic_HandleTypeRegistry::
    DiscoverAndRegisterNewDefinitionsIncremental()
    -> int32
{
    auto NewTypeCount = int32{ 0 };

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry = AssetRegistryModule.Get();

    auto DefinitionAssets = TArray<FAssetData>{};
    AssetRegistry.GetAssetsByClass(UCkDynamic_HandleDefinition::StaticClass()->GetClassPathName(), DefinitionAssets);

    for (const auto& AssetData : DefinitionAssets)
    {
        if (auto* Definition = Cast<UCkDynamic_HandleDefinition>(AssetData.GetAsset()))
        {
            if (NOT Definition->IsValidDefinition())
            { continue; }

            if (IsHandleTypeRegistered(Definition->TypeName))
            { continue; }

            if (RegisterHandleTypeFromDefinition(Definition))
            {
                NewTypeCount++;
            }
        }
    }

    return NewTypeCount;
}

auto
    FCkDynamic_HandleTypeRegistry::
    ResetJsonRegistryLoadedFlag()
    -> void
{
    _JsonRegistryLoaded = false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    IsHandleTypeRegistered(
        const FString& InTypeName)
    -> bool
{
    return FCkAngelScript_HandleRegistry::IsHandleTypeRegistered(InTypeName);
}

auto
    FCkDynamic_HandleTypeRegistry::
    GetHandleTypeInfo(
        const FString& InTypeName)
    -> const FCkAngelScript_HandleTypeInfo*
{
    return FCkAngelScript_HandleRegistry::GetHandleTypeInfo(InTypeName);
}

auto
    FCkDynamic_HandleTypeRegistry::
    GetAllRegisteredTypes()
    -> const TMap<FString, TSharedPtr<FCkAngelScript_HandleTypeInfo>>&
{
    return FCkAngelScript_HandleRegistry::GetAllRegisteredTypes();
}

auto
    FCkDynamic_HandleTypeRegistry::
    ValidateHandle(
        const FString& InTypeName,
        const FCk_Handle& InHandle)
    -> bool
{
    const auto* TypeInfo = GetHandleTypeInfo(InTypeName);
    if (TypeInfo == nullptr)
    {
        return false;
    }

    if (NOT TypeInfo->IsValidAsType)
    {
        return ck::IsValid(InHandle);
    }

    return TypeInfo->IsValidAsType(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    EnsureCallbackRegistered()
    -> void
{
    static auto CallbackRegistered = false;
    if (CallbackRegistered)
    {
        return;
    }

    _PreCompileDelegateHandle = FAngelscriptCodeModule::GetPreCompile().AddStatic([]
    {
        // A hot reload replaces the AngelScript-declared UScriptStructs the cache points at, and the
        // outgoing ones can outlive the swap — drop the memo before anything re-resolves.
        InvalidateScriptStructCache();

        LoadFromJsonRegistry();

        // AS pre-compile runs on a worker thread in packaged builds, and asset-registry discovery asserts off
        // the game thread. Asset-defined definitions are an editor-time convenience; the JSON registry above
        // is the compile-time source of truth.
        if (IsInGameThread())
        {
            DiscoverAndRegisterAllDefinitions();
        }

        FCkAngelScript_HandleRegistry::EnsureAllBindingsComplete();
    });

    CallbackRegistered = true;
}

// --------------------------------------------------------------------------------------------------------------------

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_DynamicHandleTypes_Init(
    FAngelscriptBinds::EOrder::Late, []
{
    FCkDynamic_HandleTypeRegistry::EnsureCallbackRegistered();
});

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_RegisterHandleType_Global(
    FAngelscriptBinds::EOrder::Late, []
{
    FAngelscriptBinds::BindGlobalFunction(
        "bool Ck_RegisterHandleType(const FString& in InTypeName)",
        [](const FString& InTypeName) -> bool
        {
            const auto ShortName = FCkDynamic_HandleTypeRegistry::ExtractShortNameFromTypeName(InTypeName);
            return FCkDynamic_HandleTypeRegistry::RegisterHandleType(InTypeName, ShortName);
        });

    FAngelscriptBinds::BindGlobalFunction(
        "bool Ck_RegisterHandleTypeWithFragment(const FString& in InTypeName, const FString& in InValidatorFragment)",
        [](const FString& InTypeName, const FString& InValidatorFragment) -> bool
        {
            const auto ShortName = FCkDynamic_HandleTypeRegistry::ExtractShortNameFromTypeName(InTypeName);
            return FCkDynamic_HandleTypeRegistry::RegisterHandleType(
                InTypeName,
                ShortName,
                TArray<FString>{ InValidatorFragment });
        });

    FAngelscriptBinds::BindGlobalFunction(
        "bool Ck_IsHandleTypeRegistered(const FString& in InTypeName)",
        [](const FString& InTypeName) -> bool
        {
            return FCkDynamic_HandleTypeRegistry::IsHandleTypeRegistered(InTypeName);
        });
});

#endif // WITH_ANGELSCRIPT_CK