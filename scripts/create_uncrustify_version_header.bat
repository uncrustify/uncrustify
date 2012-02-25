@echo off
rem Filter the package version number from the configure file

set configuration_file="..\\configure"
set package_version_token=PACKAGE_VERSION

FOR /F "tokens=2 delims='" %%A IN ('findstr /B /R "^%package_version_token%=.*" %configuration_file%') DO set package_version=%%A

rem Delete existing header_output_file and create new empty one

set header_output_file="..\\src\\uncrustify_version.h"
del /F /Q %header_output_file% > NUL
copy /y NUL %header_output_file% > NUL

rem Copy line by line from header template file to header output file and replace
rem the package version placeholder with the version number

set header_template_file="..\\src\\uncrustify_version.h.in"
set package_version_placeholder=@PACKAGE_VERSION@

for /f "tokens=1,* delims=]" %%A in ('"type %header_template_file%|find /n /v """') do (
set "line=%%B"
if defined line (
    call set "line=echo.%%line:%package_version_placeholder%=%package_version%%%"
    for /f "delims=" %%X in ('"echo."%%line%%""') do %%~X >> %header_output_file%
    ) ELSE echo. >> %header_output_file%
)

