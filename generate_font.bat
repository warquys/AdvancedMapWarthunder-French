@echo off
REM Font Embedding - Information Script
REM ====================================
REM 
REM PROCESS EXPLANATION:
REM ---------------------
REM The font is embedded into exe using Windows Resources (.rc file)
REM 
REM Files involved:
REM 1. font\symbols_skyquake.ttf - Source font file
REM 2. Source\font_resource.rc - Windows resource file (embeds font into exe)
REM 3. Source\FontEmbedded.h - Header with functions to load font from resources
REM 
REM HOW IT WORKS:
REM -------------
REM 1. During compilation, the .rc file embeds font\symbols_skyquake.ttf 
REM    into the exe as a Windows resource (ID=1, type=RCDATA)
REM 
REM 2. At runtime, FontEmbedded.h loads the font data from Windows resources
REM    using FindResource() and LoadResource() API calls
REM 
REM 3. ImGui loads the font from memory using AddFontFromMemoryTTF()
REM 
REM RESULT:
REM -------
REM The font is embedded INSIDE the exe file
REM No external font file needed!
REM 
REM ====================================

echo.
echo ========================================
echo Font Embedding - Information
echo ========================================
echo.
echo Process: Windows Resources (.rc file)
echo.
echo Files:
echo   - font\symbols_skyquake.ttf (source)
echo   - Source\font_resource.rc (embeds font into exe)
echo   - Source\FontEmbedded.h (loads font from resources)
echo.
echo During compilation:
echo   1. .rc file embeds font into exe as Windows resource
echo   2. Font becomes part of the executable
echo.
echo At runtime:
echo   1. FontEmbedded.h loads font from Windows resources
echo   2. ImGui uses the font from memory
echo.
echo Result: Font is INSIDE the exe file!
echo.
echo ========================================
echo.
echo No action needed - just compile the project!
echo.
pause
