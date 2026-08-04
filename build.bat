@echo off
echo Compiling resources...
windres resources.rc -o resources.o
if %errorlevel% neq 0 goto error

echo Compiling C code...
clang main.c -lole32 -luuid "-Wl,--gc-sections" -mwindows -s -Os resources.o -o centawin.exe
if %errorlevel% neq 0 goto error

echo Success! Cleaning up...
del resources.o
exit /b 0

:error
del resources.o
echo Build FAILED!
pause
exit /b 1
