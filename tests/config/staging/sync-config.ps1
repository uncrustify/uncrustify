$um = Resolve-Path("$(hg showconfig unity-repos.unity-meta)");
Copy-Item -Path "$um\Configs\Uncrustify\*" -Filter "*.cfg" -Destination "$PSScriptRoot" -Recurse