#include "CkAutoTestNetStubGenerator.h"

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

namespace ck_autotest_netstub_generator
{
#if WITH_EDITOR

    // ---- Constants -----------------------------------------------------

    // The AS base class authors subclass to define a test body. Same one the wrapper generator
    // finds; mirrors that lookup verbatim so the two generators agree on the subclass set.
    // UClass storage name strips the `U` prefix — see CkAutoTestWrapperGenerator.cpp:35 for the
    // long-form explanation.
    static const TCHAR* AutoTestBaseBareName = TEXT("Ck_AutoTest_Base");

    // Source-form prefix every AS net-mode test class shares by convention. The hand-written
    // stubs derived their per-test "suffix" by stripping this prefix from the class name; we
    // replicate that derivation so the generator's output names match the hand-written ones
    // byte-for-byte. Classes that don't start with this prefix fall back to using the bare
    // class name as the suffix (mostly a future-proofing safety net — no current test does this).
    static const TCHAR* NetTestClassPrefix = TEXT("Ck_AutoTest_Net_");

    // ---- _NetMode reflection ------------------------------------------

    auto Read_NetModeByte(const UClass* InEntityScriptClass) -> uint8
    {
        const auto* CDO = InEntityScriptClass->GetDefaultObject();
        if (NOT ck::IsValid(CDO, ck::IsValid_Policy_NullptrOnly{}))
        { return 0; }

        const auto* Property = InEntityScriptClass->FindPropertyByName(TEXT("_NetMode"));
        if (Property == nullptr)
        { return 0; }

        // UENUM-backed UPROPERTY appears as FByteProperty (or FEnumProperty wrapping a byte).
        // Both surface here depending on UE version + reflection emit path.
        if (const auto* ByteProp = CastField<FByteProperty>(Property))
        {
            return ByteProp->GetPropertyValue_InContainer(CDO);
        }
        if (const auto* EnumProp = CastField<FEnumProperty>(Property))
        {
            const auto* Underlying = EnumProp->GetUnderlyingProperty();
            if (const auto* UnderByte = CastField<FByteProperty>(Underlying))
            {
                return UnderByte->GetPropertyValue_InContainer(
                    EnumProp->ContainerPtrToValuePtr<void>(CDO));
            }
        }

        return 0;
    }

    // ---- _NetSubjectClass reflection ----------------------------------

    // Default subject class path emitted when an AS test doesn't override `_NetSubjectClass`.
    // The default `ACk_AutoTest_NetSubject` (in CkTests) adds a Float attribute on its entity-
    // script — sufficient for CkAttribute net tests but not for modules with different per-entity
    // state. Modules with custom subjects (CkRelationship, CkInventory, CkStateMachine, etc.)
    // author a subclass and override the UPROPERTY on their AS test class.
    //
    // NOTE: UE's FSoftClassPath C++-class lookup strips the `A`/`U`/`F` prefix from class names —
    // the resolved path is `/Script/<Module>.<Name-without-prefix>`. The per-test override path
    // below uses `Resolved->GetPathName()` which already strips correctly, so the bug only
    // surfaces on the default fallback. Until this fix, every AS net test that DIDN'T override
    // `_NetSubjectClass` failed at SpawnActor with "failed to resolve NetSubject class via
    // FSoftClassPath" because TryLoadClass returns nullptr on the prefixed form. The first net
    // tests authored (CkAttribute) happened to override the path implicitly via the same
    // mistake working out in the FSoftClassPath registry's tolerance — but newer net tests
    // (CkTransform, which uses the default subject) tripped it.
    static const TCHAR* DefaultNetSubjectClassPath = TEXT("/Script/CkTests.Ck_AutoTest_NetSubject");

    // Read the AS test's CDO `_NetSubjectClass` UPROPERTY (declared on UCk_AutoTest_NetBase).
    // Returns the class path string suitable for emission as an FSoftClassPath literal in the
    // generated C++ stub. Falls back to the default subject path when the property is unset
    // (nullptr) or missing (a test class that doesn't derive from NetBase). Reflection mirrors
    // the existing `Read_NetModeByte` pattern; class properties surface as FClassProperty.
    auto Read_NetSubjectClassPath(const UClass* InEntityScriptClass) -> FString
    {
        const auto* CDO = InEntityScriptClass->GetDefaultObject();
        if (NOT ck::IsValid(CDO, ck::IsValid_Policy_NullptrOnly{}))
        { return DefaultNetSubjectClassPath; }

        const auto* Property = InEntityScriptClass->FindPropertyByName(TEXT("_NetSubjectClass"));
        const auto* ClassProp = CastField<FClassProperty>(Property);
        if (ClassProp == nullptr)
        { return DefaultNetSubjectClassPath; }

        auto* Resolved = Cast<UClass>(ClassProp->GetObjectPropertyValue_InContainer(CDO));
        if (ck::Is_NOT_Valid(Resolved, ck::IsValid_Policy_NullptrOnly{}))
        { return DefaultNetSubjectClassPath; }

        return Resolved->GetPathName();
    }

    // ---- Locating UCk_AutoTest_Base ------------------------------------

    auto Find_AutoTestBaseClass() -> UClass*
    {
        if (auto* Found = FindFirstObject<UClass>(AutoTestBaseBareName, EFindFirstObjectOptions::None))
        { return Found; }
        return FindFirstObject<UClass>(TEXT("UCk_AutoTest_Base"), EFindFirstObjectOptions::None);
    }

    // ---- Filtering -----------------------------------------------------

    // Same shape as the wrapper generator's filter — same staleness checks, same disqualifying
    // flags. Diverges on the mode check: this generator keeps only Net-mode classes; the wrapper
    // keeps only Standalone-mode ones. Together they cover every subclass exactly once.
    auto Is_IncludedNetClass(UClass* InClass, UClass* InAutoTestBase) -> bool
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

        if (InClass->IsUnreachable() ||
            InClass->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
        { return false; }

#if WITH_ANGELSCRIPT_CK
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

        // Filter out Standalone-mode classes — the wrapper generator handles those. Anything
        // non-zero is a Net mode (Independent or Replicated).
        if (Read_NetModeByte(InClass) == 0)
        { return false; }

        // Filter out base/scaffolding classes — only leaf test classes named
        // `Ck_AutoTest_Net_<Name>` should emit stubs. Without this, `Ck_AutoTest_NetBase`
        // (which sets `default _NetMode = Replicated` for its subclasses' convenience) would
        // itself get a stub and surface in Session Frontend as a phantom test — and any future
        // intermediate base like `Ck_AutoTest_NetBase_WithRefill` would too. The prefix is the
        // natural leaf-test marker; bases live under `Common/` and are named without it.
        if (NOT InClass->GetName().StartsWith(NetTestClassPrefix, ESearchCase::CaseSensitive))
        { return false; }

        return true;
    }

    // ---- Naming derivation --------------------------------------------

    // Class storage name strips the `U` prefix: `UCk_AutoTest_Net_Foo` -> `Ck_AutoTest_Net_Foo`.
    // We strip the further `Ck_AutoTest_Net_` prefix to get the per-test "suffix" that all the
    // emitted names build off of. Classes lacking that prefix fall back to their bare class
    // name — generator stays useful even if naming convention drifts in the future.
    auto Get_TestSuffix(const UClass* InEntityScriptClass) -> FString
    {
        const auto BareName = InEntityScriptClass->GetName();
        if (BareName.StartsWith(NetTestClassPrefix))
        { return BareName.RightChop(FCString::Strlen(NetTestClassPrefix)); }
        return BareName;
    }

    // Resolves the AS source file path for a test class. Empty when the class isn't AS-backed
    // (or AS support is compiled out) — callers treat empty as "no path convention applies".
    auto Get_AsSourceFilePath(const UClass* InEntityScriptClass) -> FString
    {
#if WITH_ANGELSCRIPT_CK
        auto* ASClass = UASClass::GetFirstASClass(const_cast<UClass*>(InEntityScriptClass));
        if (ASClass == nullptr)
        { return {}; }

        return ASClass->GetSourceFilePath();
#else
        return {};
#endif
    }

    // Derives the "feature" segment used in the test path's middle component (e.g. "Attribute"
    // for `Ck.Attribute.Net.AS_*`) AND in the output filename. Reads the AS source path; the
    // path-convention logic lives in `Derive_FeatureFromSourcePath` (pure, unit-tested).
    auto Get_TestFeature(const UClass* InEntityScriptClass) -> FString
    {
        return FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(
            Get_AsSourceFilePath(InEntityScriptClass));
    }

    // The C++ AutomationTest class identifier emitted to `IMPLEMENT_SIMPLE_AUTOMATION_TEST`.
    // Matches the hand-written stubs' shape: `FCk<Feature>Net_AS_<Suffix>`.
    auto Get_TestClassName(const FString& InFeature, const FString& InSuffix) -> FString
    {
        return ck::Format_UE(TEXT("FCk{}Net_AS_{}"), InFeature, InSuffix);
    }

    // The test path string emitted to `IMPLEMENT_SIMPLE_AUTOMATION_TEST`. Surfaces in Session
    // Frontend / `--test-pattern` matches as e.g. `Ck.Attribute.Net.AS_Float_InitialValueReplicates`.
    auto Get_TestPath(const FString& InFeature, const FString& InSuffix) -> FString
    {
        return ck::Format_UE(TEXT("Ck.{}.Net.AS_{}"), InFeature, InSuffix);
    }

    // ---- Block formatting ---------------------------------------------

    // Per-test block. Two variants — Replicated includes the server-side NetSubject spawn +
    // post-spawn settle ticks; Independent skips both. Everything else is identical, including
    // the log-suppression bracketing the harness relies on.
    auto Format_TestBlock(
        const UClass* InEntityScriptClass,
        FCkAutoTestNetStubGenerator::ENetMode InNetMode)
        -> FString
    {
        const auto Suffix = Get_TestSuffix(InEntityScriptClass);
        const auto Feature = Get_TestFeature(InEntityScriptClass);
        const auto TestClassName = Get_TestClassName(Feature, Suffix);
        const auto TestPath = Get_TestPath(Feature, Suffix);
        const auto AsClassPath = InEntityScriptClass->GetPathName();

        const auto IsReplicated =
            (InNetMode == FCkAutoTestNetStubGenerator::ENetMode::Replicated);

        auto Block = FString{};

        Block += ck::Format_UE(TEXT("// Auto-generated from AS class {} ({}-mode).\n"),
            InEntityScriptClass->GetName(),
            IsReplicated ? TEXT("Replicated") : TEXT("ServerAndClientsIndependent"));
        Block += TEXT("// DO NOT EDIT — regenerated on editor startup and every successful AS recompile.\n\n");

        Block += TEXT("namespace\n");
        Block += TEXT("{\n");
        Block += ck::Format_UE(TEXT("    constexpr auto kAsClassPath_{} = TEXT(\"{}\");\n"),
            Suffix, AsClassPath);
        Block += ck::Format_UE(TEXT("    constexpr auto kTimeoutSeconds_{} = 30.0f;\n"), Suffix);
        Block += TEXT("}\n\n");

        Block += TEXT("IMPLEMENT_SIMPLE_AUTOMATION_TEST(\n");
        Block += ck::Format_UE(TEXT("    {},\n"), TestClassName);
        Block += ck::Format_UE(TEXT("    \"{}\",\n"), TestPath);
        Block += TEXT("    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)\n\n");

        Block += ck::Format_UE(TEXT("bool {}::RunTest(const FString& Parameters)\n"), TestClassName);
        Block += TEXT("{\n");
        Block += TEXT("    bSuppressLogErrors = true;\n");
        Block += TEXT("    bSuppressLogWarnings = true;\n\n");
        Block += TEXT("    constexpr auto NumPIEClients = 2;\n");
        Block += TEXT("    constexpr auto ExpectedTotalWorlds = 2;\n");
        Block += TEXT("    constexpr auto ReadyTimeoutSeconds = 30.0f;\n");
        if (IsReplicated)
        {
            Block += TEXT("    constexpr auto FramesAfterSpawn = 30;\n");
        }
        Block += TEXT("\n");
        Block += TEXT("    const auto MapPath = FString{TEXT(\"/Engine/Maps/Entry\")};\n\n");

        Block += TEXT("    ADD_LATENT_AUTOMATION_COMMAND(FCk_Latent_StartPIEMultiClient(NumPIEClients, MapPath));\n");
        Block += TEXT("    ADD_LATENT_AUTOMATION_COMMAND(FCk_Latent_WaitForPIEReady(ExpectedTotalWorlds, ReadyTimeoutSeconds));\n\n");

        if (IsReplicated)
        {
            // The AS class CDO's `_NetSubjectClass` UPROPERTY (declared on UCk_AutoTest_NetBase)
            // selects which subject actor the harness spawns. Default fallback is the base
            // `ACk_AutoTest_NetSubject`. The class path is embedded as an FSoftClassPath literal
            // and resolved at run time via TryLoadClass — same lazy-resolve pattern the standalone
            // wrapper generator uses for entity-script class lookups (avoids hard C++ header deps
            // on a per-test subject subclass).
            const auto SubjectClassPath = Read_NetSubjectClassPath(InEntityScriptClass);

            Block += TEXT("    ADD_LATENT_AUTOMATION_COMMAND(FCk_Latent_RunOnServer(\n");
            Block += TEXT("        FCk_NetAutoTest_ServerAction::CreateLambda([this](UWorld* InServer) -> void\n");
            Block += TEXT("        {\n");
            Block += TEXT("            auto SpawnInfo = FActorSpawnParameters{};\n");
            Block += TEXT("            SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;\n");
            Block += ck::Format_UE(
                TEXT("            const auto SubjectClassPath = FSoftClassPath(TEXT(\"{}\"));\n"),
                SubjectClassPath);
            Block += TEXT("            auto* SubjectClass = SubjectClassPath.TryLoadClass<ACk_AutoTest_NetSubject>();\n");
            Block += TEXT("            if (SubjectClass == nullptr)\n");
            Block += TEXT("            { AddError(TEXT(\"AS-test harness: failed to resolve NetSubject class via FSoftClassPath\")); return; }\n");
            Block += TEXT("            auto* Subject = InServer->SpawnActor<ACk_AutoTest_NetSubject>(\n");
            Block += TEXT("                SubjectClass, FTransform::Identity, SpawnInfo);\n");
            Block += TEXT("            if (Subject == nullptr)\n");
            Block += TEXT("            { AddError(TEXT(\"AS-test harness: server-side SpawnActor returned null\")); }\n");
            Block += TEXT("        })));\n\n");
            Block += TEXT("    ADD_LATENT_AUTOMATION_COMMAND(FCk_Latent_TickWorlds(FramesAfterSpawn));\n\n");
        }

        // Literal braces around `kAsClassPath_<Suffix>` need libfmt `{{` / `}}` escaping;
        // the FString{...} initializer in the emitted C++ collides with fmt's substitution
        // syntax otherwise.
        Block += ck::Format_UE(
            TEXT("    ADD_LATENT_AUTOMATION_COMMAND(FCk_Latent_RunAsTestOnAllWorlds(this, FString{{kAsClassPath_{}}}, kTimeoutSeconds_{}));\n\n"),
            Suffix, Suffix);

        Block += TEXT("    ADD_LATENT_AUTOMATION_COMMAND(FCk_Latent_EndPIE());\n\n");

        Block += TEXT("    ADD_LATENT_AUTOMATION_COMMAND(FCk_Latent_AssertCondition(this,\n");
        Block += TEXT("        FCk_NetAutoTest_Assertion::CreateLambda([]() -> bool\n");
        Block += TEXT("        {\n");
        Block += TEXT("            FAutomationTestBase::bSuppressLogErrors = false;\n");
        Block += TEXT("            FAutomationTestBase::bSuppressLogWarnings = false;\n");
        Block += TEXT("            return true;\n");
        Block += TEXT("        }),\n");
        Block += TEXT("        TEXT(\"restore log suppression statics\")));\n\n");

        Block += TEXT("    return true;\n");
        Block += TEXT("}\n\n");

        Block += TEXT("// --------------------------------------------------------------------------------------------------------------------\n\n");

        return Block;
    }

    // ---- Feature bucketing --------------------------------------------

    struct FFeatureBucket
    {
        FString FeatureName;
        FString OutputFilePath;
        TArray<UClass*> Classes;
    };

    // Output root for plugin-authored tests. Empty when CkTests isn't enabled.
    auto Get_CkTestsOutputDir() -> FString
    {
        if (const auto CkTestsPlugin = IPluginManager::Get().FindPlugin(TEXT("CkTests")))
        {
            return CkTestsPlugin->GetBaseDir() / TEXT("Source") / TEXT("CkTests")
                 / TEXT("Private") / TEXT("Net") / TEXT("Generated");
        }
        return {};
    }

    // Output root for project-authored tests: `<ProjectDir>/Source/<ProjectName>/Tests/Net/
    // Generated`. Empty when the project has no `Source/<ProjectName>` module dir to emit into
    // (e.g. a content-only project, or a primary module named differently from the .uproject) —
    // we refuse to guess rather than fall back to the CkTests submodule, because a cross-repo
    // stub is exactly the churn this split exists to prevent.
    auto Get_ProjectOutputDir() -> FString
    {
        const auto ProjectModuleDir = FPaths::GameSourceDir() / FApp::GetProjectName();
        if (NOT IFileManager::Get().DirectoryExists(*ProjectModuleDir))
        { return {}; }

        return ProjectModuleDir / TEXT("Tests") / TEXT("Net") / TEXT("Generated");
    }

    // Buckets by (output root, feature): plugin-authored tests group under CkTests, project-
    // authored ones under the project's own source tree — see the header docstring for why the
    // split matters. Classes whose root is unavailable are skipped with a warning (never
    // silently, never rerouted across repos).
    auto Bucket_ClassesByFeature(const TArray<UClass*>& InClasses) -> TArray<FFeatureBucket>
    {
        const auto CkTestsOutputDir = Get_CkTestsOutputDir();
        const auto ProjectOutputDir = Get_ProjectOutputDir();

        auto BucketMap = TMap<FString, FFeatureBucket>{};
        auto SkippedNoCkTests = int32{0};
        auto SkippedNoProjectDir = int32{0};

        for (auto* Class : InClasses)
        {
            const auto SourcePath = Get_AsSourceFilePath(Class);
            const auto IsProjectAuthored = FCkAutoTestNetStubGenerator::Get_IsProjectAuthoredPath(
                SourcePath, FPaths::ProjectDir());

            const auto& OutputDir = IsProjectAuthored ? ProjectOutputDir : CkTestsOutputDir;
            if (OutputDir.IsEmpty())
            {
                (IsProjectAuthored ? SkippedNoProjectDir : SkippedNoCkTests)++;
                continue;
            }

            const auto Feature = FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(SourcePath);
            const auto OutputFilePath = OutputDir
                / ck::Format_UE(TEXT("{}_NetAutoTestStubs.spec.cpp"), Feature);

            if (NOT BucketMap.Contains(OutputFilePath))
            {
                auto Bucket = FFeatureBucket{};
                Bucket.FeatureName = Feature;
                Bucket.OutputFilePath = OutputFilePath;
                BucketMap.Add(OutputFilePath, MoveTemp(Bucket));
            }

            BucketMap[OutputFilePath].Classes.Add(Class);
        }

        if (SkippedNoCkTests > 0)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[CkAS Net Stubs] CkTests plugin not enabled — {} plugin-authored net-mode ")
                TEXT("test stub(s) can't be written to disk; skipped."),
                SkippedNoCkTests);
        }
        if (SkippedNoProjectDir > 0)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[CkAS Net Stubs] Project module source dir [{}] not found — {} project-")
                TEXT("authored net-mode test stub(s) can't be written to disk; skipped."),
                FPaths::GameSourceDir() / FApp::GetProjectName(), SkippedNoProjectDir);
        }

        auto Result = TArray<FFeatureBucket>{};
        BucketMap.GenerateValueArray(Result);
        Result.Sort([](const FFeatureBucket& A, const FFeatureBucket& B)
        {
            return A.OutputFilePath < B.OutputFilePath;
        });
        return Result;
    }

    // Deletes every `*_NetAutoTestStubs.spec.cpp` under `InOutputDir` that isn't in the expected
    // set. No-op when the root is unresolved. Membership is by full path, so one combined
    // expected set serves both roots.
    auto Prune_StaleStubs(const FString& InOutputDir, const TSet<FString>& InExpectedPaths) -> void
    {
        if (InOutputDir.IsEmpty())
        { return; }

        auto ExistingFiles = TArray<FString>{};
        IFileManager::Get().FindFiles(ExistingFiles, *(InOutputDir / TEXT("*_NetAutoTestStubs.spec.cpp")), true, false);
        for (const auto& Leaf : ExistingFiles)
        {
            const auto FullPath = FPaths::ConvertRelativePathToFull(InOutputDir / Leaf);
            if (NOT InExpectedPaths.Contains(FullPath))
            {
                IFileManager::Get().Delete(*FullPath, /*RequireExists=*/false, /*EvenReadOnly=*/false, /*Quiet=*/true);
                ck::angelscriptgenerator::Log(
                    TEXT("[CkAS Net Stubs] Pruned stale generated file: {}"), FullPath);
            }
        }
    }

    // ---- File content -------------------------------------------------

    static const TCHAR* FileHeader =
        TEXT("// Auto-generated multi-client AutoTest C++ stubs — DO NOT EDIT.\n")
        TEXT("// Regenerated on editor startup and after every AngelScript recompile.\n")
        TEXT("//\n")
        TEXT("// =====================================================================\n")
        TEXT("// WHAT IS THIS FILE?\n")
        TEXT("// =====================================================================\n")
        TEXT("//\n")
        TEXT("// Each block below is the C++ orchestration glue for one AS-authored net\n")
        TEXT("// test. The actual test body lives in the corresponding .as file under\n")
        TEXT("// `Plugins/<X>/Script/Ck<Feature>/CkAutoTest_Net_*.as`. AS authors write\n")
        TEXT("// one .as file — this generator produces the C++ stub from the AS class's\n")
        TEXT("// CDO `_NetMode` default, choosing the Replicated- or Independent-mode\n")
        TEXT("// shape automatically.\n")
        TEXT("//\n")
        TEXT("// `Replicated`-mode stubs spawn an `ACk_AutoTest_NetSubject` on the server\n")
        TEXT("// then run the AS body on every world. `ServerAndClientsIndependent` stubs\n")
        TEXT("// skip the spawn — the AS body operates on each world's TransientEntity\n")
        TEXT("// without cross-world coordination.\n")
        TEXT("//\n")
        TEXT("// To author a new net test:\n")
        TEXT("//   1. Drop a `Ck_AutoTest_Net_<Name>.as` under `Plugins/<X>/Script/Ck<Feature>/`.\n")
        TEXT("//   2. Subclass `UCk_AutoTest_NetBase` (defaults to Replicated) or set\n")
        TEXT("//      `default _NetMode = ECk_AutoTest_NetMode::ServerAndClientsIndependent;`\n")
        TEXT("//      on a `UCk_AutoTest_Base` subclass.\n")
        TEXT("//   3. Recompile AS — the generator emits the matching stub here on the\n")
        TEXT("//      next PostCompile. No C++ edits required.\n")
        TEXT("// =====================================================================\n")
        TEXT("\n")
        TEXT("#include \"Misc/AutomationTest.h\"\n")
        TEXT("\n")
        TEXT("#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS\n")
        TEXT("\n")
        TEXT("#include \"Engine/World.h\"\n")
        TEXT("\n")
        TEXT("#include \"CkTests/Net/CkAutoTest_NetSubject.h\"\n")
        TEXT("#include \"CkTests/Net/CkNetAutomation_Common.h\"\n")
        TEXT("\n")
        TEXT("// --------------------------------------------------------------------------------------------------------------------\n\n");

    static const TCHAR* FileFooter =
        TEXT("#endif // WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS\n");

#endif // WITH_EDITOR
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAutoTestNetStubGenerator::
    Derive_FeatureFromSourcePath(const FString& InSourcePath) -> FString
{
    const auto Fallback = FString{TEXT("AS")};

    if (InSourcePath.IsEmpty())
    { return Fallback; }

    auto Normalized = FPaths::ConvertRelativePathToFull(InSourcePath);
    FPaths::NormalizeFilename(Normalized);

    auto Parts = TArray<FString>{};
    Normalized.ParseIntoArray(Parts, TEXT("/"), /*InCullEmpty=*/true);

    // Walk path components from leaf to root looking for a convention match; the nearest match
    // to the leaf wins, so deeper nestings (`Script/CkAttribute/Sub/foo.as`) still bucket to
    // `Attribute`. The loop starts at the file's PARENT — the feature segment must be a
    // directory, never the filename itself.
    for (auto i = Parts.Num() - 2; i > 0; --i)
    {
        // Plugin convention: `Script/Ck<Feature>/`.
        if (Parts[i - 1].Equals(TEXT("Script"), ESearchCase::IgnoreCase) &&
            Parts[i].StartsWith(TEXT("Ck"), ESearchCase::CaseSensitive))
        {
            const auto Feature = Parts[i].RightChop(2);
            if (NOT Feature.IsEmpty())
            { return Feature; }
        }

        // Project convention: `Script/Tests/<Feature>/`. Matched on the `Tests/<Feature>` pair —
        // plugin trees never produce one (their tests sit directly under `Script/Ck<Feature>/`,
        // which the branch above claims first when both could apply).
        if (Parts[i - 1].Equals(TEXT("Tests"), ESearchCase::IgnoreCase))
        { return Parts[i]; }
    }

    return Fallback;
}

auto
    FCkAutoTestNetStubGenerator::
    Get_IsProjectAuthoredPath(const FString& InSourcePath, const FString& InProjectDir) -> bool
{
    if (InSourcePath.IsEmpty() || InProjectDir.IsEmpty())
    { return false; }

    auto NormalizedSource = FPaths::ConvertRelativePathToFull(InSourcePath);
    FPaths::NormalizeFilename(NormalizedSource);

    auto ProjectScriptDir = FPaths::ConvertRelativePathToFull(InProjectDir / TEXT("Script"));
    FPaths::NormalizeDirectoryName(ProjectScriptDir);

    // Trailing separator guards against sibling-prefix false positives (`.../ScriptExtra/`).
    // Case-insensitive: generated/source paths on Windows mix casings freely.
    return NormalizedSource.StartsWith(ProjectScriptDir + TEXT("/"), ESearchCase::IgnoreCase);
}

auto
    FCkAutoTestNetStubGenerator::
    Read_NetMode(const UClass* InEntityScriptClass) -> ENetMode
{
#if WITH_EDITOR
    const auto Raw = ck_autotest_netstub_generator::Read_NetModeByte(InEntityScriptClass);
    if (Raw > static_cast<uint8>(ENetMode::Replicated))
    { return ENetMode::Standalone; }
    return static_cast<ENetMode>(Raw);
#else
    return ENetMode::Standalone;
#endif
}

auto
    FCkAutoTestNetStubGenerator::
    GenerateAll()
    -> void
{
#if WITH_EDITOR

    // Single-writer gate (G5): a secondary instance must not write/prune the net-test .spec.cpp
    // stubs (these land in Source trees but carry the identical two-process churn risk).
    if (NOT FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(
            TEXT("NetStubGenerator.GenerateAll")))
    { return; }

    ck::angelscriptgenerator::Log(TEXT("[CkAS Net Stubs] === Generating net-mode AutoTest C++ stubs ==="));

    auto* AutoTestBase = ck_autotest_netstub_generator::Find_AutoTestBaseClass();
    if (NOT ck::IsValid(AutoTestBase, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::angelscriptgenerator::Log(
            TEXT("[CkAS Net Stubs] Ck_AutoTest_Base not found in object table — ")
            TEXT("skipping pass (CkTests not loaded yet, or AS not yet compiled)."));
        return;
    }

    auto AllSubclasses = TArray<UClass*>{};
    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        auto* Class = *ClassIterator;
        if (ck_autotest_netstub_generator::Is_IncludedNetClass(Class, AutoTestBase))
        {
            AllSubclasses.Add(Class);
        }
    }

    AllSubclasses.Sort([](const UClass& InA, const UClass& InB)
    {
        return InA.GetPathName() < InB.GetPathName();
    });

    ck::angelscriptgenerator::Log(
        TEXT("[CkAS Net Stubs] Discovered {} net-mode subclasses of Ck_AutoTest_Base."),
        AllSubclasses.Num());

    const auto Buckets = ck_autotest_netstub_generator::Bucket_ClassesByFeature(AllSubclasses);

    // Prune stale generated files. Unlike the wrapper generator (one file per plugin, plugin
    // names don't churn), this generator's bucket key is "feature subdir under Script/" — and
    // a feature can disappear if its last net test is deleted, or MOVE buckets if its source
    // file relocates (a stale copy left behind would then duplicate the emitted C++ test class
    // and trip UBT). Each output root is pruned against the set of file paths the current pass
    // intends to write.
    //
    // Conservative guard: when discovery finds ZERO net-mode classes, skip pruning entirely.
    // A legitimate all-tests-deleted state is indistinguishable here from a broken discovery
    // pass (e.g. an unusual boot where AS classes aren't registered), and deleting committed
    // files on a broken pass surfaces as mysterious repo churn on every affected machine. The
    // cost of skipping is bounded: a genuinely-orphaned stub compiles standalone (its class
    // references are string paths) and merely surfaces a phantom Session Frontend entry.
    if (AllSubclasses.Num() > 0)
    {
        auto ExpectedPaths = TSet<FString>{};
        for (const auto& Bucket : Buckets)
        { ExpectedPaths.Add(FPaths::ConvertRelativePathToFull(Bucket.OutputFilePath)); }

        ck_autotest_netstub_generator::Prune_StaleStubs(
            ck_autotest_netstub_generator::Get_CkTestsOutputDir(), ExpectedPaths);
        ck_autotest_netstub_generator::Prune_StaleStubs(
            ck_autotest_netstub_generator::Get_ProjectOutputDir(), ExpectedPaths);
    }
    else
    {
        ck::angelscriptgenerator::Log(
            TEXT("[CkAS Net Stubs] No net-mode classes discovered — skipping stale-file prune ")
            TEXT("(conservative: can't distinguish all-deleted from a broken discovery pass)."));
    }

    auto TotalEmitted = int32{0};

    for (const auto& Bucket : Buckets)
    {
        auto Content = FString{ck_autotest_netstub_generator::FileHeader};

        for (auto* Class : Bucket.Classes)
        {
            const auto Mode = FCkAutoTestNetStubGenerator::Read_NetMode(Class);
            Content += ck_autotest_netstub_generator::Format_TestBlock(Class, Mode);
        }

        Content += ck_autotest_netstub_generator::FileFooter;

        const auto OutputDir = FPaths::GetPath(Bucket.OutputFilePath);
        IFileManager::Get().MakeDirectory(*OutputDir, true);

        // Diff-skip in LF-normalized form — same pattern as the wrapper generator. Without
        // this, our own write would re-trigger UBT's source-tree scan -> AS PostCompile ->
        // GenerateAll, ad infinitum.
        auto ExistingContent = FString{};
        const auto HasExisting = FFileHelper::LoadFileToString(ExistingContent, *Bucket.OutputFilePath);

        auto ContentForCompare = Content;
        ContentForCompare.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
        auto ExistingForCompare = ExistingContent;
        ExistingForCompare.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

        if (HasExisting && ExistingForCompare.Equals(ContentForCompare, ESearchCase::CaseSensitive))
        {
            ck::angelscriptgenerator::VeryVerbose(
                TEXT("[CkAS Net Stubs] [{}] up-to-date ({} stubs)"),
                Bucket.FeatureName, Bucket.Classes.Num());
            TotalEmitted += Bucket.Classes.Num();
            continue;
        }

#if PLATFORM_WINDOWS
        Content.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
        Content.ReplaceInline(TEXT("\n"), TEXT("\r\n"));
#endif

        if (FFileHelper::SaveStringToFile(Content, *Bucket.OutputFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {
            ck::angelscriptgenerator::Log(
                TEXT("[CkAS Net Stubs] [{}] {} stubs -> {}"),
                Bucket.FeatureName, Bucket.Classes.Num(), Bucket.OutputFilePath);
        }
        else
        {
            ck::angelscriptgenerator::Error(
                TEXT("[CkAS Net Stubs] Failed to write to [{}]"),
                Bucket.OutputFilePath);
        }

        TotalEmitted += Bucket.Classes.Num();
    }

    ck::angelscriptgenerator::Log(
        TEXT("[CkAS Net Stubs] Done — {} feature buckets, {} stubs emitted."),
        Buckets.Num(), TotalEmitted);

#endif // WITH_EDITOR
}

// --------------------------------------------------------------------------------------------------------------------
