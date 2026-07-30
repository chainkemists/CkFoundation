# CkCore / Policy

`ck::policy` namespace — the empty tag types passed as template arguments to select a behavioral
variant: `All` / `Any`, `TransientPackage`, `ReturnOptional`, `DontResetContainer`, `ForceErase`
(counted-tag removal), and `TMutability<Const|NonConst>`.

The types are pure tags with no CkCore-side meaning — each consumer defines what its policy
parameter does and `static_assert`s the ones it does not accept. There is no tag for a policy
slot's default; `void` fills it. Counted-tag semantics for `ForceErase` are enforced in `CkEcs`
(`FCk_Registry::Remove` / `Try_Remove`), the only layer that knows what a counted tag is.

**Key files:** `CkPolicy.h`

For the full use-case table and how this folder fits with the rest of CkCore, see [`../../Claude.md`](../../Claude.md).
