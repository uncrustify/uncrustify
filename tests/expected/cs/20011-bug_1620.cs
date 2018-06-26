new WaveformStreamer(s_sweepedClip, 0, s_sweepedClip.length, 1,
    (streamer, floats, remaining) =>
    {
        return false;
    }
);

using System.Collections.Generic;

class C
{
    public void S()
    {
        Action localMethod = () => {
            SomeClass.OtherMethod(new Dictionary<string, string>
            {
                {"a", "one"},
                {"b", "two"},
                {"c", "three"}
            });
        };
        m_Mixers.Add(
            new WeightInfo
            {
                parentMixer = parent,
                mixer = node,
                port = port,
                modulate = (type == typeof(AnimationLayerMixerPlayable))
            }
        );
    }
}

MergeJSFiles(new string[] {
    GetDecompressor(),
    Paths.Combine(buildToolsDir, "UnityConfig"),
    Paths.Combine(args.stagingAreaData, kOutputFileLoaderFileName),
}, unityLoader
);

public void GeneratesCorrectVisualStudioProjectFile()
{
    GenerateProjectsAndCompareWithTemplates(
        runInJam: InJamCreateTestProject,
        generatedPath: TestRoot.Combine("Solution"),
        templatesPath: "Tools/Unity.BuildSystem/Unity.BuildSystem.VisualStudio.Tests/Templates",
        templates: new[] {
            "TestProjectGeneration_CApplication.sln",
            "Projects/TestProjectGeneration_CApplication.vcxproj",
            "Projects/TestProjectGeneration_CApplication.vcxproj.filters"
        });
}

public void GeneratesCorrectVisualStudioProjectFile()
{
    GenerateProjectsAndCompareWithTemplates(
        runInJam: InJamCreateTestProject,
        generatedPath: TestRoot.Combine("Solution"),
        templatesPath: "Tools/Unity.BuildSystem/Unity.BuildSystem.VisualStudio.Tests/Templates",
        templates: new[]
        {
            "TestProjectGeneration_CApplication.sln",
            "Projects/TestProjectGeneration_CApplication.vcxproj",
            "Projects/TestProjectGeneration_CApplication.vcxproj.filters"
        }
    );
}
