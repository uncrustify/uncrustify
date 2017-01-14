rem set to on for debugging
@echo off
setlocal

rem Run this script from the project root directory

echo ------------------------------------------------------
echo Start pcLint analysis to check code quality ...

set SRC_DIR=src
set EXC_DIR=lnt
set OUT_DIR=tests\pclint
set LNT_DIR=scripts\pclint

rem Check if pcLint program is available
set prog=lint-nt.exe
for %%i in ("%path:;=";"%") do (
rem echo %%~i
    if exist %%~i\%prog% (
		set found=%%i
		echo found %prog% in %%i
	)
)
if %found%=="" goto PROG_MISSING

if NOT EXIST tests        md tests
if NOT EXIST tests\pclint md tests\pclint

rem create list of all C source files to analyze
rem FIXME: works only if there are no spaces in the paths 

dir /s/b %EXC_DIR%\*.lnt		> .\%OUT_DIR%\exceptions.lnt

rem to check single files activate one of the lines below
rem dir /s/b %SRC_DIR%\newlines.cpp	> .\%OUT_DIR%\files.lnt
rem dir /s/b %SRC_DIR%\indent.cpp 	>> .\%OUT_DIR%\files.lnt
rem dir /s/b %SRC_DIR%\a*.cpp 	>> .\%OUT_DIR%\files.lnt
rem dir /s/b %SRC_DIR%\b*.cpp 	>> .\%OUT_DIR%\files.lnt
rem dir /s/b %SRC_DIR%\c*.cpp 	>> .\%OUT_DIR%\files.lnt
rem dir /s/b %SRC_DIR%\d*.cpp 	>> .\%OUT_DIR%\files.lnt
rem dir /s/b %SRC_DIR%\l*.cpp 	>> .\%OUT_DIR%\files.lnt
rem dir /s/b %SRC_DIR%\o*.cpp 	>> .\%OUT_DIR%\files.lnt
rem dir /s/b %SRC_DIR%\p*.cpp 	>> .\%OUT_DIR%\files.lnt
rem dir /s/b %SRC_DIR%\u*.cpp 	>> .\%OUT_DIR%\files.lnt

rem to check all source files use the line below
dir /s/b %SRC_DIR%\*.cpp 	> .\%OUT_DIR%\files.lnt

rem use this to save the pclint errors to a file for later review
lint-nt .\%LNT_DIR%\pclint_cfg_eclipse.lnt .\%OUT_DIR%\exceptions.lnt .\%OUT_DIR%\files.lnt > .\%OUT_DIR%\pclint-results.xml

rem to make eclipse parse the pclint errors it has to be output to the console
rem lint-nt .\%LNT_DIR%\pclint_cfg_eclipse.lnt .\%OUT_DIR%\exceptions.lnt .\%OUT_DIR%\files.lnt

rem type %OUT_DIR%\pclint-results.xml | more
rem type %OUT_DIR%\pclint-results.xml
rem echo pcLint output placed in %OUT_DIR%\pclint-results.xml

goto END

:PROG_MISSING
echo.
echo ------------------------------------------------------
echo pcLint Error: %prog% not found.
echo Verify that PCLINT is correctly installed, the 
echo installation was added to the PATH and the
echo environment variable PCLINT_HOME was set to its path. 
echo ------------------------------------------------------ 
echo.
goto END

:END 
echo pcLint finished
echo ------------------------------------------------------
endlocal

