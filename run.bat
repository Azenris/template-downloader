@echo OFF
cls
SET mypath=%~dp0
pushd %mypath%\TEMP\
template-downloader.exe
popd