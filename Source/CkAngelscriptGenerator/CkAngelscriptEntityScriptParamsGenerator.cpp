#include "CkAngelscriptEntityScriptParamsGenerator.h"

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_SharedUtils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript.h"

#include <Engine/BlueprintGeneratedClass.h>
#include <HAL/FileManager.h>
#include <Interfaces/IPluginManager.h>
#include <Misc/App.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <UObject/Class.h>
#include <UObject/UObjectIterator.h>
#include <UObject/UnrealType.h>

#if WITH_ANGELSCRIPT_CK
#include "ClassGenerator/ASClass.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck_entity_script_params_generator
{
#if WITH_EDITOR

    // ---- Filtering -----------------------------------------------------

    auto Is_IncludedEntityScriptClass(UClass* InClass) -> bool
    {
        if (ck::Is_NOT_Valid(InClass))
        { return false; }

        if (NOT InClass->IsChildOf(UCk_EntityScript_UE::StaticClass()))
        { return false; }

        if (InClass == UCk_EntityScript_UE::StaticClass())
        { return false; }

        // Blueprint-asset entity scripts are excluded — their params are BP-authored and their
        // visibility depends on per-process load state. CLASS_CompiledFromBlueprint cannot be
        // used here: the AS engine fork sets that flag on AngelScript classes too.
        if (ck::IsValid(Cast<UBlueprintGeneratedClass>(InClass)))
        { return false; }

        constexpr auto DisqualifyingFlags =
            CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;

        if (InClass->HasAnyClassFlags(DisqualifyingFlags))
        { return false; }

        const auto ClassIsTornDownPendingGc = InClass->IsUnreachable() ||
            InClass->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed);
        if (ClassIsTornDownPendingGc)
        { return false; }

#if WITH_ANGELSCRIPT_CK
        // A deleted .as FILE is a strong signal the class is gone; a class deleted out of a
        // still-present file would need a re-parse to detect, so that waits for editor restart.
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

    // ---- Block formatting ----------------------------------------------

    auto Is_ConstProperty(FProperty* InProperty) -> bool
    {
        return InProperty->HasAnyPropertyFlags(CPF_ConstParm | CPF_BlueprintReadOnly);
    }

    // A container whose ELEMENT type was weakened cannot be converted in an expression: AS has no
    // TArray<UObject> -> TArray<TWeakObjectPtr<UObject>> conversion, and a wrapper-style
    // `TArray<TWeakObjectPtr<T>>(InX)` does not compile. So the Params() parameter for a container
    // is declared as the RETAINED type and the ctor assigns it straight across, instead of the
    // scalar treatment (reflected parameter, ctor wraps). The cost is that a caller holding a
    // strong TArray<T> converts at the call site; the benefit is that the generated ctor stays one
    // assignment per field and needs no emitted loops.
    auto Is_ContainerProperty(const FProperty* InProperty) -> bool
    {
        return CastField<FArrayProperty>(InProperty) != nullptr
            || CastField<FSetProperty>(InProperty) != nullptr
            || CastField<FMapProperty>(InProperty) != nullptr
            || CastField<FOptionalProperty>(InProperty) != nullptr;
    }

    auto Format_ParameterType(FProperty* InProperty) -> FString
    {
        return Is_ContainerProperty(InProperty)
            ? FCkAngelscriptEntityScriptParamsGenerator::Get_RetainedPropertyType(InProperty)
            : FCkAngelscriptGenerator_SharedUtils::Get_DetailedPropertyType(InProperty);
    }

    auto Format_RetainedValueExpression(FProperty* InProperty, const FString& InSourceExpression) -> FString
    {
        // The parameter already arrives retained for a container — wrapping it again would emit a
        // conversion that does not exist.
        if (Is_ContainerProperty(InProperty))
        { return InSourceExpression; }

        const auto SourceType = FCkAngelscriptGenerator_SharedUtils::Get_DetailedPropertyType(InProperty);
        const auto RetainedType = FCkAngelscriptEntityScriptParamsGenerator::Get_RetainedPropertyType(InProperty);
        if (SourceType == RetainedType || InSourceExpression == TEXT("nullptr"))
        { return InSourceExpression; }

        return ck::Format_UE(TEXT("{}({})"), RetainedType, InSourceExpression);
    }

    // OverrideStatements (`<Name>.<Path> = <Value>;` lines for the SpawnParams default-ctor body)
    // are non-empty only for a struct-typed property whose CDO differs from struct default AND
    // whose struct holds a UObject* — an inline positional ctor would emit `<null handle>` there.
    struct FPropertyEmission
    {
        FString         DeclLine;
        TArray<FString> OverrideStatements;
    };

    auto Format_PropertyLine(FProperty* InProperty, const UClass* InClass) -> FPropertyEmission
    {
        auto AsType = FCkAngelscriptEntityScriptParamsGenerator::Get_RetainedPropertyType(InProperty);
        if (Is_ConstProperty(InProperty) && NOT AsType.StartsWith(TEXT("const ")))
        {
            AsType = TEXT("const ") + AsType;
        }
        const auto& PropName = InProperty->GetName();

        auto Emission = FPropertyEmission{};
        // CPF_Transient must survive into the generated struct: CkSnapshot's capture/remap walks THIS
        // struct's fields (not the script class's), and keys its "never persisted" opt-out off the walked
        // field's flags. Dropping the flag here re-persists the field — a Transient handle to boot infra
        // (e.g. RentalManager's DayCycle) then makes the whole spawn recipe unresolvable at load and the
        // entity is orphaned as [unresolved-other].
        const auto Specifiers = InProperty->HasAnyPropertyFlags(CPF_Transient) ? TEXT("Transient") : TEXT("");
        Emission.DeclLine = ck::Format_UE(TEXT("    UPROPERTY({})\n    {} {}"), Specifiers, AsType, PropName);

        const auto* CDO = InClass->GetDefaultObject();
        if (ck::Is_NOT_Valid(CDO))
        {
            Emission.DeclLine += TEXT(";");
            return Emission;
        }

        if (const auto* StructProp = CastField<FStructProperty>(InProperty))
        {
            if (auto* Struct = StructProp->Struct.Get();
                Struct != nullptr && UCk_Utils_Reflection_UE::Has_UObjectPointerField(Struct))
            {
                const auto Overrides = UCk_Utils_Reflection_UE::Get_StructFieldOverrides(StructProp, CDO);
                if (Overrides.Num() > 0)
                {
                    // No initializer: InitializeStruct zero-inits the field, and the
                    // default-ctor body applies the CDO diffs as dotted-path assignments.
                    Emission.DeclLine += TEXT(";");
                    for (const auto& Override : Overrides)
                    {
                        const auto Expr = UCk_Utils_Reflection_UE::Get_AngelscriptDefaultExpression(Override._Literal);
                        if (Expr.IsEmpty())
                        { continue; }
                        Emission.OverrideStatements.Add(
                            ck::Format_UE(TEXT("        {}.{} = {};"), PropName, Override._DottedFieldPath, Expr));
                    }
                    return Emission;
                }
            }
        }

        const auto Literal = UCk_Utils_Reflection_UE::Get_PropertyDefaultValueLiteral(InProperty, CDO);
        if (Literal.IsSet())
        {
            const auto DefaultExpr = UCk_Utils_Reflection_UE::Get_AngelscriptDefaultExpression(*Literal);
            if (NOT DefaultExpr.IsEmpty())
            {
                Emission.DeclLine += ck::Format_UE(
                    TEXT(" = {}"),
                    Format_RetainedValueExpression(InProperty, DefaultExpr));
            }
        }

        Emission.DeclLine += TEXT(";");
        return Emission;
    }

    auto Format_ParameterList(const TArray<FProperty*>& InProps) -> FString
    {
        auto Out = FString{};
        for (auto Index = int32{0}; Index < InProps.Num(); ++Index)
        {
            // Reflected (not retained) type for SCALARS: only the mirror FIELD is weak, and the ctor
            // converts — so the public Params(...) call shape stays source-compatible for callers.
            // Containers take the retained type (see Format_ParameterType).
            auto AsType = Format_ParameterType(InProps[Index]);
            if (Is_ConstProperty(InProps[Index]) && NOT AsType.StartsWith(TEXT("const ")))
            {
                AsType = TEXT("const ") + AsType;
            }
            const auto& PropName = InProps[Index]->GetName();
            Out += ck::Format_UE(TEXT("{} In{}"), AsType, PropName);
            if (Index + 1 < InProps.Num())
            { Out += TEXT(", "); }
        }
        return Out;
    }

    auto Format_ArgumentList(const TArray<FProperty*>& InProps) -> FString
    {
        auto Out = FString{};
        for (auto Index = int32{0}; Index < InProps.Num(); ++Index)
        {
            Out += ck::Format_UE(TEXT("In{}"), InProps[Index]->GetName());
            if (Index + 1 < InProps.Num())
            { Out += TEXT(", "); }
        }
        return Out;
    }

    auto Format_ClassBlock(UClass* InClass, int32& OutPropertyCount) -> FString
    {
        const auto AllProperties = UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass(InClass);
        const auto ValidProps = ck::algo::Filter(AllProperties, [](FProperty* InProp)
        {
            return ck::IsValid(InProp);
        });
        OutPropertyCount = ValidProps.Num();

        const auto ClassName = InClass->GetName();
        const auto FullClassName = ck::Format_UE(TEXT("{}{}"), InClass->GetPrefixCPP(), ClassName);
        const auto StructName = ck::Format_UE(TEXT("F{}_SpawnParams"), ClassName);

        auto Block = FString{};

        // Unique file-scope USTRUCT name — a shared `Params` name across namespaces trips
        // the Unreal naming convention check.
        Block += TEXT("USTRUCT()\n");
        Block += ck::Format_UE(TEXT("struct {}\n"), StructName);
        Block += TEXT("{\n");

        auto AllOverrideStatements = TArray<FString>{};

        for (auto Index = int32{0}; Index < ValidProps.Num(); ++Index)
        {
            auto Emission = Format_PropertyLine(ValidProps[Index], InClass);
            Block += Emission.DeclLine;
            Block += TEXT("\n");
            if (Index + 1 < ValidProps.Num())
            { Block += TEXT("\n"); }
            AllOverrideStatements.Append(MoveTemp(Emission.OverrideStatements));
        }

        // The zero-override case must keep relying on the implicit zero-arg ctor — emitting an
        // empty body would churn every existing generated file for no gain.
        if (AllOverrideStatements.Num() > 0)
        {
            Block += TEXT("\n");
            Block += ck::Format_UE(TEXT("    {}()\n"), StructName);
            Block += TEXT("    {\n");
            for (const auto& Statement : AllOverrideStatements)
            {
                Block += Statement;
                Block += TEXT("\n");
            }
            Block += TEXT("    }\n");
        }

        if (ValidProps.Num() > 0)
        {
            Block += TEXT("\n");
            Block += ck::Format_UE(TEXT("    {}({})\n"), StructName, Format_ParameterList(ValidProps));
            Block += TEXT("    {\n");
            for (auto* Prop : ValidProps)
            {
                const auto& PropName = Prop->GetName();
                const auto ValueExpression = Format_RetainedValueExpression(
                    Prop,
                    ck::Format_UE(TEXT("In{}"), PropName));
                Block += ck::Format_UE(TEXT("        {} = {};\n"), PropName, ValueExpression);
            }
            Block += TEXT("    }\n");
        }

        Block += TEXT("}\n\n");

        // Namespace mirrors the class name so callers keep writing `UCk_MyEntityScript::Params(...)`.
        Block += ck::Format_UE(TEXT("namespace {}\n"), FullClassName);
        Block += TEXT("{\n");

        Block += ck::Format_UE(TEXT("    {} Params()\n"), StructName);
        Block += TEXT("    {\n");
        Block += ck::Format_UE(TEXT("        return {}();\n"), StructName);
        Block += TEXT("    }\n");

        if (ValidProps.Num() > 0)
        {
            Block += TEXT("\n");
            Block += ck::Format_UE(TEXT("    {} Params({})\n"), StructName, Format_ParameterList(ValidProps));
            Block += TEXT("    {\n");
            Block += ck::Format_UE(TEXT("        return {}({});\n"), StructName, Format_ArgumentList(ValidProps));
            Block += TEXT("    }\n");
        }

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
                // Pick the longest match so nested plugin dirs win over ancestors.
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
        // AS-defined classes all live under /Script/Angelscript, so this only matches C++ classes.
        const auto PackageName = InClass->GetOutermost()->GetName();
        constexpr auto ScriptPrefix = TEXT("/Script/");
        if (PackageName.StartsWith(ScriptPrefix))
        {
            const auto ModuleName = PackageName.RightChop(FString(ScriptPrefix).Len());
            if (const auto* Found = InModuleToPlugin.Find(ModuleName))
            { return &Found->Get(); }
        }

#if WITH_ANGELSCRIPT_CK
        // GetFirstASClass walks the hierarchy; a plain Cast<UASClass> misses derived classes.
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
                Bucket.OutputFilePath = OutputDir / (PluginName + TEXT("_EntitySpawnParams.as"));
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
        TEXT("// Auto-generated EntityScript spawn-params — DO NOT EDIT.\n")
        TEXT("// This file is regenerated on editor startup and after every AngelScript recompile.\n")
        TEXT("//\n")
        TEXT("// For each UCk_EntityScript_UE subclass, two declarations are emitted:\n")
        TEXT("//   - FCk_MyEntityScript_SpawnParams  (file-scope USTRUCT, unique name — avoids the\n")
        TEXT("//     `Params` name-collision across namespaces that trips the Unreal naming check)\n")
        TEXT("//   - namespace UCk_MyEntityScript { FCk_MyEntityScript_SpawnParams Params() { ... } }\n")
        TEXT("//     so callers can still write `UCk_MyEntityScript::Params()`.\n")
        TEXT("//\n")
        TEXT("// Properties are flattened across the hierarchy (AS has no struct inheritance). Non-\n")
        TEXT("// trivial struct defaults outside the CkReflection_Utils allowlist are emitted without\n")
        TEXT("// an initializer — set them on the instance before calling Request_SpawnEntity.\n\n");

#endif // WITH_EDITOR
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_entity_script_params_generator
{
    // A strong (non-weak, non-soft) UObject leaf anywhere under InProperty — the shape the retained
    // params blob cannot legally hold, since UE's GC does not trace an FInstancedStruct.
    auto Get_ContainsStrongObjectLeaf(const FProperty* InProperty) -> bool
    {
        if (InProperty == nullptr)
        { return false; }

        if (CastField<FWeakObjectProperty>(InProperty) != nullptr
            || CastField<FSoftObjectProperty>(InProperty) != nullptr)
        { return false; }

        if (CastField<FObjectPropertyBase>(InProperty) != nullptr)
        { return true; }

        if (const auto* ArrayProperty = CastField<FArrayProperty>(InProperty))
        { return Get_ContainsStrongObjectLeaf(ArrayProperty->Inner); }

        if (const auto* SetProperty = CastField<FSetProperty>(InProperty))
        { return Get_ContainsStrongObjectLeaf(SetProperty->ElementProp); }

        if (const auto* MapProperty = CastField<FMapProperty>(InProperty))
        {
            return Get_ContainsStrongObjectLeaf(MapProperty->KeyProp)
                || Get_ContainsStrongObjectLeaf(MapProperty->ValueProp);
        }

        if (const auto* OptionalProperty = CastField<FOptionalProperty>(InProperty))
        { return Get_ContainsStrongObjectLeaf(OptionalProperty->GetValueProperty()); }

        return false;
    }

    // Counts emitted struct blocks — the unit that actually matters for "did this bucket lose
    // classes", and cheaper/steadier than re-deriving the class list from text.
    auto Get_StructBlockCount(const FString& InContent) -> int32
    {
        constexpr auto Needle = TEXT("\nstruct F");
        auto Count = int32{0};
        auto SearchFrom = int32{0};
        for (;;)
        {
            const auto Found = InContent.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
            if (Found == INDEX_NONE)
            { break; }

            ++Count;
            SearchFrom = Found + 1;
        }
        return Count;
    }

    // True only for the unrecoverable shape: the bucket is losing struct blocks AND AngelScript has
    // no registered class to have legitimately produced that loss.
    auto Get_ShouldRefuseTruncatingWrite(const FString& InExisting, const FString& InNew) -> bool
    {
#if WITH_ANGELSCRIPT_CK
        const auto IsShrinking = Get_StructBlockCount(InNew) < Get_StructBlockCount(InExisting);
        if (NOT IsShrinking)
        { return false; }

        for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
        {
            if (UASClass::GetFirstASClass(*ClassIterator) != nullptr)
            { return false; }
        }

        return true;
#else
        return false;
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptEntityScriptParamsGenerator::
    GenerateAll()
    -> void
{
#if WITH_EDITOR

    if (NOT FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(
            TEXT("ESPGenerator.GenerateAll")))
    { return; }

    ck::angelscriptgenerator::Log(TEXT("[CkAS ES Params] === Generating EntityScript Spawn Params ==="));

    auto Classes = TArray<UClass*>{};
    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        auto* Class = *ClassIterator;
        if (ck_entity_script_params_generator::Is_IncludedEntityScriptClass(Class))
        {
            Classes.Add(Class);
        }
    }

    Classes.Sort([](const UClass& InA, const UClass& InB)
    {
        return InA.GetPathName() < InB.GetPathName();
    });

    const auto Buckets = ck_entity_script_params_generator::Bucket_ClassesByPlugin(Classes);

    auto TotalClasses = int32{0};
    auto TotalProperties = int32{0};

    for (const auto& Bucket : Buckets)
    {
        auto Content = FString{ck_entity_script_params_generator::FileHeader};

        auto BucketProperties = int32{0};
        for (auto* Class : Bucket.Classes)
        {
            auto PropertyCount = int32{0};
            Content += ck_entity_script_params_generator::Format_ClassBlock(Class, PropertyCount);
            BucketProperties += PropertyCount;
        }

        const auto OutputDir = FPaths::GetPath(Bucket.OutputFilePath);
        IFileManager::Get().MakeDirectory(*OutputDir, true);

        // Writing unchanged content re-triggers the AS PostCompile hook that called us —
        // infinite loop. Compare LF-normalized: on-disk is CRLF (see the EOL step below).
        auto ExistingContent = FString{};
        const auto HasExisting = FFileHelper::LoadFileToString(ExistingContent, *Bucket.OutputFilePath);

        auto ContentForCompare = Content;
        ContentForCompare.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
        auto ExistingForCompare = ExistingContent;
        ExistingForCompare.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

        if (HasExisting && ExistingForCompare.Equals(ContentForCompare, ESearchCase::CaseSensitive))
        {
            ck::angelscriptgenerator::VeryVerbose(
                TEXT("[CkAS ES Params] [{}] up-to-date ({} classes)"),
                Bucket.PluginName, Bucket.Classes.Num());
            TotalClasses += Bucket.Classes.Num();
            TotalProperties += BucketProperties;
            continue;
        }

        // TRUNCATION GUARD. OnPostEngineInit runs this generator BEFORE AngelScript's first compile,
        // and again after a FAILED one — in both states no AS class is registered, so the
        // TObjectIterator sweep above sees only the C++ entity scripts and this bucket collapses to
        // that handful. Writing it destroys every AS-derived block the next compile needs to resolve
        // its callers, and the damage is self-sustaining: the compile then fails on the missing
        // structs, which keeps AS unregistered, which truncates again on the next boot. Venus hit
        // exactly this (98 structs -> 1) once a stale read-only bit stopped masking it.
        //
        // Refuse the write when the canonical shrinks WHILE no AngelScript class is visible. A
        // project whose entity scripts are genuinely all-C++ is unaffected (nothing shrinks, so the
        // guard never trips), and a real deletion still lands because the compile that performed it
        // leaves the surviving AS classes registered.
        if (HasExisting && ck_entity_script_params_generator::Get_ShouldRefuseTruncatingWrite(
                ExistingForCompare, ContentForCompare))
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[CkAS ES Params] [{}] REFUSED a truncating rewrite of [{}] — no AngelScript entity-script class is "
                     "registered, so this pass would drop the AS-derived blocks and wedge the next compile. AngelScript has "
                     "not compiled yet, or the last compile FAILED; the next successful compile regenerates this file."),
                Bucket.PluginName, Bucket.OutputFilePath);
            continue;
        }

        // Every rewrite the AS file-watcher sees costs a structural hot-reload plus a
        // soft-reload sweep after engine init (multi-second freeze) — so log exactly why.
        if (NOT HasExisting)
        {
            ck::angelscriptgenerator::Log(
                TEXT("[CkAS ES Params] [{}] rewrite reason: no existing file (or unreadable) at [{}]"),
                Bucket.PluginName, Bucket.OutputFilePath);
        }
        else
        {
            auto ExistingLines = TArray<FString>{};
            auto NewLines = TArray<FString>{};
            constexpr auto CullEmpty = false;
            ExistingForCompare.ParseIntoArray(ExistingLines, TEXT("\n"), CullEmpty);
            ContentForCompare.ParseIntoArray(NewLines, TEXT("\n"), CullEmpty);

            const auto CommonLineCount = FMath::Min(ExistingLines.Num(), NewLines.Num());
            auto FirstDiffLine = int32{INDEX_NONE};
            for (auto LineIndex = int32{0}; LineIndex < CommonLineCount; ++LineIndex)
            {
                if (NOT ExistingLines[LineIndex].Equals(NewLines[LineIndex], ESearchCase::CaseSensitive))
                {
                    FirstDiffLine = LineIndex;
                    break;
                }
            }

            if (FirstDiffLine != INDEX_NONE)
            {
                ck::angelscriptgenerator::Log(
                    TEXT("[CkAS ES Params] [{}] rewrite reason: first diff at line {} (old lines: {}, new lines: {})\n    old: [{}]\n    new: [{}]"),
                    Bucket.PluginName, FirstDiffLine + 1, ExistingLines.Num(), NewLines.Num(),
                    ExistingLines[FirstDiffLine], NewLines[FirstDiffLine]);
            }
            else
            {
                ck::angelscriptgenerator::Log(
                    TEXT("[CkAS ES Params] [{}] rewrite reason: line-count change only — old lines: {}, new lines: {}, first extra line: [{}]"),
                    Bucket.PluginName, ExistingLines.Num(), NewLines.Num(),
                    ExistingLines.Num() > NewLines.Num()
                        ? ExistingLines[CommonLineCount]
                        : NewLines[CommonLineCount]);
            }
        }

#if PLATFORM_WINDOWS
        // .as files live as CRLF on Windows disk (core.autocrlf). Writing LF makes git flag
        // the file modified on every editor startup.
        Content.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
        Content.ReplaceInline(TEXT("\n"), TEXT("\r\n"));
#endif

        if (FFileHelper::SaveStringToFile(Content, *Bucket.OutputFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {
            ck::angelscriptgenerator::Log(
                TEXT("[CkAS ES Params] [{}] {} classes, {} properties -> {}"),
                Bucket.PluginName, Bucket.Classes.Num(), BucketProperties, Bucket.OutputFilePath);
        }
        else
        {
            ck::angelscriptgenerator::Error(
                TEXT("[CkAS ES Params] Failed to write to [{}]"),
                Bucket.OutputFilePath);
        }

        TotalClasses += Bucket.Classes.Num();
        TotalProperties += BucketProperties;
    }

    ck::angelscriptgenerator::Log(
        TEXT("[CkAS ES Params] Done — {} plugins, {} classes, {} properties total"),
        Buckets.Num(), TotalClasses, TotalProperties);

#endif // WITH_EDITOR
}

auto
    FCkAngelscriptEntityScriptParamsGenerator::
    Is_IncludedEntityScriptClass(
        UClass* InClass)
    -> bool
{
#if WITH_EDITOR
    return ck_entity_script_params_generator::Is_IncludedEntityScriptClass(InClass);
#else
    return false;
#endif
}

auto
    FCkAngelscriptEntityScriptParamsGenerator::
    Get_RetainedPropertyType(
        FProperty* InProperty)
    -> FString
{
    const auto ReflectedType = FCkAngelscriptGenerator_SharedUtils::Get_DetailedPropertyType(InProperty);
    if (ck::Is_NOT_Valid(InProperty))
    { return ReflectedType; }

    // Weak/soft properties already carry an explicit retention policy, and they share
    // object-property ancestry — so they must be rejected before the strong-object branch.
    if (CastField<FWeakObjectProperty>(InProperty) != nullptr
        || CastField<FSoftObjectProperty>(InProperty) != nullptr)
    { return ReflectedType; }

    // A strong UObject leaf is exactly as untraced inside a container as it is at the top level,
    // and ck::Analyze_UntracedStructSafety walks into containers — so a raw TArray<UObject> field
    // is rejected at Request_SpawnEntity even though the equivalent scalar is weakened here and
    // accepted. Recurse so the leaf is weakened in place, mirroring Get_DetailedPropertyType's own
    // container walk (which is what produces the reflected spelling these must stay parallel to).
    if (const auto* ArrayProperty = CastField<FArrayProperty>(InProperty))
    { return ck::Format_UE(TEXT("TArray<{}>"), Get_RetainedPropertyType(ArrayProperty->Inner)); }

    // TArray is the ONLY container weakened automatically, and deliberately so: weakening is half a
    // contract, and the other half is UCk_Utils_EntityScript_UE::TryInjectEntityScriptSpawnParams
    // resolving the weak element back to a strong one when it writes into the script. That resolve
    // exists for scalars and for TArray. Rewriting a TSet/TMap/TOptional here without it would hand
    // the injector's generic CopyCompleteValue a weak-vs-strong element mismatch that byte-copies
    // cleanly and yields garbage pointers — silent corruption, strictly worse than the loud runtime
    // ensure the author gets today. Say so instead, and let them declare the container weak/soft.
    if (CastField<FSetProperty>(InProperty) != nullptr
        || CastField<FMapProperty>(InProperty) != nullptr
        || CastField<FOptionalProperty>(InProperty) != nullptr)
    {
        if (ck_entity_script_params_generator::Get_ContainsStrongObjectLeaf(InProperty))
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[CkAS ES Params] Property [{}] is a {} holding a STRONG UObject leaf. Retained spawn params are not "
                     "GC-traced, so this will be REJECTED at spawn time by the untraced-struct safety check. Only TArray is "
                     "weakened automatically — declare this container's element as TWeakObjectPtr<> or TSoftObjectPtr<> at "
                     "the source instead."),
                InProperty->GetName(), ReflectedType);
        }
        return ReflectedType;
    }

    const auto* ObjectProperty = CastField<FObjectPropertyBase>(InProperty);
    const auto* ObjectClass = ObjectProperty != nullptr ? ObjectProperty->PropertyClass.Get() : nullptr;
    if (ObjectClass == nullptr)
    { return ReflectedType; }

    const auto DirectObjectType = ck::Format_UE(
        TEXT("{}{}"),
        ObjectClass->GetPrefixCPP(),
        ObjectClass->GetName());
    // A const AS object handle must stay const through the weak wrapper, or the mirror widens
    // the binding and hands out a mutable Get() result.
    const auto RetainedObjectType = ReflectedType == TEXT("const ") + DirectObjectType
        ? TEXT("const ") + DirectObjectType
        : DirectObjectType;

    return ck::Format_UE(TEXT("TWeakObjectPtr<{}>"), RetainedObjectType);
}

// --------------------------------------------------------------------------------------------------------------------
