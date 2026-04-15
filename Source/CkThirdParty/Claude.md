# CkThirdParty

**Purpose:** Third-party library distribution. All external libraries used by CkFoundation are vendored here, wrapped in a UE build module so they compile consistently across all platform targets.

**Depends on:** Nothing.
**Used by:** `CkCore` (fmt, ctti, cleantype, EnTT, bitwise-enum), `CkEcs` (EnTT), `CkPerception` (JoltPhysics), `CkSpatialQuery` (via CkCore/Geometry + JoltPhysics), `CkUI` (ThirdParty UI lib).

---

## Vendored libraries

| Folder | Library | Used for |
|---|---|---|
| `fmt/` | `{fmt}` | Format strings (`ck::Format`, `ck::Format_UE`) |
| `ctti/` | `ctti` (compile-time type info) | Type name introspection at compile time |
| `cleantype/` | `cleantype` | Cleaned-up C++ type names for diagnostics |
| `entt-3.16.0/` | EnTT 3.16 | The underlying ECS registry (`FCk_Registry`) |
| `bitwise-enum/` | `bitwise-enum` | Enables `|`/`&`/`~` on `UENUM(meta=(Bitflags))` types |
| `delegate/` | Custom delegate lib | Lightweight delegate used inside signal system |
| `JoltPhysics/` | Jolt Physics | Physics queries in `CkPerception` |

---

## Usage rules

1. **Never include ThirdParty headers directly from game code.** Include the CkCore wrappers (`Format/CkFormat.h`, `Entt/Entt.h`). This insulates you from vendored library updates.
2. **EnTT:** only include from `CkEcs` internals. All ECS operations go through `FCk_Handle` / `FCk_Registry` — not EnTT's raw API.
3. **Bitwise enum:** include via `CkCore/Enums/CkEnums.h`. The macro `ENABLE_ENUM_BITWISE_OPERATORS(EMyEnum)` is re-exported there.
4. **JoltPhysics:** exposed only through `CkPerception`. Don't add new direct Jolt includes outside that module.
5. **fmt / ctti / cleantype:** exposed only through `CkCore/Format/CkFormat.h` and `CkCore/TypeTraits/`. Don't add new raw includes.

---

## See also
- `CkCore/Format/README.md` — the fmt wrapper.
- `CkCore/Entt/README.md` — the EnTT include wrapper.
- `CkEcs/Claude.md` — the ECS layer on top of EnTT.
