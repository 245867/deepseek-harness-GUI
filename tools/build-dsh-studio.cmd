@echo off
setlocal
chcp 65001 >nul
REM 从脚本所在位置推导仓库根目录
set "REPO_ROOT=%~dp0.."

REM Visual Studio 安装位置可通过环境变量覆盖，默认使用本机已装的 2019 社区版
if not defined VS_DEV_CMD set "VS_DEV_CMD=D:\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
if not defined VS_MSBUILD set "VS_MSBUILD=D:\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"

if not exist "%VS_DEV_CMD%" (
  echo 找不到 Visual Studio 开发环境脚本：%VS_DEV_CMD%
  echo 请先设置 VS_DEV_CMD 环境变量，指向 VsDevCmd.bat。
  exit /b 1
)

call "%VS_DEV_CMD%" -arch=x64 -host_arch=x64
"%VS_MSBUILD%" "%REPO_ROOT%\jade-frontend\DSHStudio.vcxproj" /p:Configuration=Release /p:Platform=x64 /m /v:minimal
if errorlevel 1 exit /b %errorlevel%
copy /Y "%REPO_ROOT%\jade-frontend\DSHStudio.exe" "%REPO_ROOT%\DSH-GUI.exe" >nul
if errorlevel 1 (
  echo 发布 DSH-GUI.exe 失败，请先关闭正在运行的界面再重试。
  exit /b 1
)
copy /Y "%REPO_ROOT%\jade-frontend\JadeView_x64.dll" "%REPO_ROOT%\JadeView_x64.dll" >nul
if errorlevel 1 exit /b 1
echo 已发布：%REPO_ROOT%\DSH-GUI.exe
