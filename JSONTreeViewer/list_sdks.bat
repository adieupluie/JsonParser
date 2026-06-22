@echo off
if exist "C:\Program Files (x86)\Windows Kits\10\Include" (
  dir /b "C:\Program Files (x86)\Windows Kits\10\Include"
) else echo NO_WIN10_SDK
