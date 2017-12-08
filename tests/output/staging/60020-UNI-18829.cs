// It shouldn't detele the space after the tuple definition
public static (bool updated, Warnings warnings) UpdateIncludesInFile(
    string fileToUpdate, string oldIncludeFile, string newIncludeFile)
{
    // ...
}

// It shouldn't detele the space after the tuple definition
public static (int, string) UpdateIncludesInFile(
    string fileToUpdate, string oldIncludeFile, string newIncludeFile)
{
    // ...
}

// It shouldn't detele the space after the tuple definition and updated, warnings should be tokenized as types
public static (updated, warnings) UpdateIncludesInFile(
    string fileToUpdate, string oldIncludeFile, string newIncludeFile)
{
    // ...
}
