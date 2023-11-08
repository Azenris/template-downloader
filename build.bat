@echo OFF
cls

SET zlib=C:/vcpkg/packages/zlib_x64-windows-static-md
SET bzip2=C:/vcpkg/packages/bzip2_x64-windows-static-md
SET libzip=C:/vcpkg/packages/libzip_x64-windows-static-md
SET curl=C:/vcpkg/packages/curl_x64-windows-static-md

cmake -S . -B build ^
	-DZLIB_INCLUDE_DIR=%zlib%/include/ ^
	-DZLIB_LIBRARY_RELEASE=%zlib%/lib/zlib.lib ^
	-DZLIB_LIBRARY_DEBUG=%zlib%/debug/lib/zlibd.lib ^
	-DBZIP2_INCLUDE_DIR=%bzip2%/include/ ^
	-DBZIP2_LIBRARY_RELEASE=%bzip2%/lib/bz2.lib ^
	-DBZIP2_LIBRARY_DEBUG=%bzip2%/debug/lib/bz2d.lib ^
	-DLIBZIP_DIR=%libzip%/share/libzip/ ^
	-DCURL_DIR=%curl%/share/curl/

cmake --build build --config=Release