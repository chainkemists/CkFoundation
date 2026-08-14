#include "CkAutoTestWrapperGenerator.h"

#include "CkAngelscriptGenerator/AutoTests/CkAutoTestNetStubGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"

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

    // UE object storage drops the type prefix, and AS-defined classes follow suit — source-form
    // `UCk_AutoTest_Base` (Plugins/CkTests/Script/Common) is stored as `Ck_AutoTest_Base`.
    static const TCHAR* AutoTestBaseBareName = TEXT("Ck_AutoTest_Base");

    // Source form, because this one is EMITTED into .as files rather than looked up.
    static const TCHAR* AutoTestRunnerSourceName = TEXT("ACk_AutoTestRunner");

    // A class whose source path contains this was emitted by us — never a hand-authored collision.
    static const TCHAR* GeneratedDirSegment = TEXT("/Script/Generated/");

    // ---- Locating UCk_AutoTest_Base ------------------------------------

    auto Find_AutoTestBaseClass() -> UClass*
    {
        // AS-defined, so there is no C++ symbol to link against. The prefixed fallback only
        // matters if a future AS-UE revision changes the storage convention.
        if (auto* Found = FindFirstObject<UClass>(AutoTestBaseBareName, EFindFirstObjectOptions::None))
        { return Found; }
        return FindFirstObject<UClass>(TEXT("UCk_AutoTest_Base"), EFindFirstObjectOptions::None);
    }

    // ---- Filtering -----------------------------------------------------

    auto Is_IncludedAutoTestClass(UClass* InClass, UClass* InAutoTestBase) -> bool
    {
        if (ck::Is_NOT_Valid(InClass))
        { return false; }

        if (InClass == InAutoTestBase)
        { return false; }

        if (NOT InClass->IsChildOf(InAutoTestBase))
        { return false; }

        constexpr auto DisqualifyingFlags =
            CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;

        if (InClass->HasAnyClassFlags(DisqualifyingFlags))
        { return false; }

        // A class deleted from .as source lingers in TObjectIterator until GC runs, and would
        // otherwise keep reappearing in the generated file.
        if (InClass->IsUnreachable() ||
            InClass->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
        { return false; }

#if WITH_ANGELSCRIPT_CK
        // A vanished source FILE is the strong signal the class is gone; a class merely deleted
        // out of a still-present file leaves its stale UASClass behind and is not caught here.
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

        // Net-mode tests belong to FCkAutoTestNetStubGenerator: a wrapper emitted for one would
        // silently fail in single-PIE, its AS body needing the multi-PIE harness' cross-world
        // coordination.
        if (FCkAutoTestNetStubGenerator::Read_NetMode(InClass) !=
            FCkAutoTestNetStubGenerator::ENetMode::Standalone)
        { return false; }

        return true;
    }

    // ---- Naming --------------------------------------------------------

    auto Get_WrapperSourceName(const UClass* InEntityScriptClass) -> FString
    {
        return FString::Printf(TEXT("A%s_Actor"), *InEntityScriptClass->GetName());
    }

    // No `A` prefix — this form is for FindFirstObject, which sees UE's stripped storage names.
    auto Get_WrapperBareName(const UClass* InEntityScriptClass) -> FString
    {
        return FString::Printf(TEXT("%s_Actor"), *InEntityScriptClass->GetName());
    }

    // Prefix preserved: the AS compiler expects `U<X>` in the emitted source.
    auto Get_EntityScriptSourceName(const UClass* InEntityScriptClass) -> FString
    {
        return ck::Format_UE(TEXT("{}{}"),
            InEntityScriptClass->GetPrefixCPP(), InEntityScriptClass->GetName());
    }

    // ---- Hand-authored wrapper detection -------------------------------

    // True only for a USER-authored wrapper. Our own previous output is ignored (we would
    // otherwise deadlock ourselves), as are stale UClasses of just-removed AS classes.
    auto Has_HandAuthoredWrapper(const FString& InWrapperBareName) -> bool
    {
        auto* Existing = FindFirstObject<UClass>(*InWrapperBareName, EFindFirstObjectOptions::None);
        if (ck::Is_NOT_Valid(Existing))
        { return false; }

        // A just-deleted wrapper lingers until GC; without this it keeps tripping the collision
        // check and deleting a hand-authored wrapper would never hand emission back to us.
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
            if (NOT SourcePath.IsEmpty() && NOT FPaths::FileExists(SourcePath))
            { return false; }

            // Not an AS class we can reason about — assume hand-authored and stay out of the way.
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

    // Unset when the field is missing, unreadable, OR equal to the harness default — the caller
    // emits nothing in that case, keeping the generated file tight for the common test.
    auto Get_TimeoutOverride(const UClass* InEntityScriptClass) -> TOptional<float>
    {
        constexpr auto HarnessDefault = 5.0f;

        const auto* CDO = InEntityScriptClass->GetDefaultObject();
        if (ck::Is_NOT_Valid(CDO))
        { return {}; }

        const auto* Property = InEntityScriptClass->FindPropertyByName(TEXT("_TimeoutSeconds"));
        if (Property == nullptr)
        { return {}; }

        // AS-UE binds a script-side `float` to EITHER FFloatProperty or FDoubleProperty depending
        // on the property, so both have to be handled.
        auto Value = HarnessDefault;
        if (const auto* FloatProp = CastField<FFloatProperty>(Property))
        {
            Value = FloatProp->GetPropertyValue_InContainer(CDO);
        }
        else if (const auto* DoubleProp = CastField<FDoubleProperty>(Property))
        {
            Value = static_cast<float>(DoubleProp->GetPropertyValue_InContainer(CDO));
        }
        else
        {
            return {};
        }

        if (FMath::IsNearlyEqual(Value, HarnessDefault, KINDA_SMALL_NUMBER))
        { return {}; }

        return Value;
    }

    auto Format_WrapperBlock(const UClass* InEntityScriptClass) -> FString
    {
        const auto WrapperSourceName = Get_WrapperSourceName(InEntityScriptClass);

        // Emitted as a string literal resolved at runtime, NOT as a compile-time class reference —
        // FileHeader below carries the full rationale into the generated file itself.
        const auto EntityScriptPath = InEntityScriptClass->GetPathName();
        const auto TimeoutOverride = Get_TimeoutOverride(InEntityScriptClass);

        auto Block = FString{};
        Block += ck::Format_UE(TEXT("class {} : {}\n"), WrapperSourceName, AutoTestRunnerSourceName);
        Block += TEXT("{\n");
        if (TimeoutOverride.IsSet())
        {
            // The wrapper's own _TimeoutSeconds is what ACk_AutoTestRunner::PrepareTest turns into
            // AFunctionalTest::TimeLimit. MinFractionalDigits=1 forces "2.0f" over "2f".
            const auto Literal = FString::SanitizeFloat(*TimeoutOverride, /*MinFractionalDigits=*/1);
            Block += ck::Format_UE(TEXT("    default _TimeoutSeconds = {}f;\n"), Literal);
        }
        Block += TEXT("    UFUNCTION(BlueprintOverride)\n");
        Block += TEXT("    TSubclassOf<UCk_EntityScript_UE> Get_TestEntityScriptClass() const\n");
        Block += TEXT("    {\n");
        Block += ck::Format_UE(TEXT("        auto Path = FSoftClassPath(\"{}\");\n"), EntityScriptPath);
        Block += TEXT("        TSubclassOf<UCk_EntityScript_UE> ResolvedClass;\n");
        Block += TEXT("        ResolvedClass = Path.TryLoadClass();\n");
        Block += TEXT("        return ResolvedClass;\n");
        Block += TEXT("    }\n");
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
        TEXT("// Regenerated on editor startup and after every AngelScript recompile.\n")
        TEXT("//\n")
        TEXT("// =====================================================================\n")
        TEXT("// WHY DO THESE WRAPPERS LOOK SO WEIRD?\n")
        TEXT("// =====================================================================\n")
        TEXT("//\n")
        TEXT("// You'd normally write a wrapper like this — short, type-safe:\n")
        TEXT("//\n")
        TEXT("//   class A<TestName>_Actor : ACk_AutoTestRunner\n")
        TEXT("//   {\n")
        TEXT("//       default _TestEntityScriptClass = U<TestName>;   // compile-time ref\n")
        TEXT("//   }\n")
        TEXT("//\n")
        TEXT("// We don't, because that compile-time reference creates a deadlock when\n")
        TEXT("// the entity-script .as file is deleted while the editor is running:\n")
        TEXT("//\n")
        TEXT("//   1. AS file watcher misses the delete for one cycle.\n")
        TEXT("//   2. Generator emits a wrapper still referencing U<TestName>.\n")
        TEXT("//   3. AS recompiles the generated file → fails because U<TestName> is\n")
        TEXT("//      gone → PostCompile stops firing → generator can't fix the file\n")
        TEXT("//      it just emitted. Editor stays broken until manual recovery.\n")
        TEXT("//\n")
        TEXT("// The runtime-resolved form below sidesteps the deadlock: the entity-\n")
        TEXT("// script class is referenced as a string literal inside an override of\n")
        TEXT("// Get_TestEntityScriptClass, looked up at runtime via FSoftClassPath.\n")
        TEXT("// AS doesn't resolve the string at compile time, so the wrapper compiles\n")
        TEXT("// regardless of whether U<TestName> exists. If it's gone, the lookup\n")
        TEXT("// returns null and the test reports a clear runtime failure; one sync\n")
        TEXT("// pass later the wrapper is removed entirely. Self-healing.\n")
        TEXT("//\n")
        TEXT("// =====================================================================\n")
        TEXT("// HAND-AUTHORED OPT-OUT\n")
        TEXT("// =====================================================================\n")
        TEXT("//\n")
        TEXT("// For tests that need a custom _TimeoutSeconds or any other wrapper\n")
        TEXT("// customization, hand-author your own A<TestName>_Actor class anywhere\n")
        TEXT("// OUTSIDE Script/Generated/ using the simpler compile-time form:\n")
        TEXT("//\n")
        TEXT("//   class A<TestName>_Actor : ACk_AutoTestRunner\n")
        TEXT("//   {\n")
        TEXT("//       default _TestEntityScriptClass = U<TestName>;\n")
        TEXT("//       default _TimeoutSeconds = 2.0f;\n")
        TEXT("//   }\n")
        TEXT("//\n")
        TEXT("// The generator detects hand-authored wrappers by class name + source\n")
        TEXT("// path and skips emission for that test, leaving yours authoritative.\n")
        TEXT("// (Hand-authored wrappers don't carry the deletion-race risk because\n")
        TEXT("// deleting the .as file removes BOTH classes atomically — no stale\n")
        TEXT("// generated file to get out of sync.)\n\n");

#endif // WITH_EDITOR
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAutoTestWrapperGenerator::
    GenerateAll()
    -> void
{
#if WITH_EDITOR

    // Single-writer gate (G4): a secondary instance must not rewrite <Plugin>_AutoTestActors.as.
    if (NOT FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(
            TEXT("AutoTestWrapperGenerator.GenerateAll")))
    { return; }

    ck::angelscriptgenerator::Log(TEXT("[CkAS AutoTest Wrappers] === Generating AutoTest actor wrappers ==="));

    auto* AutoTestBase = ck_autotest_wrapper_generator::Find_AutoTestBaseClass();
    if (ck::Is_NOT_Valid(AutoTestBase))
    {
        // Normal during early startup. Log rather than VeryVerbose so the silent-no-write case
        // stays diagnosable.
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

        // Only the PROJECT bucket is guarded. Its Script root is always compiled, so without the
        // guard its wrappers reference an absent ACk_AutoTestRunner and break the packaged AS
        // compile. Plugin buckets must stay UNguarded: a disabled test plugin drops its whole
        // Script root anyway, and its sibling *Assets.as references these wrapper types outside
        // any EDITOR block, which would then fail "editor-only type outside of an EDITOR block".
        const auto IsProjectBucket = Bucket.PluginName == FApp::GetProjectName();
        if (IsProjectBucket)
        { Content += TEXT("#if EDITOR\n\n"); }

        for (auto* Class : Bucket.Classes)
        {
            Content += ck_autotest_wrapper_generator::Format_WrapperBlock(Class);
        }

        if (IsProjectBucket)
        { Content += TEXT("#endif // EDITOR\n"); }

        const auto OutputDir = FPaths::GetPath(Bucket.OutputFilePath);
        IFileManager::Get().MakeDirectory(*OutputDir, true);

        // Writing unchanged content re-triggers the AngelScript PostCompile hook, which calls us
        // again, ad infinitum. Compare LF-normalized — the on-disk file may be CRLF.
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
        // .as files live as CRLF on Windows disk (core.autocrlf). Writing LF makes git flag the
        // file modified on every editor startup even though `git diff` shows nothing.
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
