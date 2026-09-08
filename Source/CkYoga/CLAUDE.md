# CkYoga

`CkYoga` is the isolated Runtime module for Yoga 3.2.1's native C API. It exposes
the upstream public header through `<yoga/Yoga.h>` and deliberately has no Slate,
Blueprint, AngelScript, or Ck adapter surface.

The upstream source is vendored unchanged under `Public/CkYoga/yoga-3.2.1/yoga/`.
Do not edit it locally; update it only as a complete verified upstream import with
the accompanying source identity recorded in [UPSTREAM.md](UPSTREAM.md).

The module uses plain `ModuleRules`, C++20, no PCH/unity builds, RTTI disabled by
default, precise floating-point semantics, and local exception support because Yoga's upstream CMake configuration
requires exceptions. On modular Win64 targets `_WINDLL` is private to this module so
Yoga exports its native C ABI without making consumers exporters.

Keep `FPSemanticsMode.Precise`: under Unreal's `/fp:fast`, Yoga's `value != value`
undefined-dimension check compiled to unconditional false. This skipped intrinsic
measurement. `Ck.Yoga.Integration.UndefinedDimensionsPreserveNaNSemantics` pins
the required cross-module behavior.
