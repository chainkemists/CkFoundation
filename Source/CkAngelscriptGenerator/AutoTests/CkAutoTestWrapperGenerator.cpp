#include "CkAutoTestWrapperGenerator.h"

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include <HAL/FileManager.h>
#include <Interfaces/IPluginManager.h>
#include <Misc/App.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <UObject/Class.h>
#include <UObject/UObjectIterator.h>

#if WITH_ANGELSCRIPT_CK
#include "ClassGenerator/ASClass.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck_autotest_wrapper_generator
{
#if WITH_EDITOR

    // ---- Constants -----------------------------------------------------

    // The AS base class authors subclass to define a test body. Defined in
    //   Plugins/CkTests/Script/Common/CkAutoTest_Base.as
    //
    // NOTE: UE stores UClass names *without* the type prefix (GetName() returns the bare
    // stem; GetPrefixCPP() yields "U"/"A"). AS-defined classes follow the same convention,
    // so source-form `UCk_AutoTest_Base` lives as `Ck_AutoTest_Base` in object storage.
    static const TCHAR* AutoTestBaseBareName = TEXT("Ck_AutoTest_Base");

    // The C++ AFunctionalTest subclass that hosts each test body. Source form (with `A`
    // prefix) is what we emit into generated .as files; storage form is `Ck_AutoTestRunner`.
    static const TCHAR* AutoTestRunnerSourceName = TEXT("ACk_AutoTestRunner");

    // Generated-file marker: any class whose source path contains this segment was emitted
    // by us, so it must not be treated as a "hand-authored collision" during the next pass.
    static const TCHAR* GeneratedDirSegment = TEXT("/Script/Generated/");

    // ---- Locating UCk_AutoTest_Base ------------------------------------

    auto Find_AutoTestBaseClass() -> UClass*
    {
        // AS-defined; not visible as a C++ symbol. Look up by storage name (no `U` prefix).
        // Try the bare name first; fall back to the prefixed form for safety in case some
        // future AS-UE revision changes the storage convention.
        if (auto* Found = FindFirstObject<UClass>(AutoTestBaseBareName, EFindFirstObjectOptions::None))
        { return Found; }
        return FindFirstObject<UClass>(TEXT("UCk_AutoTest_Base"), EFindFirstObjectOptions::None);
    }

    // ---- Filtering -----------------------------------------------------

    auto Is_IncludedAutoTestClass(UClass* InClass, UClass* InAutoTestBase) -> bool
    {
        if (ck::Is_NOT_Valid(InClass, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        if (InClass == InAutoTestBase)
        { return false; }

        if (NOT InClass->IsChildOf(InAutoTestBase))
        { return false; }

        constexpr auto DisqualifyingFlags =
            CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;

        if (InClass->HasAnyClassFlags(DisqualifyingFlags))
        { return false; }

        // Filter classes that have been torn down / are pending-kill. Without this, AS classes
        // that were deleted from source linger in TObjectIterator until GC runs and keep showing
        // up in the generated file even though the .as file no longer defines them.
        if (InClass->IsUnreachable() ||
            InClass->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
        { return false; }

#if WITH_ANGELSCRIPT_CK
        // For AS-defined classes, also check whether the source file still exists on disk.
        // When a user deletes a class from an .as file and the file still exists, the stale
        // UASClass hangs around; but if the whole file was deleted, that's a strong signal
        // the class is gone.
        if (auto* ASClass = UASClass::GetFirstASClass(InClass))
        {
            if (ASClass->NewerVersion != nullptr)
            { return false; }

            const auto SourcePath = ASClass->GetSourceFilePath();
            if (NOT SourcePath.IsEmpty() && NOT FPaths::FileExists(SourcePath))
            { return false; }
        }
#endif

        if (UCk_Utils_Reflection_UE::Is_PlaceholderClass(InClass))
        { return false; }

        return true;
    }

    // ---- Naming --------------------------------------------------------

    // The conventional source-form wrapper name: A<X>_Actor (e.g., "ACk_AutoTest_Foo_Actor").
    // Used when emitting AS source.
    auto Get_WrapperSourceName(const UClass* InEntityScriptClass) -> FString
    {
        return FString::Printf(TEXT("A%s_Actor"), *InEntityScriptClass->GetName());
    }

    // The storage-form wrapper name: <X>_Actor (no `A` prefix). Used for FindFirstObject
    // lookups since UE strips type prefixes from UClass storage names.
    auto Get_WrapperBareName(const UClass* InEntityScriptClass) -> FString
    {
        return FString::Printf(TEXT("%s_Actor"), *InEntityScriptClass->GetName());
    }

    // The source-form entity-script name with prefix preserved: U<X>. Used when emitting the
    // `default _TestEntityScriptClass = U<X>;` line — the AS compiler expects the prefixed form.
    auto Get_EntityScriptSourceName(const UClass* InEntityScriptClass) -> FString
    {
        return ck::Format_UE(TEXT("{}{}"),
            InEntityScriptClass->GetPrefixCPP(), InEntityScriptClass->GetName());
    }

    // ---- Hand-authored wrapper detection -------------------------------

    // Returns true if a class with the wrapper name already exists AND it was authored by the
    // user (i.e. its source path is not under Script/Generated/). Generated wrappers from a
    // previous pass are intentionally ignored so we don't deadlock ourselves. Stale/torn-down
    // UClasses from a just-removed AS class are also ignored.
    auto Has_HandAuthoredWrapper(const FString& InWrapperBareName) -> bool
    {
        auto* Existing = FindFirstObject<UClass>(*InWrapperBareName, EFindFirstObjectOptions::None);
        if (NOT ck::IsValid(Existing, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        // Filter stale classes — a wrapper class the user just deleted from .as source can
        // linger in TObjectIterator/FindFirstObject until GC. Without this, removing a hand-
        // authored wrapper would never trigger generator emission because the stale UClass
        // keeps tripping the collision check. Mirrors the staleness checks in
        // CkAngelscriptEntityScriptParamsGenerator's Is_IncludedEntityScriptClass.
        if (Existing->IsUnreachable() ||
            Existing->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
        { return false; }

        constexpr auto DisqualifyingFlags = CLASS_NewerVersionExists | CLASS_Deprecated;
        if (Existing->HasAnyClassFlags(DisqualifyingFlags))
        { return false; }

#if WITH_ANGELSCRIPT_CK
        if (auto* ExistingAS = UASClass::GetFirstASClass(Existing))
        {
            // The class has been replaced by a recompile; its slot is dead.
            if (ExistingAS->NewerVersion != nullptr)
            { return false; }

            const auto SourcePath = ExistingAS->GetSourceFilePath();
            // Source file deleted entirely — class is gone, just lingering in memory.
            if (NOT SourcePath.IsEmpty() && NOT FPaths::FileExists(SourcePath))
            { return false; }

            // No source path means this isn't an AS class we can reason about;
            // treat as hand-authored to be safe.
            if (SourcePath.IsEmpty())
            { return true; }

            auto Normalized = FPaths::ConvertRelativePathToFull(SourcePath);
            FPaths::NormalizeFilename(Normalized);
            return NOT Normalized.Contains(GeneratedDirSegment, ESearchCase::IgnoreCase);
        }
#endif

        // C++ class with the wrapper name — definitely hand-authored.
        return true;
    }

    // ---- Block formatting ----------------------------------------------

    auto Format_WrapperBlock(const UClass* InEntityScriptClass) -> FString
    {
        const auto WrapperSourceName = Get_WrapperSourceName(InEntityScriptClass);
        const auto EntityScriptSourceName = Get_EntityScriptSourceName(InEntityScriptClass);

        auto Block = FString{};
        Block += ck::Format_UE(TEXT("class {} : {}\n"), WrapperSourceName, AutoTestRunnerSourceName);
        Block += TEXT("{\n");
        Block += ck::Format_UE(TEXT("    default _TestEntityScriptClass = {};\n"), EntityScriptSourceName);
        Block += TEXT("}\n\n");
        return Block;
    }

    // ---- Plugin bucketing ----------------------------------------------

    struct FPluginBucket
    {
        FString PluginName;
        FString OutputFilePath;
        TArray<UClass*> Classes;
    };

    auto Build_ModuleToPluginMap() -> TMap<FString, TSharedRef<IPlugin>>
    {
        auto Result = TMap<FString, TSharedRef<IPlugin>>{};
        for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
        {
            for (const auto& Module : Plugin->GetDescriptor().Modules)
            {
                Result.Add(Module.Name.ToString(), Plugin);
            }
        }
        return Result;
    }

    auto Find_PluginByPathPrefix(const FString& InPath) -> const IPlugin*
    {
        if (InPath.IsEmpty())
        { return nullptr; }

        auto NormalizedPath = FPaths::ConvertRelativePathToFull(InPath);
        FPaths::NormalizeFilename(NormalizedPath);

        const IPlugin* BestMatch = nullptr;
        auto BestMatchLen = int32{0};

        for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
        {
            auto PluginDir = FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir());
            FPaths::NormalizeDirectoryName(PluginDir);

            if (NormalizedPath.StartsWith(PluginDir, ESearchCase::IgnoreCase))
            {
                if (PluginDir.Len() > BestMatchLen)
                {
                    BestMatch = &Plugin.Get();
                    BestMatchLen = PluginDir.Len();
                }
            }
        }
        return BestMatch;
    }

    auto Find_PluginForClass(
        const UClass* InClass,
        const TMap<FString, TSharedRef<IPlugin>>& InModuleToPlugin)
        -> const IPlugin*
    {
        const auto PackageName = InClass->GetOutermost()->GetName();
        constexpr auto ScriptPrefix = TEXT("/Script/");
        if (PackageName.StartsWith(ScriptPrefix))
        {
            const auto ModuleName = PackageName.RightChop(FString(ScriptPrefix).Len());
            if (const auto* Found = InModuleToPlugin.Find(ModuleName))
            { return &Found->Get(); }
        }

#if WITH_ANGELSCRIPT_CK
        if (auto* ASClass = UASClass::GetFirstASClass(const_cast<UClass*>(InClass)))
        {
            if (const auto* Plugin = Find_PluginByPathPrefix(ASClass->GetSourceFilePath()))
            { return Plugin; }
        }
#endif

        return nullptr;
    }

    auto Bucket_ClassesByPlugin(const TArray<UClass*>& InClasses) -> TArray<FPluginBucket>
    {
        const auto ModuleToPlugin = Build_ModuleToPluginMap();

        auto BucketMap = TMap<FString, FPluginBucket>{};

        for (auto* Class : InClasses)
        {
            auto PluginName = FString{};
            auto OutputDir = FString{};

            if (const auto* Plugin = Find_PluginForClass(Class, ModuleToPlugin))
            {
                PluginName = Plugin->GetName();
                OutputDir = Plugin->GetBaseDir() / TEXT("Script") / TEXT("Generated");
            }
            else
            {
                PluginName = FApp::GetProjectName();
                OutputDir = FPaths::ProjectDir() / TEXT("Script") / TEXT("Generated");
            }

            if (NOT BucketMap.Contains(PluginName))
            {
                auto Bucket = FPluginBucket{};
                Bucket.PluginName = PluginName;
                Bucket.OutputFilePath = OutputDir / (PluginName + TEXT("_AutoTestActors.as"));
                BucketMap.Add(PluginName, MoveTemp(Bucket));
            }

            BucketMap[PluginName].Classes.Add(Class);
        }

        auto Result = TArray<FPluginBucket>{};
        BucketMap.GenerateValueArray(Result);
        Result.Sort([](const FPluginBucket& A, const FPluginBucket& B)
        {
            return A.PluginName < B.PluginName;
        });
        return Result;
    }

    // ---- File content --------------------------------------------------

    static const TCHAR* FileHeader =
        TEXT("// Auto-generated AutoTest actor wrappers — DO NOT EDIT.\n")
        TEXT("// This file is regenerated on editor startup and after every AngelScript recompile.\n")
        TEXT("//\n")
        TEXT("// For each UCk_AutoTest_Base subclass discovered, one wrapper class is emitted:\n")
        TEXT("//   class A<TestName>_Actor : ACk_AutoTestRunner\n")
        TEXT("//   {\n")
        TEXT("//       default _TestEntityScriptClass = U<TestName>;\n")
        TEXT("//   }\n")
        TEXT("//\n")
        TEXT("// To opt out for a specific test, hand-author your own A<TestName>_Actor class\n")
        TEXT("// (anywhere outside Script/Generated/). The generator detects the existing class\n")
        TEXT("// by source path and skips emission, leaving your wrapper authoritative.\n\n");

#endif // WITH_EDITOR
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAutoTestWrapperGenerator::
    GenerateAll()
    -> void
{
#if WITH_EDITOR

    ck::angelscriptgenerator::Log(TEXT("[CkAS AutoTest Wrappers] === Generating AutoTest actor wrappers ==="));

    auto* AutoTestBase = ck_autotest_wrapper_generator::Find_AutoTestBaseClass();
    if (NOT ck::IsValid(AutoTestBase, ck::IsValid_Policy_NullptrOnly{}))
    {
        // CkTests not loaded, or AS not yet compiled — nothing to do. This is the normal state
        // during very early startup before AS classes have been registered. Promoted to Log
        // (not VeryVerbose) so the user can diagnose the silent-no-write case at Trace level.
        ck::angelscriptgenerator::Log(
            TEXT("[CkAS AutoTest Wrappers] Ck_AutoTest_Base not found in object table — ")
            TEXT("skipping pass (CkTests not loaded yet, or AS not yet compiled)."));
        return;
    }

    auto AllSubclasses = TArray<UClass*>{};
    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        auto* Class = *ClassIterator;
        if (ck_autotest_wrapper_generator::Is_IncludedAutoTestClass(Class, AutoTestBase))
        {
            AllSubclasses.Add(Class);
        }
    }

    AllSubclasses.Sort([](const UClass& InA, const UClass& InB)
    {
        return InA.GetPathName() < InB.GetPathName();
    });

    // Drop tests that already have a hand-authored wrapper of the conventional name.
    auto Emittable = TArray<UClass*>{};
    auto SkippedCount = int32{0};
    for (auto* Class : AllSubclasses)
    {
        const auto WrapperBareName = ck_autotest_wrapper_generator::Get_WrapperBareName(Class);
        if (ck_autotest_wrapper_generator::Has_HandAuthoredWrapper(WrapperBareName))
        {
            ++SkippedCount;
            ck::angelscriptgenerator::VeryVerbose(
                TEXT("[CkAS AutoTest Wrappers] Skipping {} — hand-authored {} already exists."),
                Class->GetName(),
                ck_autotest_wrapper_generator::Get_WrapperSourceName(Class));
            continue;
        }
        Emittable.Add(Class);
    }

    ck::angelscriptgenerator::Log(
        TEXT("[CkAS AutoTest Wrappers] Discovered {} subclasses of Ck_AutoTest_Base — ")
        TEXT("{} will be emitted, {} have hand-authored wrappers (skipped)."),
        AllSubclasses.Num(), Emittable.Num(), SkippedCount);

    const auto Buckets = ck_autotest_wrapper_generator::Bucket_ClassesByPlugin(Emittable);

    auto TotalEmitted = int32{0};

    for (const auto& Bucket : Buckets)
    {
        auto Content = FString{ck_autotest_wrapper_generator::FileHeader};

        for (auto* Class : Bucket.Classes)
        {
            Content += ck_autotest_wrapper_generator::Format_WrapperBlock(Class);
        }

        const auto OutputDir = FPaths::GetPath(Bucket.OutputFilePath);
        IFileManager::Get().MakeDirectory(*OutputDir, true);

        // Skip writing when the content hasn't changed — otherwise our own write triggers the
        // AngelScript PostCompile hook, which calls us again, ad infinitum.
        // Compare in LF-normalized form: Content is LF here (literals below), but the on-disk
        // file may be CRLF on Windows, and LoadFileToString normalizes to LF.
        auto ExistingContent = FString{};
        const auto HasExisting = FFileHelper::LoadFileToString(ExistingContent, *Bucket.OutputFilePath);

        auto ContentForCompare = Content;
        ContentForCompare.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
        auto ExistingForCompare = ExistingContent;
        ExistingForCompare.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

        if (HasExisting && ExistingForCompare.Equals(ContentForCompare, ESearchCase::CaseSensitive))
        {
            ck::angelscriptgenerator::VeryVerbose(
                TEXT("[CkAS AutoTest Wrappers] [{}] up-to-date ({} wrappers)"),
                Bucket.PluginName, Bucket.Classes.Num());
            TotalEmitted += Bucket.Classes.Num();
            continue;
        }

#if PLATFORM_WINDOWS
        // Match project convention: .as files live as CRLF on Windows disk (checkout converts
        // index-LF -> WC-CRLF via core.autocrlf). Without this step, the generator writes LF,
        // disagrees with every other .as file on disk, and git flags the file as modified on
        // every editor startup even though `git diff` is empty.
        Content.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
        Content.ReplaceInline(TEXT("\n"), TEXT("\r\n"));
#endif

        if (FFileHelper::SaveStringToFile(Content, *Bucket.OutputFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {
            ck::angelscriptgenerator::Log(
                TEXT("[CkAS AutoTest Wrappers] [{}] {} wrappers -> {}"),
                Bucket.PluginName, Bucket.Classes.Num(), Bucket.OutputFilePath);
        }
        else
        {
            ck::angelscriptgenerator::Error(
                TEXT("[CkAS AutoTest Wrappers] Failed to write to [{}]"),
                Bucket.OutputFilePath);
        }

        TotalEmitted += Bucket.Classes.Num();
    }

    ck::angelscriptgenerator::Log(
        TEXT("[CkAS AutoTest Wrappers] Done — {} plugins, {} wrappers emitted, {} skipped (hand-authored)"),
        Buckets.Num(), TotalEmitted, SkippedCount);

#endif // WITH_EDITOR
}

// --------------------------------------------------------------------------------------------------------------------
