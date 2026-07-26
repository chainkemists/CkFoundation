#include "CkAutoTestNetStubGenerator.h"

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include <HAL/FileManager.h>
#include <Interfaces/IPluginManager.h>
#include <ModuleDescriptor.h>
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

    // Storage name (no `U` prefix). The wrapper generator's lookup is mirrored VERBATIM so the
    // two generators can never disagree on the subclass set.
    static const TCHAR* AutoTestBaseBareName = TEXT("Ck_AutoTest_Base");

    // Every leaf net test is named `Ck_AutoTest_Net_<Suffix>` by convention; stripping this
    // prefix yields the suffix every emitted name is built from.
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

        // A UENUM-backed UPROPERTY surfaces as FByteProperty OR as an FEnumProperty wrapping one.
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

    // Prefix-LESS on purpose: FSoftClassPath's C++-class lookup strips `A`/`U`/`F`, so the
    // prefixed form resolves to nullptr and every non-overriding test fails at SpawnActor.
    static const TCHAR* DefaultNetSubjectClassPath = TEXT("/Script/CkTests.Ck_AutoTest_NetSubject");

    // Emitted as an FSoftClassPath literal in the stub. Falls back to the default subject when
    // `_NetSubjectClass` is unset or the test doesn't derive from UCk_AutoTest_NetBase at all.
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

    // Deliberately the wrapper generator's filter with the mode check INVERTED: it keeps
    // Standalone, this keeps Net, and between them every subclass is covered exactly once.
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

        constexpr auto StandaloneNetModeByte = uint8{0};
        if (Read_NetModeByte(InClass) == StandaloneNetModeByte)
        { return false; }

        // The prefix is the leaf-test marker — scaffolding bases live under `Common/` without it.
        // `Ck_AutoTest_NetBase` sets `default _NetMode` for its subclasses and would otherwise get
        // a stub of its own, surfacing in Session Frontend as a phantom test.
        if (NOT InClass->GetName().StartsWith(NetTestClassPrefix, ESearchCase::CaseSensitive))
        { return false; }

        return true;
    }

    // ---- Naming derivation --------------------------------------------

    // Falls back to the bare class name so the generator survives a drift in naming convention.
    auto Get_TestSuffix(const UClass* InEntityScriptClass) -> FString
    {
        const auto BareName = InEntityScriptClass->GetName();
        if (BareName.StartsWith(NetTestClassPrefix))
        { return BareName.RightChop(FCString::Strlen(NetTestClassPrefix)); }
        return BareName;
    }

    // Empty when the class isn't AS-backed — callers read that as "no path convention applies".
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

    auto Get_TestFeature(const UClass* InEntityScriptClass) -> FString
    {
        return FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(
            Get_AsSourceFilePath(InEntityScriptClass));
    }

    auto Get_TestClassName(const FString& InFeature, const FString& InSuffix) -> FString
    {
        return ck::Format_UE(TEXT("FCk{}Net_AS_{}"), InFeature, InSuffix);
    }

    // This is what Session Frontend rows and `--test-pattern` match against.
    auto Get_TestPath(const FString& InFeature, const FString& InSuffix) -> FString
    {
        return ck::Format_UE(TEXT("Ck.{}.Net.AS_{}"), InFeature, InSuffix);
    }

    // ---- Block formatting ---------------------------------------------

    // Replicated adds the server-side NetSubject spawn plus post-spawn settle ticks; Independent
    // skips both. Everything else — including the harness' log-suppression bracketing — is shared.
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
            // Embedded as a literal and resolved at run time via TryLoadClass — the same lazy
            // resolve the wrapper generator uses, so a per-test subject subclass needs no C++ dep.
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

        // The emitted `FString{...}` initializer collides with fmt substitution syntax, hence the
        // `{{` / `}}` escaping around `kAsClassPath_<Suffix>`.
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

    // Empty when CkTests isn't enabled.
    auto Get_CkTestsOutputDir() -> FString
    {
        if (const auto CkTestsPlugin = IPluginManager::Get().FindPlugin(TEXT("CkTests")))
        {
            return CkTestsPlugin->GetBaseDir() / TEXT("Source") / TEXT("CkTests")
                 / TEXT("Private") / TEXT("Net") / TEXT("Generated");
        }
        return {};
    }

    // Empty when there is no `Source/<ProjectName>` dir to emit into. Refusing to guess is
    // deliberate: falling back to the CkTests submodule is the cross-repo churn the split prevents.
    auto Get_ProjectOutputDir() -> FString
    {
        const auto ProjectModuleDir = FPaths::GameSourceDir() / FApp::GetProjectName();
        if (NOT IFileManager::Get().DirectoryExists(*ProjectModuleDir))
        { return {}; }

        return ProjectModuleDir / TEXT("Tests") / TEXT("Net") / TEXT("Generated");
    }

    // Mirrors FCkAutoTestWrapperGenerator::Find_PluginByPathPrefix verbatim — the engine fork
    // offers no path->plugin lookup, so longest-base-dir-prefix is the in-repo way to resolve one.
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

    // `<Plugin>/Source/<Module>/Private/Net/Generated`, mirroring CkTests' own layout. Empty on
    // every case the caller should route to CkTests instead. The hosting module MUST publicly
    // depend on CkTests — the emitted C++ includes the `CkTests/Net/*` harness headers.
    auto Get_PluginOutputDir(const FString& InSourcePath) -> FString
    {
        const auto* Plugin = Find_PluginByPathPrefix(InSourcePath);
        if (Plugin == nullptr)
        { return {}; }

        // CkTests routes through its own dedicated root instead, keeping that path untouched.
        if (Plugin->GetName() == TEXT("CkTests"))
        { return {}; }

        // Runtime first (where CkTests hosts its own), then UncookedOnly / DeveloperTool.
        // Editor-typed modules are skipped: the bodies are WITH_DEV_AUTOMATION_TESTS-guarded but
        // the file must still compile in the editor target's build.
        auto HostModuleName = FString{};
        const auto& Modules = Plugin->GetDescriptor().Modules;
        for (const auto& Module : Modules)
        {
            if (Module.Type == EHostType::Runtime)
            { HostModuleName = Module.Name.ToString(); break; }
        }
        if (HostModuleName.IsEmpty())
        {
            for (const auto& Module : Modules)
            {
                if (Module.Type == EHostType::UncookedOnly || Module.Type == EHostType::DeveloperTool)
                { HostModuleName = Module.Name.ToString(); break; }
            }
        }
        if (HostModuleName.IsEmpty())
        { return {}; }

        const auto ModuleDir = Plugin->GetBaseDir() / TEXT("Source") / HostModuleName;
        if (NOT IFileManager::Get().DirectoryExists(*ModuleDir))
        { return {}; }

        return ModuleDir / TEXT("Private") / TEXT("Net") / TEXT("Generated");
    }

    // Buckets by (output root, feature); the root routing keeps a committed stub in the same repo
    // as its .as test. A class whose root is unavailable is skipped LOUDLY — never rerouted
    // cross-repo.
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

            auto OutputDir = FString{};
            if (IsProjectAuthored)
            {
                OutputDir = ProjectOutputDir;
            }
            else
            {
                const auto PluginOutputDir = Get_PluginOutputDir(SourcePath);
                OutputDir = PluginOutputDir.IsEmpty() ? CkTestsOutputDir : PluginOutputDir;
            }

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

    // Membership is by FULL path, so one combined expected set can serve every root.
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

    // Leaf-to-root so the nearest match wins (`Script/CkAttribute/Sub/foo.as` still buckets to
    // `Attribute`), starting at the file's PARENT because the feature must be a directory.
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

        // Project convention: `Script/Tests/<Feature>/`. Plugin trees never produce that pair, and
        // the branch above claims the ambiguous case first.
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

    // The trailing separator guards against sibling-prefix false positives (`.../ScriptExtra/`);
    // the comparison is case-insensitive because Windows paths mix casings freely.
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

    // Single-writer gate: a secondary instance must never write or prune these stubs.
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

    // A feature bucket vanishes when its last net test is deleted and MOVES when its source
    // relocates; a stale copy left behind duplicates the emitted C++ test class and trips UBT.
    // ZERO discovered classes skips the prune entirely: all-tests-deleted is indistinguishable
    // from a broken discovery pass, and deleting committed files on the latter is repo-wide
    // churn, whereas an orphaned stub only costs a phantom Session Frontend entry.
    if (AllSubclasses.Num() > 0)
    {
        auto ExpectedPaths = TSet<FString>{};
        auto OutputRoots   = TSet<FString>{};
        for (const auto& Bucket : Buckets)
        {
            ExpectedPaths.Add(FPaths::ConvertRelativePathToFull(Bucket.OutputFilePath));
            OutputRoots.Add(FPaths::ConvertRelativePathToFull(FPaths::GetPath(Bucket.OutputFilePath)));
        }

        // The two fixed roots are pruned even when this pass wrote nothing to them, so a vanished
        // bucket still loses its orphan. Plugin roots deliberately appear only when a bucket
        // targeted them: never enumerate — and risk deleting in — a dir this pass knows nothing of.
        const auto CkTestsRoot = ck_autotest_netstub_generator::Get_CkTestsOutputDir();
        if (NOT CkTestsRoot.IsEmpty())
        { OutputRoots.Add(FPaths::ConvertRelativePathToFull(CkTestsRoot)); }

        const auto ProjectRoot = ck_autotest_netstub_generator::Get_ProjectOutputDir();
        if (NOT ProjectRoot.IsEmpty())
        { OutputRoots.Add(FPaths::ConvertRelativePathToFull(ProjectRoot)); }

        for (const auto& Root : OutputRoots)
        { ck_autotest_netstub_generator::Prune_StaleStubs(Root, ExpectedPaths); }
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

        // Diff-skip LF-normalized: writing unchanged content re-triggers UBT's source-tree scan ->
        // AS PostCompile -> GenerateAll, ad infinitum.
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
