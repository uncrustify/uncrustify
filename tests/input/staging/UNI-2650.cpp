MergeJSFiles(new string[] {
    GetDecompressor(),
    Paths.Combine(buildToolsDir, "UnityConfig"),
    Paths.Combine(args.stagingAreaData, kOutputFileLoaderFileName),
}, unityLoader
);


throw new System.Exception(
    "'Fast Rebuild' option requires prebuilt JavaScript version of Unity engine. The following files are missing: "
    + (!File.Exists(UnityNativeJs) ? "\n" + UnityNativeJs : "")
    + (!File.Exists(UnityNativeJs + ".mem") ? "\n" + UnityNativeJs + ".mem" : "")
);
