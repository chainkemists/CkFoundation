# CkCore / IO

File / asset discovery helpers and deferred-config writing.

**Key files:** `CkIO_Utils.h`, `CkDeferredConfig.h`

## Asset search

```cpp
UENUM() enum class ECk_AssetLocalRootType : uint8 { Project, Engine, ProjectPlugin, EnginePlugin, Invalid };

UENUM(meta = (Bitflags)) enum class ECk_AssetSearchScope : uint8
{
    None    = 0, Game = 1<<0, Plugins = 1<<1, Engine = 1<<2,
    All     = Game | Plugins | Engine
};

UENUM() enum class ECk_AssetSearchStrategy : uint8 { /* ... */ };
```

`UCk_Utils_IO_UE` exposes asset-registry-backed discovery across these scopes, plus text-size helpers (`ECk_Engine_TextFontSize`) used by debug overlays.

## Deferred config

`CkDeferredConfig.h` — write to `.ini` but batch flushes to the end of frame / end of transaction. Use when you'd otherwise spam `GConfig->SetString` in a loop.

## When to use

- **Asset discovery** — when you need to enumerate all `UDataAsset`s of a kind under a scope. The asset registry's raw API is correct but verbose; this wraps the common cases.
- **Deferred config** — batch settings writes that need to survive a session but shouldn't hit disk on every change (UI state, editor prefs).

## Depends on
`Format/`, `Macros/`, `Enums/`, UE's asset registry.

## Used by
`CkResourceLoader`, `CkAngelscriptGenerator`, editor tooling modules.

## See also
- UE's asset-registry docs — the underlying API.
