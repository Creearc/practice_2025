// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Voice_Capture : ModuleRules
{
	public Voice_Capture(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore",
			"AudioCapture", "SignalProcessing", "AudioMixer",
			"UMG", "Slate", "SlateCore"
		});

        var VoskPath = Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "Vosk");

        // 1) include-путь для vosk_api.h
        PublicIncludePaths.Add(Path.Combine(VoskPath, "include"));

        // 2) линковка статической библиотеки libvosk.lib
        PublicAdditionalLibraries.Add(Path.Combine(VoskPath, "lib", "libvosk.lib"));

        // 3) runtime‑зависимость для .dll файлов для Binaries
        RuntimeDependencies.Add(Path.Combine(VoskPath, "lib", "libvosk.dll"), StagedFileType.NonUFS);
        RuntimeDependencies.Add(Path.Combine(VoskPath, "lib", "libstdc++-6.dll"), StagedFileType.NonUFS);
        RuntimeDependencies.Add(Path.Combine(VoskPath, "lib", "libgcc_s_seh-1.dll"), StagedFileType.NonUFS);
        RuntimeDependencies.Add(Path.Combine(VoskPath, "lib", "libwinpthread-1.dll"), StagedFileType.NonUFS);


        PublicDelayLoadDLLs.Add("libvosk.dll");



        PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
