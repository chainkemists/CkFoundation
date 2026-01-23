#pragma once

#include "CkEcs/Handle/CkHandle.h"

#if WITH_ANGELSCRIPT_CK

#include "CkEcs/Handle/CkHandle_AngelScript_Registry.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkDynamic_HandleDefinition;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Registry for dynamic (data asset-defined) handle types.
 *
 * This class handles loading dynamic handle definitions from JSON and data assets,
 * then delegates to FCkAngelScript_HandleRegistry for unified binding.
 */
class CKDYNAMIC_API FCkDynamic_HandleTypeRegistry
{
public:
    // ------------------------------------------------
    // JSON Registry
    // ------------------------------------------------

    static auto
    LoadFromJsonRegistry() -> bool;

    static auto
    GetRegistryFilePath() -> FString;

    // ------------------------------------------------
    // Registration
    // ------------------------------------------------

    static auto
    RegisterHandleTypeFromDefinition(
        const UCkDynamic_HandleDefinition* InDefinition) -> bool;

    static auto
    RegisterHandleType(
        const FString& InTypeName,
        const TArray<FString>& InRequiredFragments = {},
        const FString& InDescription = {},
        const FString& InSourceAsset = {}) -> bool;

    static auto
    RegisterHandleType(
        const FString& InTypeName,
        const FString& InValidatorFragmentName) -> bool;

    static auto
    DiscoverAndRegisterAllDefinitions() -> void;

    // ------------------------------------------------
    // Queries (delegated to unified registry)
    // ------------------------------------------------

    static auto
    IsHandleTypeRegistered(
        const FString& InTypeName) -> bool;

    static auto
    GetHandleTypeInfo(
        const FString& InTypeName) -> const FCkAngelScript_HandleTypeInfo*;

    static auto
    GetAllRegisteredTypes() -> const TMap<FString, TSharedPtr<FCkAngelScript_HandleTypeInfo>>&;

    static auto
    ValidateHandle(
        const FString& InTypeName,
        const FCk_Handle& InHandle) -> bool;

    // ------------------------------------------------
    // Initialization
    // ------------------------------------------------

    static auto
    EnsureCallbackRegistered() -> void;

private:
    static auto
    CreateMultiFragmentValidator(
        const TArray<FString>& InFragmentNames) -> TFunction<bool(const FCk_Handle&)>;

    static auto
    FindScriptStructByName(
        const FString& InStructName) -> const UScriptStruct*;

    static auto
    ExtractShortName(
        const FString& InTypeName) -> FString;

private:
    static inline FDelegateHandle _PreCompileDelegateHandle;
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