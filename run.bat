@echo OFF
cls
SET mypath=%~dp0
pushd %mypath%\TEMP\
template-downloader.exe -p ld99 -o . -s Azenris/timer -v
popd