# Gate119 presentation candidate — artifact identity

Date: 2026-09-10. Source frozen for Gate119c and the following PathNetwork gate. Root/Foundation/Tests revisions are unchanged from `GATE119_SHOWCASE_BASELINE_AND_CONTRACT.md`; this manifest identifies the uncommitted presentation source and Toolbox-generated discovery actor. It is not a capture manifest or visual sign-off.

## Runtime artifacts reused (SHA256)

No C++ build was performed. These existing host DLLs were hashed during the gate:

| Root-relative path | SHA256 |
|---|---|
| `Binaries/Win64/CkPluginsEditor-CkTests.dll` | `6B79B2A32542F3FFB9AF53AE82A5A220AC3B8B2444F3195416E5A4FD713F9D37` |
| `Binaries/Win64/CkPluginsEditor-CkGroundNav.dll` | `FC6139DA4293463745582F13BF3840BAD3DB17670CA1252D746246300EEDA45F` |
| `Binaries/Win64/CkPluginsEditor-CkNavigation.dll` | `D658072EA9F7B0A692DE6D96133D5B4639EA28F2D1BB535563E2CC2EEB05A71F` |

## Candidate files (relative to Plugins/CkTests)

| Path | SHA256 |
|---|---|
| `Content/__ExternalActors__/AutoTests/AutoTests_CkTests_Level/D/B9/1J8TN2AMOLHHONH0UD0PS6.uasset` | `B3C4D990F5BCA93D5637A057C89316C1F8BA2B6FDABB71AEE0BD61CD6364199B` |
| `Script/CkGroundNav/CkAutoTest_GroundNav_GymPresentationRestoresState.as` | `F783F989DCC11D9F23CDC310D72B1759A267352717CF79418B6F3259B6909D4B` |
| `Script/CkGroundNav/CkGroundNavGym_Common.as` | `B416D1B5102BC6F8451D5B8F6205DCF224D3B0EDB319878E762D48F535F40CD2` |
| `Script/CkGroundNav/CkGroundNavGym_Links_GameMode.as` | `B0F256819309B58F065532B4D810AEAD5BC1FB73EBD3C4C91B4E1940B6C63A87` |
| `Script/CkGroundNav/CkGroundNavGym_Links_PlayerController.as` | `1A70125F89CCEF8ECCC33B651EAAC4D26134BD0FC5BFD0568C8B380662A74720` |
| `Script/CkGroundNav/CkGroundNavGym_Markup_GameMode.as` | `F1C018677BFDBDB01CCBFAC53F0F5A983986988ECA6B84D48E0DCD27224F7F4F` |
| `Script/CkGroundNav/CkGroundNavGym_Markup_PlayerController.as` | `10393D1C811C1409C17D8F164DFA5EE679DC9803F240441F935B8196774D530D` |
| `Script/CkGroundNav/CkGroundNavGym_Obstacle_GameMode.as` | `D73E717B824DF36516237CE3E6012EED020B24C208E5A2764E3A8331390738E4` |
| `Script/CkGroundNav/CkGroundNavGym_Obstacle_PlayerController.as` | `C6DFCC773C2862BD8502C80D1E31AD697067FD7380F1D4B474D0441406380087` |
| `Script/CkGroundNav/CkGroundNavGym_Parity_GameMode.as` | `4448150FAFBDADFB2685E9E7F5E2652270B2C3604C1E8A98E87CC9B83DC0AC42` |
| `Script/CkGroundNav/CkGroundNavGym_Parity_PlayerController.as` | `232E0C1F318498B3141C4515DAACB0936CD252F542EEBB763BC2FCA6F33A4FDF` |
| `Script/CkGroundNav/CkGroundNavGym_TuningRange_GameMode.as` | `2D7CCD7EBA95349DE0793BE9400DB8BC3B185F40B6F59A802467B203A29F50CF` |
| `Script/CkGroundNav/CkGroundNavGym_TuningRange_PlayerController.as` | `05926D5995986387648E2BF6F2AE15D7AA6FB6BD71E34E5DA9DA0419F4BBD354` |
| `Script/CkGroundNav/CkGroundNavGym_Walk_GameMode.as` | `F1EA1B312B2CD3376CA9079E381302B83F4A54C527A66E33643041221F1AC637` |
| `Script/CkGroundNav/CkGroundNavGym_Walk_PlayerController.as` | `DDF0580136794B29B4932BBC96F1AA980A3DC8F558D6C14A5863455A5850E85F` |
| `Script/CkPathNetwork/CkPathNetworkGym_Following_GameMode.as` | `C1A49C457B1F61870C0D17D3DB41BAF718352129390FE7D19F1EB4EDCE104E61` |
| `Script/CkPathNetwork/CkPathNetworkGym_Following_PlayerController.as` | `FB9A82E0E3694A8DD5AD3B50B69130121440BAD194C5D30A9BDBE7D9B479E197` |
| `Script/Common/CkGym_ControlPanelHUD.as` | `E5DEC2DD0E1DF5A581AA9DB608907FB406D588A516FD275FB6EABFC6E77B2772` |
| `Script/Common/CkNavigationGym_Presentation_HUD.as` | `6E84E160AF46257FBCE64EFF7E90BA70D6E22FDB4AE21F90966A3BFA09518F38` |
| `Script/Common/CkNavigationGym_Presentation_PlayerController.as` | `34A00DBEB7B487019F0B06254FE30A87C2A48E9AA9E9FFD5D31138FAF0C3624F` |
| `Script/Generated/CkTests_AutoTestActors.as` | `A3CA7F1C2A10516895B319A745B8A2FBCCDF4CD256915A84EC041606F34FBE40` |

## Evidence limits

The focused presentation test inherits production control dispatch, framing and material methods but suppresses unrelated gym-base BeginPlay and uses a plain APawn. It proves valid/rejected control state and material/geometry/collision/provider/revision/persisted-preference invariants only. It does not prove real input routing, rendered camera framing, actual HUD appearance, gym startup/lifecycle, or any of the seven feature walkthroughs. Gate119b's failed synthetic gym bootstrap is retained in PROGRESS.md rather than suppressed or described as a production fix.

After Gate120 completed, all thirteen unrelated dirty files in the entry manifest and all twenty-one candidate files above were rehashed: 34 checked, zero changed. No original user files or build artifacts were deleted, and no other checkout was modified. The generated test wrapper and external actor are new task outputs, not unrelated dirt.

Final automation: Gate119c GroundNav 523/523; Gate120 PathNetwork 108/108; zero failed/skipped/contaminated in both. GroundNav's network lane retains the historical Iris diagnostic; PathNetwork has no fresh script errors or ensure failures. Exact commands, durations and log hashes are in PROGRESS.md. The source remains a presentation candidate until the capture runbook and human review are complete.
