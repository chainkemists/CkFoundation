# CkCore / Format

`{fmt}`-based string formatting for CkFoundation. Works with UE's `FString`/`FName`/`FText`, handles custom types through registered formatters, and is AS-aware.

**Key files:** `CkFormat.h`, `CkFormat_Defaults.h`, `CkFormat_AngelScript.h`

## Public API

```cpp
namespace ck
{
    template <typename TString, typename... TArgs>
    auto Format      (TString InStr, TArgs&&... InArgs) -> std::basic_string<TCHAR>;

    template <typename TString, typename... TArgs>
    auto Format_ANSI (TString InStr, TArgs&&... InArgs) -> std::string;

    // Wrapper returning FString:
    template <typename TString, typename... TArgs>
    auto Format_UE   (TString InStr, TArgs&&... InArgs) -> FString;
}
```

## Syntax

`{fmt}`-style, not printf. Use `{}` for default formatting, `{:spec}` for format specs:

```cpp
ck::Format(TEXT("Count={}, Name={}"), N, Name);          // std::wstring
ck::Format_UE(TEXT("Count={:04}, Name={}"), N, Name);    // FString
```

## Registering a formatter for your type

The pattern is `CK_DEFINE_CUSTOM_FORMATTER_*`. Enums get a one-liner:

```cpp
UENUM(BlueprintType)
enum class ECk_MyMode : uint8 { Off, On, Auto };
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MyMode);
```

For structs, specialize `fmt::formatter<YourType, TCHAR>` or use the project helper macros (see `CkFormat_Defaults.h`). Built-in formatters cover: `FName`, `FString`, `FText`, `FGameplayTag`, `FGameplayTagContainer`, `FSoftObjectPath`, `FVector`, `FRotator`, most core CkFoundation types.

## Why not `FString::Printf`?

1. **Type-safe.** No format-specifier/arg-type mismatches at runtime.
2. **Extensible.** Custom types register a formatter once; any call site formats them correctly.
3. **Unified with log/ensure.** The log macros and `CK_ENSURE_IF_NOT` both take fmt syntax. Using `FString::Printf` means you'd have two different mental models.
4. **AngelScript parity.** `CkFormat_AngelScript.h` exposes the same formatters to AS.

## Wide-char note

`ck::Format` returns `std::basic_string<TCHAR>` (wide on Windows/UE). `ck::Format_UE` wraps that as `FString`. `ck::Format_ANSI` exists for the narrow-string edge cases (log file paths, etc.) — reach for it rarely.

## Depends on
`CkThirdParty/fmt` (header-only), `CkThirdParty/ctti/nameof`, `cleantype`. Pulls in `Validation/CkIsValid.h` and `Debug/CkDebug_Utils.h` so that default formatters for handle-like types can print validity state.

## Used by
Log, Ensure, and every module that emits diagnostic strings.

## See also
- `Enums/` — `CK_DEFINE_CUSTOM_FORMATTER_ENUM` lives in the enum/format glue.
- `Ensure/` — `CK_ENSURE` consumes fmt-format strings directly.
