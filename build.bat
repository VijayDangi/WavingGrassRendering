
@echo off
if "%~1" == ""  goto BUILD
if "%~1" == "x64"  goto BUILDx64
if "%~1" == "link"  goto LINK
if "%~1" == "linkx64"  goto LINKx64
if "%~1" == "clean" goto CLEAN
else goto BUILD


:BUILD
    RC.exe Resource.rc

    CL.exe /c /EHsc ^
    /I "C:\glew-2.1.0\include" ^
    /I "C:\Program Files (x86)\IntelSWTools\system_studio_2020\OpenCL\sdk\include" ^
    /I "C:\Library\freetype\include" ^
    Main.cpp ^
    LoadShaders.cpp ^
    TextureLoading.cpp ^
    Geometry.cpp ^
    FreeType2DText.cpp

:LINK
    LINK.exe ^
    /OUT:App.exe ^
    /LIBPATH:"C:\glew-2.1.0\lib\Release\Win32" ^
    /LIBPATH:"C:\Program Files (x86)\IntelSWTools\system_studio_2020\OpenCL\sdk\lib\x86" ^
    /LIBPATH:"C:\Library\freetype\release dll\win32" ^
    Main.obj ^
    LoadShaders.obj ^
    TextureLoading.obj ^
    Geometry.obj ^
    FreeType2DText.obj ^
    Resource.res ^
    user32.lib ^
    gdi32.lib

    goto EXIT



:BUILDx64
    RC.exe Resource.rc

    CL.exe /c /EHsc ^
    /I "C:\glew-2.1.0\include" ^
    /I "C:\Program Files (x86)\IntelSWTools\system_studio_2020\OpenCL\sdk\include" ^
    /I "C:\Library\freetype\include" ^
    Main.cpp ^
    LoadShaders.cpp ^
    TextureLoading.cpp ^
    Geometry.cpp ^
    FreeType2DText.cpp


:LINKx64
    LINK.exe ^
    /OUT:App.exe ^
    /LIBPATH:"C:\glew-2.1.0\lib\Release\x64" ^
    /LIBPATH:"C:\Program Files (x86)\IntelSWTools\system_studio_2020\OpenCL\sdk\lib\x64" ^
    /LIBPATH:"C:\Library\freetype\release dll\win64" ^
    /MACHINE:x64 ^
    Main.obj ^
    LoadShaders.obj ^
    TextureLoading.obj ^
    Geometry.obj ^
    FreeType2DText.obj ^
    Resource.res ^
    user32.lib ^
    gdi32.lib

    goto EXIT



:CLEAN
    DEL App.exe ^
    Main.obj ^
    LoadShaders.obj ^
    TextureLoading.obj ^
    Geometry.obj ^
    FreeType2DText.obj ^
    Resource.res

    goto EXIT

:EXIT
@echo on
