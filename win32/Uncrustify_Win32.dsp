# Microsoft Developer Studio Project File - Name="Uncrustify_Win32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Uncrustify_Win32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Uncrustify_Win32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Uncrustify_Win32.mak" CFG="Uncrustify_Win32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Uncrustify_Win32 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Uncrustify_Win32 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Uncrustify_Win32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /I ".\win32\\" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"Release/Uncrustify.exe"

!ELSEIF  "$(CFG)" == "Uncrustify_Win32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /I ".\win32\\" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/Uncrustify.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Uncrustify_Win32 - Win32 Release"
# Name "Uncrustify_Win32 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\align.cpp
# End Source File
# Begin Source File

SOURCE=..\src\align_stack.cpp
# End Source File
# Begin Source File

SOURCE=..\src\args.cpp
# End Source File
# Begin Source File

SOURCE=..\src\backup.cpp
# End Source File
# Begin Source File

SOURCE=..\src\brace_cleanup.cpp
# End Source File
# Begin Source File

SOURCE=..\src\braces.cpp
# End Source File
# Begin Source File

SOURCE=..\src\chunk_list.cpp
# End Source File
# Begin Source File

SOURCE=..\src\ChunkStack.cpp
# End Source File
# Begin Source File

SOURCE=..\src\combine.cpp
# End Source File
# Begin Source File

SOURCE=..\src\defines.cpp
# End Source File
# Begin Source File

SOURCE=..\src\detect.cpp
# End Source File
# Begin Source File

SOURCE=..\src\indent.cpp
# End Source File
# Begin Source File

SOURCE=..\src\keywords.cpp
# End Source File
# Begin Source File

SOURCE=..\src\lang_pawn.cpp
# End Source File
# Begin Source File

SOURCE=..\src\logger.cpp
# End Source File
# Begin Source File

SOURCE=..\src\logmask.cpp
# End Source File
# Begin Source File

SOURCE=..\src\md5.cpp
# End Source File
# Begin Source File

SOURCE=..\src\newlines.cpp
# End Source File
# Begin Source File

SOURCE=..\src\options.cpp
# End Source File
# Begin Source File

SOURCE=..\src\output.cpp
# End Source File
# Begin Source File

SOURCE=..\src\parens.cpp
# End Source File
# Begin Source File

SOURCE=..\src\parse_frame.cpp
# End Source File
# Begin Source File

SOURCE=..\src\punctuators.cpp
# End Source File
# Begin Source File

SOURCE=..\src\semicolons.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sorting.cpp
# End Source File
# Begin Source File

SOURCE=..\src\space.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tokenize.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tokenize_cleanup.cpp
# End Source File
# Begin Source File

SOURCE=..\src\uncrustify.cpp
# End Source File
# Begin Source File

SOURCE=..\src\universalindentgui.cpp
# End Source File
# Begin Source File

SOURCE=..\src\width.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\align_stack.h
# End Source File
# Begin Source File

SOURCE=..\src\args.h
# End Source File
# Begin Source File

SOURCE=..\src\backup.h
# End Source File
# Begin Source File

SOURCE=..\src\base_types.h
# End Source File
# Begin Source File

SOURCE=..\src\char_table.h
# End Source File
# Begin Source File

SOURCE=..\src\chunk_list.h
# End Source File
# Begin Source File

SOURCE=..\src\ChunkStack.h
# End Source File
# Begin Source File

SOURCE=..\src\config.h
# End Source File
# Begin Source File

SOURCE=..\src\ListManager.h
# End Source File
# Begin Source File

SOURCE=..\src\log_levels.h
# End Source File
# Begin Source File

SOURCE=..\src\logger.h
# End Source File
# Begin Source File

SOURCE=..\src\logmask.h
# End Source File
# Begin Source File

SOURCE=..\src\md5.h
# End Source File
# Begin Source File

SOURCE=..\src\options.h
# End Source File
# Begin Source File

SOURCE=..\src\prototypes.h
# End Source File
# Begin Source File

SOURCE=..\src\punctuators.h
# End Source File
# Begin Source File

SOURCE=..\src\token_enum.h
# End Source File
# Begin Source File

SOURCE=..\src\token_names.h
# End Source File
# Begin Source File

SOURCE=..\src\uncrustify_types.h
# End Source File
# Begin Source File

SOURCE=..\src\uncrustify_version.h
# End Source File
# Begin Source File

SOURCE=.\windows_compat.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
