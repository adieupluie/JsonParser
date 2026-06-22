@echo off
cd /d C:\workspace\VisualCode\JSONTreeViewer
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
  call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
  msbuild JSONTreeViewer.sln /p:Configuration=Release /p:Platform=x64
  goto :eof
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\Tools\VsDevCmd.bat" (
  call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\Tools\VsDevCmd.bat"
  msbuild JSONTreeViewer.sln /p:Configuration=Release /p:Platform=x64
  goto :eof
)
msbuild JSONTreeViewer.sln /p:Configuration=Release /p:Platform=x64
