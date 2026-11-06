@echo off
chcp 65001 >nul
echo ============================================================
echo  Встановлення інструментів для ЛР1 (STM32 + Wokwi)
echo  Пакети з офіційних джерел через winget.
echo  Якщо з'явиться вікно UAC - натисніть "Так".
echo ============================================================
where winget >nul 2>nul || (echo winget не знайдено. Оновіть "App Installer" у Microsoft Store. & pause & exit /b 1)

set OPTS=--accept-package-agreements --accept-source-agreements --silent -e

echo [1/5] Git...
winget install --id Git.Git %OPTS%
echo [2/5] VS Code...
winget install --id Microsoft.VisualStudioCode %OPTS%
echo [3/5] GNU Make...
winget install --id GnuWin32.Make %OPTS%
echo [4/5] Docker Desktop (великий, ~600 МБ, може попросити перезавантаження)...
winget install --id Docker.DockerDesktop %OPTS%

echo [5/5] Плагін Wokwi для VS Code...
set "CODE=%LOCALAPPDATA%\Programs\Microsoft VS Code\bin\code.cmd"
if exist "%CODE%" (
  call "%CODE%" --install-extension wokwi.wokwi-vscode
  call "%CODE%" --install-extension ms-vscode.cpptools
) else (
  echo VS Code ще не в PATH - після перезапуску відкрийте цей файл ще раз.
)

echo.
echo Додайте Make у PATH (якщо команда make не знаходиться):
echo   C:\Program Files (x86)\GnuWin32\bin
echo.
echo ГОТОВО. Далі вручну:
echo  1) Перезавантажте ПК, якщо Docker попросив; запустіть Docker Desktop.
echo  2) Отримайте ключ Wokwi: https://wokwi.com/dashboard  (вхід через GitHub),
echo     у VS Code: F1 - "Wokwi: Request a new License".
echo  3) (Рекомендовано) STM32CubeMX: https://www.st.com/en/development-tools/stm32cubemx.html
pause
