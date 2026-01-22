#pragma once

#include "CkEcs/Handle/CkHandle.h"

#if WITH_ANGELSCRIPT_CK

#include <AngelscriptBinds.h>

// --------------------------------------------------------------------------------------------------------------------

class UCkDynamic_HandleDefinition;

using FHandleTypeValidator = TFunction<bool(const FCk_Handle&)>;

// --------------------------------------------------------------------------------------------------------------------

struct CKDYNAMIC_API FCkDynamic_HandleTypeInfo
{
    FString TypeName;
    FString ShortName;
    TArray<FString> RequiredFragments;
    FString Description;
    FString SourceAsset;
    FHandleTypeValidator Validator;
};

// --------------------------------------------------------------------------------------------------------------------

class CKDYNAMIC_API FCkDynamic_HandleTypeRegistry
{
public:
    /**
     * Load handle type definitions from the JSON registry file.
     * Called during pre-compile to ensure types exist before AS compilation.
     */
    static auto LoadFromJsonRegistry() -> bool;

    /**
     * Get the path to the JSON registry file.
     */
    static auto GetRegistryFilePath() -> FString;

    static auto RegisterHandleTypeFromDefinition(
        const UCkDynamic_HandleDefinition* InDefinition) -> bool;

    static auto RegisterHandleType(
        const FString& InTypeName,
        const TArray<FString>& InRequiredFragments = {},
        const FString& InDescription = {},
        const FString& InSourceAsset = {}) -> bool;

    static auto RegisterHandleType(
        const FString& InTypeName,
        const FString& InValidatorFragmentName) -> bool;

    static auto RegisterHandleTypeWithValidator(
        const FString& InTypeName,
        const FHandleTypeValidator& InValidator) -> bool;

    static auto DiscoverAndRegisterAllDefinitions() -> void;

    static auto IsHandleTypeRegistered(
        const FString& InTypeName) -> bool;

    static auto GetHandleTypeInfo(
        const FString& InTypeName) -> const FCkDynamic_HandleTypeInfo*;

    static auto GetAllRegisteredTypes()
        -> const TMap<FString, TSharedPtr<FCkDynamic_HandleTypeInfo>>&;

    static auto ValidateHandle(
        const FString& InTypeName,
        const FCk_Handle& InHandle) -> bool;

    static auto EnsureCallbackRegistered() -> void;

private:
    static auto CreateAngelScriptBindings(
        const FString& InTypeName) -> void;

    static auto BindCrossHandleConversions() -> void;
    static auto BindConversionsToStaticHandles() -> void;

    static auto BindBaseMixinMethods() -> void;

    static auto Get_RegisteredTypes()
        -> TMap<FString, TSharedPtr<FCkDynamic_HandleTypeInfo>>&;

    static auto Get_PendingTypes() -> TArray<FCkDynamic_HandleTypeInfo>&;

    static auto Get_BoundConversionPairs() -> TSet<TPair<FString, FString>>&;

    static auto RegisterAllPendingTypes() -> void;

    static auto CreateMultiFragmentValidator(
        const TArray<FString>& InFragmentNames) -> FHandleTypeValidator;

    static auto FindScriptStructByName(
        const FString& InStructName) -> const UScriptStruct*;

    static auto ExtractShortName(
        const FString& InTypeName) -> FString;

private:
    static inline FDelegateHandle _PreCompileDelegateHandle;
    static inline bool _CrossConversionsBound = false;
    static inline bool _BaseMixinsBound = false;
    static inline bool _JsonRegistryLoaded = false;
};

// --------------------------------------------------------------------------------------------------------------------

#define CK_REGISTER_ANGELSCRIPT_DYNAMIC_HANDLE(_HandleTypeName_)                                      \
    static inline bool CK_CONCAT(AngelScriptDynamicHandle_, _HandleTypeName_) = []() -> bool          \
    {                                                                                                  \
        FCkDynamic_HandleTypeRegistry::RegisterHandleType(TEXT(#_HandleTypeName_));                   \
        return true;                                                                                   \
    }();

#define CK_REGISTER_ANGELSCRIPT_DYNAMIC_HANDLE_WITH_FRAGMENTS(_HandleTypeName_, ...)                  \
    static inline bool CK_CONCAT(AngelScriptDynamicHandle_, _HandleTypeName_) = []() -> bool          \
    {                                                                                                  \
        FCkDynamic_HandleTypeRegistry::RegisterHandleType(                                            \
            TEXT(#_HandleTypeName_), TArray<FString>{__VA_ARGS__});                                   \
        return true;                                                                                   \
    }();

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------