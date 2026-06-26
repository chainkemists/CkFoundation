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
// Utilities
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

auto
    FCkDynamic_HandleTypeRegistry::
    FindScriptStructByName(
        const FString& InStructName)
    -> const UScriptStruct*
{
    auto* FoundStruct = FindObject<UScriptStruct>(nullptr, *InStructName);
    if (FoundStruct != nullptr)
    {
        return FoundStruct;
    }

    const auto AngelscriptType = FAngelscriptType::GetByAngelscriptTypeName(InStructName);
    if (AngelscriptType.IsValid())
    {
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
        if (NOT ck::IsValid(InHandle))
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
// JSON Registry Loading
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_HandleTypeRegistry::
    LoadFromJsonRegistry()
    -> bool
{
    if (_JsonRegistryLoaded)
    { return true; }

    const auto FilePath = GetRegistryFilePath();

    auto JsonString = FString{};
    if (NOT FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        ck::dynamic::Log(TEXT("[DynamicHandleTypes] No registry file found at: %s"), *FilePath);
        return false;
    }

    auto JsonReader = TJsonReaderFactory<>::Create(JsonString);
    auto RootObject = TSharedPtr<FJsonObject>{};

    if (NOT FJsonSerializer::Deserialize(JsonReader, RootObject) || NOT RootObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[DynamicHandleTypes] Failed to parse registry file: %s"), *FilePath);
        return false;
    }

    auto HandleTypesArray = RootObject->GetArrayField(TEXT("HandleTypes"));

    // Self-heal sibling stub merge — `_StubRecovery_DynamicHandleTypes.json` in
    // the same directory as the canonical registry. The dispatcher writes
    // synthesized stub entries there at AS-failure modal-tick time so the
    // canonical file stays byte-clean from HEAD. We merge the stub's
    // `HandleTypes` array into the canonical load here, deduping by
    // `TypeName` with stub entries winning (they only exist because the
    // canonical lacks them). The stub file is deleted by the PostCompile
    // hook after a successful AS compile.
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
                        TEXT("[DynamicHandleTypes] Merged %d stub entry/entries from sibling recovery file: %s"),
                        MergedCount, *StubFilePath);
                }
            }
        }
    }

    // Test-only handle registries. Any enabled plugin (or the project) may drop a
    // `Script/Generated/DynamicHandleTypes.TESTONLY.json` to register `FCk_Handle_TESTONLY_*`
    // types for automation tests WITHOUT polluting the committed canonical registry. Discovered
    // by directory iteration (thread-safe, no IPluginManager dependency so this is safe on the
    // packaged off-thread PreCompile path), merged with the same dedup-by-TypeName rule — canonical
    // and stub entries always win, so a test registry can never override a real handle type.
    //
    // SHIPPING/TEST GATE: this merge is strictly "for automation tests". Test plugins (CkTests, etc.)
    // are disabled in Shipping/Test client builds, so their `FCk_Handle_TESTONLY_*` types have NO C++
    // backing — registering them yields asINVALID_TYPE and cascades through every handle mixin
    // (StoreVar_*, GetVar_*, ...) registered onto the dead types. The staged `.TESTONLY.json` is
    // harmless as data; it must simply never be merged outside dev/editor builds. Condition mirrors
    // the engine's WITH_AS_DEBUGSERVER gate (!UE_BUILD_SHIPPING && !UE_BUILD_TEST).
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
                    TEXT("[DynamicHandleTypes] Merged %d TEST-ONLY handle type(s) from: %s"),
                    MergedCount, *TestFilePath);
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

    ck::dynamic::Log(TEXT("[DynamicHandleTypes] Loaded %d handle types from registry"), RegisteredCount);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Registration
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

    // Register-or-update: if a previous registration of this type exists
    // (e.g. a stub written by the self-heal dispatcher, or a stale entry
    // whose RequiredFragments have since changed on the underlying data
    // asset), replace the in-memory validator + metadata in place rather
    // than silently no-op. This fixes the long-standing
    // ForceRefreshDynamicHandleBindings-doesn't-update bug — the AS-side
    // registry's append-only RegisterHandleType returned false for
    // existing entries, so the button wrote a fresh JSON but in-memory
    // bindings stayed stale until editor restart.
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
// Queries
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
// Initialization
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
        LoadFromJsonRegistry();

        // AS pre-compile runs on a worker thread in packaged (non-editor) builds. The asset-registry
        // discovery below enumerates in-memory assets, which asserts off the game thread
        // (AssetRegistry's EnumerateMemoryAssetsHelper requires the game or loading thread). The JSON
        // registry loaded above is the compile-time source of truth for handle types; asset-defined
        // UCkDynamic_HandleDefinition discovery is an editor-time convenience, so skip it off-thread.
        if (IsInGameThread())
        {
            DiscoverAndRegisterAllDefinitions();
        }

        FCkAngelScript_HandleRegistry::EnsureAllBindingsComplete();
    });

    CallbackRegistered = true;
}

// --------------------------------------------------------------------------------------------------------------------
// Static Initialization
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