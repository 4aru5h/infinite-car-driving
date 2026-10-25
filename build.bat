@echo off
setlocal enabledelayedexpansion

echo [INFO] Locating Visual Studio MSVC environment...

if defined VCINSTALLDIR (
    goto :compile
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "!VSWHERE!" (
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_DIR=%%i"
    )
    if defined VS_DIR (
        if exist "!VS_DIR!\VC\Auxiliary\Build\vcvars64.bat" (
            call "!VS_DIR!\VC\Auxiliary\Build\vcvars64.bat"
            goto :compile
        )
    )
)

:: Fallback standard locations
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    goto :compile
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    goto :compile
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    goto :compile
)
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
    goto :compile
)

echo [ERROR] Could not find Visual Studio vcvars64.bat. Please run from Developer Command Prompt for VS.
exit /b 1

:compile
echo [INFO] Compiling NFS_Endless_Horizon.exe with MSVC...
cl.exe /nologo /O2 /EHsc /Fe:NFS_Endless_Horizon.exe main.cpp /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib opengl32.lib glu32.lib winmm.lib
if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] Build completed successfully: NFS_Endless_Horizon.exe
) else (
    echo [ERROR] Compilation failed with error code %ERRORLEVEL%
)
