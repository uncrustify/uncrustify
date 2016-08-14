@echo off
python tests\run_tests.py %*
exit /b %ERRORLEVEL%
