# Gate126: liveness watchdog fixture correction

Focused BusterBlock `NpcAI` passes **27/27** (Gate125 baseline: 26/27). Gate126a used one fresh editor boot, the existing Development Editor binaries, and no C++ build: 6m4s, Toolbox/editor exit 0, zero failed/skipped/contaminated.

The old fixture enrolled its synthetic subject into a queue but omitted `FBb_Fragment_Npc_Agent`. The production observer resolves membership through `npc_ai_loco::Get_NpcAgent`, so it saw no member and accrued 1.066707 seconds instead of applying the queue exemption. Production NPCs are themselves CrowdAgents; the corrected fixture uses `utils_crowd_agent::Add` on the subject transform and publishes that exact alias.

Adding the alias also admits the subject to the production FrameCache processor. A test-only session fragment and stimulus processor therefore provide controlled leaf/location inputs after FrameCache and before ObserverSafety. They never write the watchdog result. The original stall/trip/backoff/reset/exemption/leave assertions, seeded intervals and time limits remain intact. New preconditions verify the production-resolved membership, same-entity identity, and active checkout leaf both while queued and after leaving.

Changed BusterBlock files:

- `Plugins/BusterBlockTests/Script/Tests/NpcAI/BB_AutoTest_NpcAI_LivenessWatchdogAccrual.as`
- `Plugins/BusterBlockTests/Script/Tests/NpcAI/BB_NpcAI_Liveness_TestStimulus.as`
- `Plugins/BusterBlockTests/Script/Generated/BusterBlockTests_ScriptProcessorDrivers.as` (24 lines generated automatically during startup)

No production code changed. The generated addition was reviewed, the startup reload completed before automation, and a post-generation/pre-test identity checkpoint remained identical through completion. Compiled artifacts match Gate125. Final preservation checks: eight repository HEADs/local-dev/index gitlinks unchanged; 13 protected files exact; 16 shared framework/test files mirrored exactly. Independent review caught and resolved an initial child-agent fixture topology mistake before the only test boot.

Command: `CkAuto/UnrealToolbox.exe --test --test-pattern=NpcAI --no-live --parallel=1 --serial-lane=* --project=D:/Repos/BusterBlock --output=D:/Repos/BusterBlock/Saved/Logs/Nav-Gate126a-NpcAI-LivenessFixture.log` (detached launch with progress window).

Raw archive: `D:/Repos/BusterBlock/Saved/Logs/Nav-Gate126a-NpcAI-Runtime.log`; SHA-256 `5d2a18d94dcc842cffc6916023f27f4a3a85e6df2508555057f438e907bd83a7`. No ensures or AngelScript errors/warnings. The two known missing external-actor paths each appeared twice; ordinary startup warnings remain. Full rows, hashes and current tracking refs are in [Gate126 evidence](GATE126_VALIDATION_EVIDENCE.json).

Newer BusterBlock tracked dev tips still require a controlled preservation/rebase checkpoint. No commit, rebase, push, dev advancement or local cook/package was performed. Production remains Recast with shadow Off. Gate124 A/B identities remain historical, and every deferred editor, streaming, cooked-data/package, matched-performance, production-map-adoption and unrelated-work item remains in the [continuation handoff](CONTINUATION_PROMPT_BusterBlockCompatibilityAndABReadiness.md). The seven approved gyms remain closed.
