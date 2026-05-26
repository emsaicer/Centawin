@echo off
echo Compiling resources...
windres resources.rc -o resources.o
if %errorlevel% neq 0 goto error

echo Compiling C code...
gcc main.c resources.o -o centawin.exe -mwindows -s
if %errorlevel% neq 0 goto error

echo Success! Cleaning up...
del resources.o
exit /b 0

:error
echo Build FAILED!
pause
exit /b 1
