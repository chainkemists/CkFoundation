# CkEcsTemplate

**Purpose:** Entity template data assets and the spawn infrastructure for templates that use the full `CkEcsExt`/`CkProvider` feature set. Complementary to `CkTemplate` (which is the lighter-weight template module).

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Systems spawning complex entity presets with provider-driven fragment defaults.

See `CkTemplate/Claude.md` for the pattern overview. `CkEcsTemplate` adds: provider-driven default values, EcsExt meta-fragment support, and label pre-assignment in the template definition.

## See also
- `CkTemplate/Claude.md` — the lighter template module.
- `CkProvider/Claude.md` — provider-driven values in template params.
- `CkEcsExt/Claude.md` — meta fragment infrastructure.
