@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cl.exe /nologo /O2 /EHsc /Fe:NFS_Endless_Horizon.exe main.cpp /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib opengl32.lib glu32.lib winmm.lib
