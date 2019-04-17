# Microsoft Developer Studio Project File - Name="p5" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=p5 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "p5.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "p5.mak" CFG="p5 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "p5 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "p5 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "p5 - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "p5 - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../Debug/p5.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "p5 - Win32 Release"
# Name "p5 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\console.c
# End Source File
# Begin Source File

SOURCE=.\helpwin.c
# End Source File
# Begin Source File

SOURCE=.\info.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\math.c
# End Source File
# Begin Source File

SOURCE=.\p5.rc
# End Source File
# Begin Source File

SOURCE=..\Common\p5lib.c
# End Source File
# Begin Source File

SOURCE=.\parser.c
# End Source File
# Begin Source File

SOURCE=.\plugin.c
# End Source File
# Begin Source File

SOURCE=.\rootparser.c
# End Source File
# Begin Source File

SOURCE=.\script.c
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# Begin Source File

SOURCE=.\watcher.c
# End Source File
# Begin Source File

SOURCE=..\Common\wp_layout.c

!IF  "$(CFG)" == "p5 - Win32 Release"

!ELSEIF  "$(CFG)" == "p5 - Win32 Debug"

# ADD CPP /D LmMalloc=PfMalloc /D LmFree=PfFree

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\wp_mainwin.c

!IF  "$(CFG)" == "p5 - Win32 Release"

!ELSEIF  "$(CFG)" == "p5 - Win32 Debug"

# ADD CPP /D MwMalloc=PfMalloc /D MwFree=PfFree

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\wp_parse.c

!IF  "$(CFG)" == "p5 - Win32 Release"

!ELSEIF  "$(CFG)" == "p5 - Win32 Debug"

# ADD CPP /D StrMalloc=PfMalloc /D StrFree=PfFree /D StrRealloc=PfRealloc

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\wp_scheme2.c

!IF  "$(CFG)" == "p5 - Win32 Release"

# ADD CPP /D SchMalloc=ParMalloc /D SchFree=ParFree

!ELSEIF  "$(CFG)" == "p5 - Win32 Debug"

# ADD CPP /D SchMalloc=ParMalloc /D SchFree=ParFree /D "SchExceptions"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\p5.h
# End Source File
# Begin Source File

SOURCE=..\Common\p5lib.h
# End Source File
# Begin Source File

SOURCE=..\Common\plugin.h
# End Source File
# Begin Source File

SOURCE=..\Common\wp_layout.h
# End Source File
# Begin Source File

SOURCE=..\Common\wp_mainwin.h
# End Source File
# Begin Source File

SOURCE=..\Common\wp_parse.h
# End Source File
# Begin Source File

SOURCE=..\Common\wp_scheme2.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\alias.ico
# End Source File
# Begin Source File

SOURCE=.\appicon.ico
# End Source File
# Begin Source File

SOURCE=.\define.ico
# End Source File
# Begin Source File

SOURCE=.\folder.ico
# End Source File
# Begin Source File

SOURCE=.\function.ico
# End Source File
# Begin Source File

SOURCE=.\helpwin.ico
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\info.ico
# End Source File
# Begin Source File

SOURCE=.\p5doc.ico
# End Source File
# Begin Source File

SOURCE=.\variable.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\commandline.txt
# End Source File
# Begin Source File

SOURCE=.\script.txt
# End Source File
# Begin Source File

SOURCE=.\scriptha.txt
# End Source File
# Begin Source File

SOURCE=.\watcher.txt
# End Source File
# End Target
# End Project
