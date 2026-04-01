@echo off
REM MTProxy Secrets Update Script for Windows
REM This script automatically updates Telegram secrets and configuration

setlocal enabledelayedexpansion

REM Configuration
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "CONFIG_DIR=%PROJECT_DIR%\config"
set "BACKUP_DIR=%PROJECT_DIR%\backups"
set "LOG_DIR=%PROJECT_DIR%\logs"
set "LOG_FILE=%LOG_DIR%\update-secrets.log"

REM URLs
set "SECRET_URL=https://core.telegram.org/getProxySecret"
set "CONFIG_URL=https://core.telegram.org/getProxyConfig"

REM Files
set "SECRET_FILE=%CONFIG_DIR%\proxy-secret"
set "CONFIG_FILE=%CONFIG_DIR%\proxy-multi.conf"

REM Create directories
if not exist "%CONFIG_DIR%" mkdir "%CONFIG_DIR%"
if not exist "%BACKUP_DIR%" mkdir "%BACKUP_DIR%"
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

REM Get timestamp
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set "TIMESTAMP=%datetime:~0,4%-%datetime:~4,2%-%datetime:~6,2% %datetime:~8,2%:%datetime:~10,2%:%datetime:~12,2%"
set "BACKUP_TIMESTAMP=%datetime:~0,4%%datetime:~4,2%%datetime:~6,2%_%datetime:~8,2%%datetime:~10,2%%datetime:~12,2%"

echo [%TIMESTAMP%] [INFO] Starting secrets update process >> "%LOG_FILE%"
echo Starting secrets update process...

REM Backup existing files
if exist "%SECRET_FILE%" (
    copy "%SECRET_FILE%" "%BACKUP_DIR%\proxy-secret.%BACKUP_TIMESTAMP%" >nul
    echo [%TIMESTAMP%] [INFO] Backed up proxy-secret >> "%LOG_FILE%"
    echo Backed up proxy-secret
)

if exist "%CONFIG_FILE%" (
    copy "%CONFIG_FILE%" "%BACKUP_DIR%\proxy-multi.conf.%BACKUP_TIMESTAMP%" >nul
    echo [%TIMESTAMP%] [INFO] Backed up proxy-multi.conf >> "%LOG_FILE%"
    echo Backed up proxy-multi.conf
)

REM Download new secret
echo Downloading new proxy-secret...
curl -f -s -S "%SECRET_URL%" -o "%SECRET_FILE%.tmp"
if %errorlevel% equ 0 (
    move /y "%SECRET_FILE%.tmp" "%SECRET_FILE%" >nul
    echo [%TIMESTAMP%] [INFO] Successfully updated proxy-secret >> "%LOG_FILE%"
    echo Successfully updated proxy-secret
) else (
    echo [%TIMESTAMP%] [ERROR] Failed to download proxy-secret >> "%LOG_FILE%"
    echo ERROR: Failed to download proxy-secret
    del "%SECRET_FILE%.tmp" 2>nul
    exit /b 1
)

REM Download new config
echo Downloading new proxy-multi.conf...
curl -f -s -S "%CONFIG_URL%" -o "%CONFIG_FILE%.tmp"
if %errorlevel% equ 0 (
    move /y "%CONFIG_FILE%.tmp" "%CONFIG_FILE%" >nul
    echo [%TIMESTAMP%] [INFO] Successfully updated proxy-multi.conf >> "%LOG_FILE%"
    echo Successfully updated proxy-multi.conf
) else (
    echo [%TIMESTAMP%] [ERROR] Failed to download proxy-multi.conf >> "%LOG_FILE%"
    echo ERROR: Failed to download proxy-multi.conf
    del "%CONFIG_FILE%.tmp" 2>nul
    exit /b 1
)

REM Cleanup old backups (keep last 7 days)
echo Cleaning up old backups...
forfiles /p "%BACKUP_DIR%" /m proxy-secret.* /d -7 /c "cmd /c del @path" 2>nul
forfiles /p "%BACKUP_DIR%" /m proxy-multi.conf.* /d -7 /c "cmd /c del @path" 2>nul

REM Check for Docker
where docker-compose >nul 2>&1
if %errorlevel% equ 0 (
    if exist "%PROJECT_DIR%\docker-compose.yml" (
        echo Restarting Docker container...
        cd /d "%PROJECT_DIR%"
        docker-compose restart
        if %errorlevel% equ 0 (
            echo [%TIMESTAMP%] [INFO] Docker container restarted successfully >> "%LOG_FILE%"
            echo Docker container restarted successfully
        ) else (
            echo [%TIMESTAMP%] [ERROR] Failed to restart Docker container >> "%LOG_FILE%"
            echo ERROR: Failed to restart Docker container
        )
    )
) else (
    echo [%TIMESTAMP%] [WARNING] Docker not found. Please restart MTProxy manually. >> "%LOG_FILE%"
    echo WARNING: Docker not found. Please restart MTProxy manually.
)

echo [%TIMESTAMP%] [INFO] Secrets update completed >> "%LOG_FILE%"
echo.
echo Secrets update completed successfully!
echo   Secret file: %SECRET_FILE%
echo   Config file: %CONFIG_FILE%
echo   Backups: %BACKUP_DIR%
echo   Log file: %LOG_FILE%

endlocal
