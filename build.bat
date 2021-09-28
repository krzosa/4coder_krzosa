@echo off
pushd %~dp0
call ..\custom\bin\buildsuper_x64-win.bat kr_init.cpp %1
popd
