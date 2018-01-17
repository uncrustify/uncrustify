@echo off
setlocal

rem
rem 14.12.2016
rem
rem Checks if all source files conform
rem to the uncrustify configuration
rem The check is performed with the uncrustify
rem program that is available in the path.
rem This might be an older version than
rem a newly build one.
rem 
rem Call this batch script from the top level
rem directory of the uncrustify project.

set SRC_DIR=src
set OUT_DIR=results
set CFG_FILE=forUncrustifySources.cfg
set DIFF_FILE=lastdiff.txt
set "PWD=%cd%"

rem Check if uncrustify is available in path
set prog=uncrustify.exe
for %%i in ("%path:;=";"%") do (
    if exist %%~i\%prog% (
		set found=%%i
rem		echo found %prog% in %%i
	)
)

rem Check if fc (file compare tool) is available in path
set CMP=fc.exe
for %%i in ("%path:;=";"%") do (
    if exist %%~i\%CMP% (
		set found=%%i
rem		echo found %prog% in %%i
	)
)

if %found%=="" goto PROG_MISSING

rem build the list of source files
set SRC_LIST=%PWD%\%OUT_DIR%\files.txt
dir /b %SRC_DIR%\*.h   >  %SRC_LIST%
dir /b %SRC_DIR%\*.cpp >> %SRC_LIST%

rem ensure output directory exists
if NOT EXIST %OUT_DIR% md %OUT_DIR%

rem check every source file with uncrustify
rem and compare it with the original file
cd .\%SRC_DIR%
set DIFF=0
for /F %%i in (%SRC_LIST%) do (
	%prog% -q -c ..\%CFG_FILE% -f .\%%i -o ..\%OUT_DIR%\%%i
	%CMP% /L .\%%i ..\%OUT_DIR%\%%i > ..\%OUT_DIR%\%DIFF_FILE% 
	if %ERRORLEVEL% NEQ 0 (
		echo "Problem with %%i"
		echo "use: diff %SRC_DIR%\%%i %OUT_DIR%\%%i to find why"
		set DIFF=1
	)
	if %ERRORLEVEL% EQU 0 (
		del ..\%OUT_DIR%\%%i
		del ..\%OUT_DIR%\%DIFF_FILE%
	)
	pause
)

del %SRC_LIST%

if %DIFF% NEQ 0 (
  echo "some problem(s) are still present"
  exit /b 1
)

echo "all sources are uncrustify-ed"
exit /b 0


:PROG_MISSING
echo.
echo ------------------------------------------------------
echo Error: %prog% not found.
echo Verify that %prog% is correctly installed and
echo is included in PATH environment variable
echo ------------------------------------------------------ 
echo.
goto END

endlocal
