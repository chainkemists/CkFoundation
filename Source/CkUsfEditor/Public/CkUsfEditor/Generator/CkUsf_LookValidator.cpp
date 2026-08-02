#include "CkUsfEditor/Generator/CkUsf_LookValidator.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"

#include "CkCore/Macros/CkMacros.h"

#include "Misc/FileHelper.h"
#include "ShaderCore.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_usf_look_validator
{
    // KEEP IN SYNC with Build_CustomCode / Get_ExtraOutputs in CkUsf_Generator.cpp.
    static const TCHAR* kReservedParamNames[] =
    {
        // generated locals
        TEXT("In"), TEXT("O"),
        TEXT("Time"), TEXT("UV"), TEXT("UV1"), TEXT("UV2"), TEXT("LocalPosition"),
        TEXT("WorldPosition"), TEXT("CameraVector"), TEXT("VertexNormal"),
        TEXT("VertexTangent"), TEXT("PixelDepth"), TEXT("VertexColor"),
        TEXT("SceneColor"), TEXT("SceneDepth"), TEXT("SceneNormal"), TEXT("CustomDepth"), TEXT("CustomStencil"),
        TEXT("ParticleColor"), TEXT("ParticleAlpha"),
        TEXT("DynParam0"), TEXT("DynParam1"), TEXT("DynParam2"), TEXT("DynParam3"),
        TEXT("EmissiveColor"), TEXT("Roughness"), TEXT("Metallic"), TEXT("Specular"),
        TEXT("AmbientOcclusion"), TEXT("Normal"), TEXT("Opacity"), TEXT("OpacityMask"),
        TEXT("Refraction"), TEXT("SubsurfaceColor"), TEXT("ClearCoat"), TEXT("ClearCoatRoughness"),
    };

    // Lowercase set — HLSL is case-sensitive, and so is the comparison against it.
    static const TCHAR* kHlslKeywords[] =
    {
        TEXT("bool"), TEXT("break"), TEXT("case"), TEXT("centroid"), TEXT("const"), TEXT("continue"),
        TEXT("default"), TEXT("discard"), TEXT("do"), TEXT("double"), TEXT("else"), TEXT("false"),
        TEXT("float"), TEXT("float2"), TEXT("float3"), TEXT("float4"), TEXT("for"), TEXT("half"),
        TEXT("if"), TEXT("in"), TEXT("inline"), TEXT("inout"), TEXT("int"), TEXT("linear"),
        TEXT("matrix"), TEXT("out"), TEXT("register"), TEXT("return"), TEXT("sampler"),
        TEXT("static"), TEXT("struct"), TEXT("switch"), TEXT("true"), TEXT("uint"),
        TEXT("uniform"), TEXT("vector"), TEXT("void"), TEXT("while"),
    };

    static auto Get_IsValidHlslIdentifier(const FString& InName) -> bool
    {
        if (InName.IsEmpty())
        { return false; }

        const auto IsIdentChar = [](TCHAR C, bool InFirst) -> bool
        {
            return FChar::IsAlpha(C) || C == TEXT('_') || (NOT InFirst && FChar::IsDigit(C));
        };

        for (auto Index = 0; Index < InName.Len(); ++Index)
        {
            if (NOT IsIdentChar(InName[Index], Index == 0))
            { return false; }
        }
        return true;
    }

    struct FHlslParam
    {
        FString Type;
        FString Name;
    };

    // KEEP IN SYNC with the positional argument assembly in CkUsf_Generator.cpp.
    static auto Build_ExpectedParams(const UCkUsf_LookDefinition* InDef) -> TArray<FHlslParam>
    {
        TArray<FHlslParam> Expected;
        for (const auto& Param : InDef->_Parameters)
        {
            const auto Name = Param._Name.ToString();
            switch (Param._Type)
            {
                case ECk_Usf_ParamType::Scalar:
                    Expected.Add({TEXT("float"), Name});
                    break;
                case ECk_Usf_ParamType::Vector:
                    Expected.Add({TEXT("float3"), Name});
                    break;
                case ECk_Usf_ParamType::Texture2D:
                    Expected.Add({TEXT("Texture2D"), Name});
                    Expected.Add({TEXT("SamplerState"), Name + TEXT("Sampler")});
                    break;
                case ECk_Usf_ParamType::TextureCube:
                    Expected.Add({TEXT("TextureCube"), Name});
                    Expected.Add({TEXT("SamplerState"), Name + TEXT("Sampler")});
                    break;
                case ECk_Usf_ParamType::Texture2DArray:
                    Expected.Add({TEXT("Texture2DArray"), Name});
                    Expected.Add({TEXT("SamplerState"), Name + TEXT("Sampler")});
                    break;
            }
        }
        return Expected;
    }

    static auto Render_Signature(const FString& InReturnType, const FString& InFnName,
        const FString& InFirstParam, const TArray<FHlslParam>& InParams) -> FString
    {
        FString Rendered = FString::Printf(TEXT("%s %s(%s"), *InReturnType, *InFnName, *InFirstParam);
        for (const auto& Param : InParams)
        { Rendered += FString::Printf(TEXT(", %s %s"), *Param.Type, *Param.Name); }
        return Rendered + TEXT(")");
    }

    static auto Strip_Comments(const FString& InSource) -> FString
    {
        FString Out;
        Out.Reserve(InSource.Len());
        auto InLine = false;
        auto InBlock = false;
        for (auto Index = 0; Index < InSource.Len(); ++Index)
        {
            const auto C = InSource[Index];
            const auto Next = Index + 1 < InSource.Len() ? InSource[Index + 1] : TEXT('\0');
            if (InLine)
            {
                if (C == TEXT('\n')) { InLine = false; Out.AppendChar(C); }
                continue;
            }
            if (InBlock)
            {
                if (C == TEXT('*') && Next == TEXT('/')) { InBlock = false; ++Index; }
                continue;
            }
            if (C == TEXT('/') && Next == TEXT('/')) { InLine = true; ++Index; continue; }
            if (C == TEXT('/') && Next == TEXT('*')) { InBlock = true; ++Index; continue; }
            Out.AppendChar(C);
        }
        return Out;
    }

    // OutNameAppears separates "wrong signature" from "not found" for the caller's error wording.
    static auto Find_FunctionParamList(const FString& InStripped, const FString& InFnName,
        const FString& InExpectedReturnType, FString& OutParamList, bool& OutNameAppears) -> bool
    {
        OutNameAppears = false;

        auto SearchFrom = 0;
        while (true)
        {
            const auto NameAt = InStripped.Find(InFnName, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
            if (NameAt == INDEX_NONE)
            { return false; }
            SearchFrom = NameAt + InFnName.Len();

            const auto Before = NameAt > 0 ? InStripped[NameAt - 1] : TEXT(' ');
            const auto After = NameAt + InFnName.Len() < InStripped.Len() ? InStripped[NameAt + InFnName.Len()] : TEXT(' ');
            const auto IsPartOfALongerIdentifier =
                FChar::IsAlnum(Before) || Before == TEXT('_') || FChar::IsAlnum(After) || After == TEXT('_');
            if (IsPartOfALongerIdentifier)
            { continue; }

            OutNameAppears = true;

            auto OpenAt = NameAt + InFnName.Len();
            while (OpenAt < InStripped.Len() && FChar::IsWhitespace(InStripped[OpenAt])) { ++OpenAt; }
            if (OpenAt >= InStripped.Len() || InStripped[OpenAt] != TEXT('('))
            { continue; }

            // Rejects call sites: only a declaration/definition has the return type as its previous token.
            auto TokenEnd = NameAt;
            while (TokenEnd > 0 && FChar::IsWhitespace(InStripped[TokenEnd - 1])) { --TokenEnd; }
            auto TokenStart = TokenEnd;
            while (TokenStart > 0 && (FChar::IsAlnum(InStripped[TokenStart - 1]) || InStripped[TokenStart - 1] == TEXT('_'))) { --TokenStart; }
            if (InStripped.Mid(TokenStart, TokenEnd - TokenStart) != InExpectedReturnType)
            { continue; }

            auto Depth = 0;
            for (auto Index = OpenAt; Index < InStripped.Len(); ++Index)
            {
                if (InStripped[Index] == TEXT('(')) { ++Depth; }
                else if (InStripped[Index] == TEXT(')'))
                {
                    --Depth;
                    if (Depth == 0)
                    {
                        OutParamList = InStripped.Mid(OpenAt + 1, Index - OpenAt - 1);
                        return true;
                    }
                }
            }
            return false;   // unbalanced parens — treat as not found
        }
    }

    static auto Parse_ParamList(const FString& InParamList) -> TArray<FHlslParam>
    {
        TArray<FString> Entries;
        InParamList.ParseIntoArray(Entries, TEXT(","), /*CullEmpty=*/ true);

        TArray<FHlslParam> Params;
        for (auto& Entry : Entries)
        {
            TArray<FString> Tokens;
            Entry.TrimStartAndEndInline();
            Entry.ParseIntoArrayWS(Tokens);
            if (Tokens.IsEmpty())
            { continue; }

            FHlslParam Param;
            Param.Name = Tokens.Last();
            Tokens.RemoveAt(Tokens.Num() - 1);
            Param.Type = FString::Join(Tokens, TEXT(" "));
            Params.Add(Param);
        }
        return Params;
    }

    static auto Compare_Signature(const FName InLookName, const FString& InFnName,
        const FString& InFirstParamType, const FString& InReturnType,
        const TArray<FHlslParam>& InExpected, const FString& InFoundParamList,
        ck::usf_editor::FLookValidationResult& InOutResult) -> void
    {
        auto Found = Parse_ParamList(InFoundParamList);

        if (Found.IsEmpty() || NOT Found[0].Type.Contains(InFirstParamType))
        {
            InOutResult.Errors.Add(FString::Printf(
                TEXT("Look [%s]: [%s]'s first parameter must be `%s In` — expected signature: %s"),
                *InLookName.ToString(), *InFnName, *InFirstParamType,
                *Render_Signature(InReturnType, InFnName, InFirstParamType + TEXT(" In"), InExpected)));
            return;
        }
        Found.RemoveAt(0);

        if (Found.Num() != InExpected.Num())
        {
            InOutResult.Errors.Add(FString::Printf(
                TEXT("Look [%s]: [%s] takes %d parameter(s) after In but the asset declares %d — expected signature: %s"),
                *InLookName.ToString(), *InFnName, Found.Num(), InExpected.Num(),
                *Render_Signature(InReturnType, InFnName, InFirstParamType + TEXT(" In"), InExpected)));
            return;
        }

        for (auto Index = 0; Index < InExpected.Num(); ++Index)
        {
            if (Found[Index].Type != InExpected[Index].Type)
            {
                InOutResult.Errors.Add(FString::Printf(
                    TEXT("Look [%s]: [%s] parameter %d is [%s %s] but the asset declares [%s %s] — wiring is positional, this would mis-bind"),
                    *InLookName.ToString(), *InFnName, Index + 1,
                    *Found[Index].Type, *Found[Index].Name,
                    *InExpected[Index].Type, *InExpected[Index].Name));
                continue;
            }
            if (Found[Index].Name != InExpected[Index].Name)
            {
                InOutResult.Warnings.Add(FString::Printf(
                    TEXT("Look [%s]: [%s] parameter %d is named [%s] in HLSL but [%s] in the asset — works (positional), but rename one to match"),
                    *InLookName.ToString(), *InFnName, Index + 1,
                    *Found[Index].Name, *InExpected[Index].Name));
            }
        }
    }

    static auto Validate_EntryPoint(const FName InLookName, const FString& InStripped,
        const FString& InFnName, const FString& InReturnType, const FString& InFirstParamType,
        const TArray<FHlslParam>& InExpected, const FString& InIncludePath,
        ck::usf_editor::FLookValidationResult& InOutResult) -> void
    {
        FString ParamList;
        auto NameAppears = false;
        if (NOT Find_FunctionParamList(InStripped, InFnName, InReturnType, ParamList, NameAppears))
        {
            InOutResult.Errors.Add(NameAppears
                ? FString::Printf(
                    TEXT("Look [%s]: [%s] appears in [%s] but not as `%s %s(...)` — wrong return type, or generated via macro (the entry point must be a plain definition)"),
                    *InLookName.ToString(), *InFnName, *InIncludePath, *InReturnType, *InFnName)
                : FString::Printf(
                    TEXT("Look [%s]: entry point [%s] not found in [%s] (it must be defined directly in that file) — expected signature: %s"),
                    *InLookName.ToString(), *InFnName, *InIncludePath,
                    *Render_Signature(InReturnType, InFnName, InFirstParamType + TEXT(" In"), InExpected)));
            return;
        }

        Compare_Signature(InLookName, InFnName, InFirstParamType, InReturnType, InExpected, ParamList, InOutResult);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::usf_editor
{
    auto Validate_LookDefinition(const UCkUsf_LookDefinition* InDef) -> FLookValidationResult
    {
        using namespace ck_usf_look_validator;

        FLookValidationResult Result;

        if (InDef == nullptr)
        {
            Result.Errors.Add(TEXT("Null LookDefinition"));
            return Result;
        }

        const auto LookName = InDef->Get_EffectiveLookName();

        if (InDef->_UshFunctionName.IsNone() || InDef->_UshIncludePath.IsEmpty())
        {
            Result.Errors.Add(FString::Printf(
                TEXT("Look [%s]: _UshFunctionName/_UshIncludePath not set"), *LookName.ToString()));
            return Result;
        }

        // ---- 1+2: parameter descriptor sanity (cheap, no file IO) ----
        TSet<FName> SeenNames;
        for (const auto& Param : InDef->_Parameters)
        {
            const auto Name = Param._Name.ToString();

            if (NOT Get_IsValidHlslIdentifier(Name))
            {
                Result.Errors.Add(FString::Printf(
                    TEXT("Look [%s]: param [%s] is not a valid HLSL identifier"), *LookName.ToString(), *Name));
                continue;
            }

            if (SeenNames.Contains(Param._Name))
            {
                Result.Errors.Add(FString::Printf(
                    TEXT("Look [%s]: duplicate param name [%s] (material parameter names are case-insensitive)"),
                    *LookName.ToString(), *Name));
            }
            SeenNames.Add(Param._Name);

            for (const auto* Reserved : kReservedParamNames)
            {
                if (Param._Name == FName{Reserved})
                {
                    Result.Errors.Add(FString::Printf(
                        TEXT("Look [%s]: param [%s] collides with the generated Custom-node input/output/local of the same name — rename it"),
                        *LookName.ToString(), *Name));
                    break;
                }
            }

            for (const auto* Keyword : kHlslKeywords)
            {
                if (Name.Equals(Keyword, ESearchCase::CaseSensitive))
                {
                    Result.Errors.Add(FString::Printf(
                        TEXT("Look [%s]: param [%s] is an HLSL keyword"), *LookName.ToString(), *Name));
                    break;
                }
            }

            if (Param._PerInstance
                && Param._Type != ECk_Usf_ParamType::Scalar
                && Param._Type != ECk_Usf_ParamType::Vector)
            {
                Result.Errors.Add(FString::Printf(
                    TEXT("Look [%s]: param [%s] is _PerInstance but not Scalar/Vector — textures cannot ride per-instance custom data"),
                    *LookName.ToString(), *Name));
            }
        }

        const auto IsSurfaceDomain = InDef->_Domain == ECk_Usf_Domain::SurfaceLit
                                  || InDef->_Domain == ECk_Usf_Domain::SurfaceUnlit;

        if (InDef->Get_NumPerInstanceFloats() > 0 && NOT IsSurfaceDomain)
        {
            Result.Warnings.Add(FString::Printf(
                TEXT("Look [%s]: has _PerInstance params but a non-surface domain — there are no instances to source them from"),
                *LookName.ToString()));
        }

        // Particle inputs are wired on the SURFACE branch of the generator only; asking for them anywhere
        // else silently leaves In.ParticleColor / In.DynamicParameter at their inert defaults.
        if ((InDef->_ParticleColor || InDef->_ParticleDynamicParameter) && NOT IsSurfaceDomain)
        {
            Result.Errors.Add(FString::Printf(
                TEXT("Look [%s]: _ParticleColor/_ParticleDynamicParameter require a Surface domain — the generator wires them nowhere else"),
                *LookName.ToString()));
        }

        if ((InDef->_UsedWithNiagaraSprites || InDef->_UsedWithNiagaraMeshParticles || InDef->_UsedWithNiagaraRibbons)
            && NOT IsSurfaceDomain)
        {
            Result.Warnings.Add(FString::Printf(
                TEXT("Look [%s]: a Niagara usage flag is set but the domain is not Surface — a particle renderer cannot use this master"),
                *LookName.ToString()));
        }

        // A particle look whose master declares NO renderer usage renders as the DEFAULT material in a packaged
        // build — the exact silent-fallback these flags exist to prevent.
        if ((InDef->_ParticleColor || InDef->_ParticleDynamicParameter)
            && NOT InDef->_UsedWithNiagaraSprites && NOT InDef->_UsedWithNiagaraMeshParticles
            && NOT InDef->_UsedWithNiagaraRibbons)
        {
            Result.Warnings.Add(FString::Printf(
                TEXT("Look [%s]: reads particle inputs but declares none of _UsedWithNiagaraSprites / _UsedWithNiagaraMeshParticles / _UsedWithNiagaraRibbons — a Niagara renderer would fall back to the default material"),
                *LookName.ToString()));
        }

        if (InDef->_ParticleDynamicParameterNames.Num() > 4)
        {
            Result.Warnings.Add(FString::Printf(
                TEXT("Look [%s]: _ParticleDynamicParameterNames has %d entries — only the first 4 are used"),
                *LookName.ToString(), InDef->_ParticleDynamicParameterNames.Num()));
        }

        if (InDef->_SceneTextures.Num() > 0)
        {
            if (InDef->_Domain != ECk_Usf_Domain::PostProcess)
            {
                Result.Warnings.Add(FString::Printf(
                    TEXT("Look [%s]: _SceneTextures is set but the domain is not PostProcess — it is ignored"),
                    *LookName.ToString()));
            }

            TSet<ECk_Usf_SceneTexture> SeenTextures;
            for (const auto SceneTexture : InDef->_SceneTextures)
            {
                if (SeenTextures.Contains(SceneTexture))
                {
                    Result.Errors.Add(FString::Printf(
                        TEXT("Look [%s]: duplicate _SceneTextures entry — it would wire two Custom-node inputs of the same name"),
                        *LookName.ToString()));
                    break;
                }
                SeenTextures.Add(SceneTexture);
            }
        }

        if (NOT Result.Get_IsValid())
        { return Result; }

        // ---- 3: resolve the include through the registered shader source mappings ----
        const auto DiskPath = GetShaderSourceFilePath(InDef->_UshIncludePath);
        FString Source;
        if (DiskPath.IsEmpty() || NOT FFileHelper::LoadFileToString(Source, *DiskPath))
        {
            Result.Errors.Add(FString::Printf(
                TEXT("Look [%s]: _UshIncludePath [%s] does not resolve to a readable file — check the virtual path and the module's AddShaderSourceDirectoryMapping"),
                *LookName.ToString(), *InDef->_UshIncludePath));
            return Result;
        }

        // ---- 4+5: entry-point signatures ----
        const auto Stripped = Strip_Comments(Source);
        const auto Expected = Build_ExpectedParams(InDef);

        Validate_EntryPoint(LookName, Stripped, InDef->_UshFunctionName.ToString(),
            TEXT("FCkUsf_SurfaceOutput"), TEXT("FCkUsf_SurfaceInput"), Expected,
            InDef->_UshIncludePath, Result);

        if (NOT InDef->_WpoFunctionName.IsNone())
        {
            if (IsSurfaceDomain)
            {
                Validate_EntryPoint(LookName, Stripped, InDef->_WpoFunctionName.ToString(),
                    TEXT("float3"), TEXT("FCkUsf_VertexInput"), Expected,
                    InDef->_UshIncludePath, Result);
            }
            else
            {
                Result.Warnings.Add(FString::Printf(
                    TEXT("Look [%s]: _WpoFunctionName is set but the domain is not Surface — WPO is ignored"),
                    *LookName.ToString()));
            }
        }

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
