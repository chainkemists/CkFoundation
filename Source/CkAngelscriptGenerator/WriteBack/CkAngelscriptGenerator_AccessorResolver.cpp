#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AccessorResolver.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::write_back
{
    namespace ck_angelscript_generator_accessor_resolver
    {
        const auto SoftPathPrefix  = FString{TEXT("FSoftObjectPath(\"")};
        const auto ClassSuffix     = FString{TEXT("_C")};
        const auto ClassFnSuffix   = FString{TEXT("_Class")};
        const auto NamespaceKeyword = FString{TEXT("namespace ")};

        auto Is_IdentChar(
            TCHAR InChar) -> bool
        {
            return FChar::IsAlnum(InChar) || InChar == TEXT('_');
        }

        // `assets` / `assets::load` — the emitter writes nothing else.
        auto Try_ParseNamespaceDeclaration(
            const FString& InTrimmedLine,
            FString&       OutNamespace) -> bool
        {
            if (NOT InTrimmedLine.StartsWith(NamespaceKeyword, ESearchCase::CaseSensitive))
            { return false; }

            auto Name = InTrimmedLine.RightChop(NamespaceKeyword.Len()).TrimStartAndEnd();

            if (auto BraceIndex = Name.Find(TEXT("{"), ESearchCase::CaseSensitive);
                BraceIndex != INDEX_NONE)
            { Name = Name.Left(BraceIndex).TrimEnd(); }

            if (Name.IsEmpty())
            { return false; }

            OutNamespace = MoveTemp(Name);
            return true;
        }

        // The emitter writes exactly `#if Editor`; hand-authored sources spell it `#if EDITOR` or
        // `#if editor`, so the token compare is case-insensitive. Any other `#if` still pushes onto
        // the stack so its `#endif` pairs correctly instead of closing an editor guard.
        auto Try_ParsePreprocessorLine(
            const FString& InTrimmedLine,
            bool&          OutIsEditorGuard,
            bool&          OutIsOpen,
            bool&          OutIsClose,
            bool&          OutIsInvert) -> bool
        {
            OutIsEditorGuard = false;
            OutIsOpen        = false;
            OutIsClose       = false;
            OutIsInvert      = false;

            if (NOT InTrimmedLine.StartsWith(TEXT("#"), ESearchCase::CaseSensitive))
            { return false; }

            const auto Directive = InTrimmedLine.RightChop(1).TrimStart();

            if (Directive.StartsWith(TEXT("endif"), ESearchCase::CaseSensitive))
            { OutIsClose = true; return true; }

            if (Directive.StartsWith(TEXT("else"), ESearchCase::CaseSensitive))
            { OutIsInvert = true; return true; }

            if (Directive.StartsWith(TEXT("if"), ESearchCase::CaseSensitive))
            {
                OutIsOpen = true;
                const auto Condition = Directive.RightChop(2).TrimStartAndEnd();
                OutIsEditorGuard = Condition.Equals(TEXT("Editor"), ESearchCase::IgnoreCase);
                return true;
            }

            return false;
        }

        // The function name is the identifier immediately before the first `()` on the line — the
        // same rule the subsystem's own seeder uses.
        auto Try_ExtractFunctionName(
            const FString& InLine,
            int32          InBeforeIndex,
            FString&       OutFunctionName) -> bool
        {
            const auto Head = InLine.Left(InBeforeIndex);

            const auto ParenIndex = Head.Find(TEXT("()"), ESearchCase::CaseSensitive);
            if (ParenIndex == INDEX_NONE)
            { return false; }

            auto NameStart = ParenIndex;
            while (NameStart > 0 && Is_IdentChar(Head[NameStart - 1]))
            { --NameStart; }

            if (NameStart >= ParenIndex)
            { return false; }

            OutFunctionName = Head.Mid(NameStart, ParenIndex - NameStart);
            return true;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAccessorResolver::
        Strip_ClassPathSuffix(
            const FString& InObjectPath)
        -> FString
    {
        namespace detail = ck_angelscript_generator_accessor_resolver;

        return InObjectPath.EndsWith(detail::ClassSuffix, ESearchCase::CaseSensitive)
            ? InObjectPath.LeftChop(detail::ClassSuffix.Len())
            : InObjectPath;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAccessorResolver::
        Parse_GeneratedAccessorFile(
            const FCk_GeneratedAccessorFile& InFile)
        -> TArray<FCk_ScriptAccessorEntry>
    {
        namespace detail = ck_angelscript_generator_accessor_resolver;

        auto Lines = TArray<FString>{};
        InFile.Contents.ParseIntoArrayLines(Lines, /*InCullEmpty=*/false);

        auto CurrentNamespace = InFile.FallbackNamespace;
        auto GuardStack       = TArray<bool>{};

        auto Entries          = TArray<FCk_ScriptAccessorEntry>{};
        auto IndexByPath      = TMap<FString, int32>{};
        auto ClassAccessorFor = TSet<FString>{};

        for (const auto& Line : Lines)
        {
            const auto Trimmed = Line.TrimStart();

            auto IsEditorGuard = false;
            auto IsOpen        = false;
            auto IsClose       = false;
            auto IsInvert      = false;
            if (detail::Try_ParsePreprocessorLine(Trimmed, IsEditorGuard, IsOpen, IsClose, IsInvert))
            {
                if (IsOpen)
                { GuardStack.Add(IsEditorGuard); }
                else if (IsClose && NOT GuardStack.IsEmpty())
                { GuardStack.Pop(); }
                else if (IsInvert && NOT GuardStack.IsEmpty())
                { GuardStack.Last() = NOT GuardStack.Last(); }
                continue;
            }

            if (auto Declared = FString{};
                detail::Try_ParseNamespaceDeclaration(Trimmed, Declared))
            {
                CurrentNamespace = MoveTemp(Declared);
                continue;
            }

            const auto PathStart = Line.Find(detail::SoftPathPrefix, ESearchCase::CaseSensitive);
            if (PathStart == INDEX_NONE)
            { continue; }

            const auto ValueStart = PathStart + detail::SoftPathPrefix.Len();
            const auto ValueEnd   = Line.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, ValueStart);
            if (ValueEnd == INDEX_NONE)
            { continue; }

            const auto ObjectPath = Line.Mid(ValueStart, ValueEnd - ValueStart);
            if (ObjectPath.IsEmpty())
            { continue; }

            auto FunctionName = FString{};
            if (NOT detail::Try_ExtractFunctionName(Line, PathStart, FunctionName))
            { continue; }

            const auto IsEditorOnly = GuardStack.Contains(true);

            // A `_C` path is the Blueprint class sibling of a base entry, never an entry of its own.
            if (ObjectPath.EndsWith(detail::ClassSuffix, ESearchCase::CaseSensitive)
                && FunctionName.EndsWith(detail::ClassFnSuffix, ESearchCase::CaseSensitive))
            {
                ClassAccessorFor.Add(Strip_ClassPathSuffix(ObjectPath));
                continue;
            }

            if (IndexByPath.Contains(ObjectPath))
            { continue; }

            auto Entry = FCk_ScriptAccessorEntry{};
            Entry.ObjectPath   = ObjectPath;
            Entry.Namespace    = CurrentNamespace;
            Entry.FunctionName = MoveTemp(FunctionName);
            Entry.SourceFile   = InFile.AbsolutePath;
            Entry.IsEditorOnly = IsEditorOnly;

            IndexByPath.Add(ObjectPath, Entries.Num());
            Entries.Add(MoveTemp(Entry));
        }

        for (auto& Entry : Entries)
        {
            if (ClassAccessorFor.Contains(Entry.ObjectPath))
            { Entry.HasClassAccessor = true; }
        }

        return Entries;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAccessorResolver::
        Build_Index(
            const TArray<FCk_ScriptAccessorEntry>& InEntries)
        -> TMap<FString, FCk_ScriptAccessorEntry>
    {
        auto Index = TMap<FString, FCk_ScriptAccessorEntry>{};
        Index.Reserve(InEntries.Num());

        for (const auto& Entry : InEntries)
        {
            if (Index.Contains(Entry.ObjectPath))
            { continue; }

            Index.Add(Entry.ObjectPath, Entry);
        }

        return Index;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAccessorResolver::
        Resolve(
            const TMap<FString, FCk_ScriptAccessorEntry>& InIndex,
            bool                                          InAnyProviderRegistered,
            const FString&                                InObjectPath,
            ECk_ScriptAccessorKind                        InKind,
            bool                                          InTargetBlockIsEditorOnly)
        -> FCk_AccessorResolveResult
    {
        auto Result = FCk_AccessorResolveResult{};

        if (NOT InAnyProviderRegistered)
        {
            Result.FailReason   = ECk_AccessorResolve_FailReason::NoProviderRegistered;
            Result.ErrorMessage = FString::Printf(
                TEXT("No AngelScript asset-reference provider is registered, so no accessor exists for any asset ")
                TEXT("(wanted one for '%s'). Generate the asset registry first."), *InObjectPath);
            return Result;
        }

        const auto WantsClass = InKind == ECk_ScriptAccessorKind::SoftClass
                             || InKind == ECk_ScriptAccessorKind::HardClass;

        const auto LookupPath = WantsClass ? Strip_ClassPathSuffix(InObjectPath) : InObjectPath;

        const auto* Entry = InIndex.Find(LookupPath);
        if (Entry == nullptr)
        {
            Result.FailReason   = ECk_AccessorResolve_FailReason::NoAccessorFound;
            Result.ErrorMessage = FString::Printf(
                TEXT("No generated `assets::` accessor references '%s'. It is outside every configured ")
                TEXT("discovery root, so no expression can reach it from AngelScript."), *LookupPath);
            return Result;
        }

        if (WantsClass && NOT Entry->HasClassAccessor)
        {
            Result.FailReason   = ECk_AccessorResolve_FailReason::NoClassAccessor;
            Result.ErrorMessage = FString::Printf(
                TEXT("'%s' has a soft-object accessor (`%s::%s()`) but no `_Class` sibling — the generator only ")
                TEXT("emits one for Blueprint assets."), *LookupPath, *Entry->Namespace, *Entry->FunctionName);
            return Result;
        }

        if (Entry->IsEditorOnly && NOT InTargetBlockIsEditorOnly)
        {
            Result.FailReason   = ECk_AccessorResolve_FailReason::EditorOnlyAccessorFromRuntimeBlock;
            Result.ErrorMessage = FString::Printf(
                TEXT("The accessor for '%s' is emitted inside `#if Editor`, but this asset block is not guarded — ")
                TEXT("writing it would break the cooked build. Wrap the asset in `#if EDITOR` first."), *LookupPath);
            return Result;
        }

        const auto FunctionSuffix = WantsClass
            ? ck_angelscript_generator_accessor_resolver::ClassFnSuffix
            : FString{};

        const auto WantsLoad = InKind == ECk_ScriptAccessorKind::HardObject
                            || InKind == ECk_ScriptAccessorKind::HardClass;

        Result.Success    = true;
        Result.Expression = FString::Printf(TEXT("%s::%s%s%s()"),
            *Entry->Namespace,
            WantsLoad ? TEXT("load::") : TEXT(""),
            *Entry->FunctionName,
            *FunctionSuffix);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
