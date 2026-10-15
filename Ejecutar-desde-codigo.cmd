@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\ejecutar-desde-codigo.ps1" %*
set "resultado=%ERRORLEVEL%"
if not "%resultado%"=="0" (
  echo.
  echo No se pudo preparar o ejecutar ConcurrentSimulator. Lee el mensaje anterior.
  pause
)
exit /b %resultado%
