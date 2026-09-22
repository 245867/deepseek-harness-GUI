@echo off
setlocal
REM Resolve repository root from script location
set "REPO_ROOT=%~dp0.."
call "D:\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
"D:\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "%REPO_ROOT%\jade-frontend\DSHStudio.vcxproj" /p:Configuration=Release /p:Platform=x64 /m /v:minimal
if errorlevel 1 exit /b %errorlevel%
copy /Y "%REPO_ROOT%\jade-frontend\DSHStudio.exe" "%REPO_ROOT%\DSH-GUI.exe" >nul
if errorlevel 1 (
  echo Failed to publish DSH-GUI.exe. Close the running GUI and retry.
  exit /b 1
)
copy /Y "%REPO_ROOT%\jade-frontend\JadeView_x64.dll" "%REPO_ROOT%\JadeView_x64.dll" >nul
if errorlevel 1 exit /b 1
echo Published: %REPO_ROOT%\DSH-GUI.exe
