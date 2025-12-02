-- Premake5 Configuration
workspace "WarThunderAdvanced"
    architecture "x64"
    configurations { "Main" }
    startproject "WarThunderAdvanced"

project "WarThunderAdvanced"
    location "Build"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++latest"
    
    -- Visual Studio 2026 последний тулсет
    toolset "v145"
    
    -- Выходная директория для бинарников
    targetdir ("Bin/%{cfg.buildcfg}")
    
    -- Директория для промежуточных файлов
    objdir ("Build/Intermediate/%{prj.name}/%{cfg.buildcfg}")
    
    -- Исходные файлы проекта
    files {
        "Source/**.h",
        "Source/**.hpp",
        "Source/**.cpp",
        "Source/**.rc"
    }
    
    -- Явно указываем основные файлы для надежности
    files {
        "Source/main.cpp",
        "Source/UI.cpp",
        "Source/JsonParser.cpp"
    }
    
    -- ImGui файлы
    files {
        "vendor/imgui-master/imgui.cpp",
        "vendor/imgui-master/imgui_demo.cpp",
        "vendor/imgui-master/imgui_draw.cpp",
        "vendor/imgui-master/imgui_tables.cpp",
        "vendor/imgui-master/imgui_widgets.cpp",
        "vendor/imgui-master/backends/imgui_impl_win32.cpp",
        "vendor/imgui-master/backends/imgui_impl_dx11.cpp"
    }
    
    -- Включаемые директории
    includedirs {
        "Source",
        "font",
        "vendor/imgui-master",
        "vendor/imgui-master/backends"
    }
    
    -- Линковка DirectX11 + HTTP + WIC
    links {
        "d3d11",
        "dxgi",
        "d3dcompiler",
        "dwmapi",
        "wininet",
        "windowscodecs",
        "ole32"
    }

    -- Конфигурация Main (Release)
    filter "configurations:Main"
        defines { "NDEBUG" }
        symbols "Off"
        optimize "Full"
        runtime "Release"
        linktimeoptimization "On"

    -- Настройки для Windows
    filter "system:windows"
        systemversion "latest"
        defines { "PLATFORM_WINDOWS", "_CRT_SECURE_NO_WARNINGS", "WIN32_LEAN_AND_MEAN" }
        buildoptions { "/utf-8" }  -- UTF-8 кодировка для исходных файлов
        
    filter {}
