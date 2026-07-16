using System;
using System.Diagnostics;
using System.IO;
using UnrealBuildTool;

public class CkCore : CkModuleRules
{
    public CkCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivatePCHHeaderFile = "Public/CkCommon_PCH.h";
        SharedPCHHeaderFile = "Public/CkCommon_PCH.h";

        PublicIncludePaths.AddRange(
            new string[] {
            }
            );

        PrivateIncludePaths.AddRange(
            new string[] {
            }
            );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "GameplayTags",
                "NetCore",
                "Projects",

                "IrisCore",

                "CkBuildConfig",
                "CkLog",
                "CkSettings",
                "CkThirdParty",
            }
            );

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "MessageLog",
                    "ToolWidgets",
                }
                );
        }

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "BlueprintGraph",
                    "GameplayTagsEditor",
                }
                );
        }

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "DeveloperSettings",
                "GameplayAbilities",
                "GeometryCore",

                "CkLog",
                "CkThirdParty",
            }
            );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
            }
            );

        // Determine the repo root: prefer the .uproject directory so the hashes
        // reflect the game project, not the CkFoundation submodule. Fall back to
        // walking 4 levels up from Source/CkCore/ if Target.ProjectFile is
        // unavailable (e.g., engine-tool builds).
        string repoDir = (Target.ProjectFile != null)
            ? Target.ProjectFile.Directory.FullName
            : Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "..", ".."));

        GenerateBuildIdHeader(ModuleDirectory, repoDir);
    }

    // Reference branches to compute merge-bases against.
    // Extend this list to add more branches; Build.cs tries each local name
    // first then falls back to origin/<name>.
    private static readonly string[] s_ReferenceBranches = { "dev", "develop", "main", "master", "demo", "release" };

    // -------------------------------------------------------------------------
    // Bakes the project's git hashes into a constexpr header so the shipped
    // binary can display them — and compare them across the network — without
    // needing git at runtime. The short HEAD hash is the canonical build id.
    // Failure-safe: any exception falls through to writing "unknown".
    // -------------------------------------------------------------------------

    private static void GenerateBuildIdHeader(string ModuleDir, string RepoDir)
    {
        string head = "unknown";
        var branchNames  = new System.Collections.Generic.List<string>();
        var branchHashes = new System.Collections.Generic.List<string>();

        try
        {
            head = RunGit(RepoDir, "rev-parse --short HEAD") ?? "unknown";

            foreach (string branch in s_ReferenceBranches)
            {
                // Try the local branch first, then the remote tracking branch.
                string fullBase = RunGit(RepoDir, "merge-base HEAD " + branch)
                               ?? RunGit(RepoDir, "merge-base HEAD origin/" + branch);
                string hash = (fullBase != null)
                    ? (RunGit(RepoDir, "rev-parse --short " + fullBase.Trim()) ?? "unknown")
                    : "unknown";
                branchNames.Add(branch);
                branchHashes.Add(hash);
            }
        }
        catch { /* git unavailable — fall through to write "unknown" for all entries */ }

        // Fill any entries that didn't make it through the try block.
        while (branchNames.Count < s_ReferenceBranches.Length)
        {
            branchNames.Add(s_ReferenceBranches[branchNames.Count]);
            branchHashes.Add("unknown");
        }

        int branchCount = branchNames.Count;

        var quotedNames  = new System.Collections.Generic.List<string>();
        var quotedHashes = new System.Collections.Generic.List<string>();
        foreach (string n in branchNames)  quotedNames.Add("\"" + n + "\"");
        foreach (string h in branchHashes) quotedHashes.Add("\"" + h + "\"");

        string namesArray  = string.Join(", ", quotedNames.ToArray());
        string hashesArray = string.Join(", ", quotedHashes.ToArray());

        string outDir  = Path.Combine(ModuleDir, "Public", "CkCore", "Generated");
        string outPath = Path.Combine(outDir, "CkCore_BuildId.h");
        string content =
            "// Auto-generated by CkCore.Build.cs — do not edit.\n"
          + "#pragma once\n\n"
          + "namespace CkCoreBuildId\n{\n"
          + "    constexpr const char* HeadHash = \"" + head.Trim() + "\";\n\n"
          + "    constexpr int         BranchCount           = " + branchCount + ";\n"
          + "    constexpr const char* BranchNames["     + branchCount + "]     = { " + namesArray  + " };\n"
          + "    constexpr const char* MergeBaseHashes[" + branchCount + "] = { " + hashesArray + " };\n"
          + "}\n";

        try
        {
            Directory.CreateDirectory(outDir);
            // Only rewrite when content changes to avoid unnecessary recompilation.
            if (!File.Exists(outPath) || File.ReadAllText(outPath) != content)
                File.WriteAllText(outPath, content);
        }
        catch { /* silently ignore write failure — header may exist from a prior build */ }
    }

    private static string RunGit(string WorkDir, string Args)
    {
        try
        {
            var psi = new ProcessStartInfo("git", Args)
            {
                WorkingDirectory       = WorkDir,
                RedirectStandardOutput = true,
                RedirectStandardError  = true,
                UseShellExecute        = false,
                CreateNoWindow         = true,
            };
            using (var proc = Process.Start(psi))
            {
                if (proc == null) return null;
                proc.WaitForExit(3000);
                return proc.ExitCode == 0 ? proc.StandardOutput.ReadToEnd().Trim() : null;
            }
        }
        catch { return null; }
    }
}