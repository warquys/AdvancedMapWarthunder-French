@echo off
echo ========================================
echo    Cleaning project...
echo ========================================
echo.

:: Удаление Build (но не old_project)
if exist "Build" (
    echo Removing Build...
    rmdir /s /q "Build"
)

:: Защита old_project от удаления
if exist "old_project" (
    echo Skipping old_project (protected)...
)

:: Удаление .sln файлов
if exist "*.sln" (
    echo Removing .sln files...
    del /q "*.sln" 2>nul
)

:: Удаление .vcxproj файлов (если есть в корне)
if exist "*.vcxproj" (
    echo Removing .vcxproj files...
    del /q "*.vcxproj" 2>nul
    del /q "*.vcxproj.filters" 2>nul
    del /q "*.vcxproj.user" 2>nul
)

:: Удаление .vs папки
if exist ".vs" (
    echo Removing .vs...
    rmdir /s /q ".vs"
)

echo.
echo ========================================
echo    Clean complete!
echo ========================================
echo.
pause

