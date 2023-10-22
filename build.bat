@echo OFF
tools\timer.exe start

cls

:: Automatically runs C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
if NOT defined VSCMD_ARG_TGT_ARCH (
	call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

:: MUST be after the enviroment setup
SETLOCAL EnableDelayedExpansion

SET debugMode=1
SET platform=PLATFORM_WINDOWS
SET name=template-downloader
SET buildDir=TEMP\
SET objectDir=%buildDir%Objects\
SET warnings=-WX -W4 -wd4100 -wd4201 -wd4324
SET includes=-Ithird_party\ -Ithird_party\libzip\
SET defines=-DC_PLUS_PLUS -D_CRT_SECURE_NO_WARNINGS -DLITTLE_ENDIAN -D%platform% -DPLATFORM_ENGINE="\"%platform%\"" -DCURL_STATICLIB -DZIP_STATIC
SET links=-link Normaliz.lib Ws2_32.lib Wldap32.lib Crypt32.lib advapi32.lib
SET linker=-subsystem:console
SET flags=-std:c++20 -Zc:preprocessor -Zc:strictStrings -GR- -EHsc

if not exist %buildDir% ( mkdir %buildDir% )
if not exist %objectDir% ( mkdir %objectDir% )

if %debugMode% == 1 (
	SET warnings=%warnings% -wd4189
	SET defines=%defines% -DDEBUG
	SET includes=%includes% -Ithird_party\libcurl\debug\64\include\
	SET links=%links% ^
		third_party\zlib\zlibd64.lib ^
		third_party\libcurl\debug\64\lib\libcurl_a_debug.lib ^
		third_party\libzip\debug\64\zip.lib
	SET flags=%flags% -Z7 -FC -MDd
) else (
	SET includes=%includes% -Ithird_party\libcurl\release\64\include\
	SET links=%links% ^
		third_party\zlib\zlib64.lib ^
		third_party\libcurl\release\64\lib\libcurl_a.lib ^
		third_party\libzip\release\64\zip.lib
	SET flags=%flags% -MD -O2
)

cl -nologo %flags% %warnings% %defines% -Fe%buildDir%%name%.exe -Fo%objectDir% src\main.cpp %includes% %links% -INCREMENTAL:NO %linker%

:build_success
echo Build success!
goto build_end

:build_failed
echo Build failed.
goto build_end

:build_end
tools\timer.exe end