@echo off
setlocal

:: Settings
set VCPKG_ROOT=%USERPROFILE%\vcpkg
set BUILD_DIR=build
set CMAKE_VERSION=3.29.0
set CMAKE_ZIP=cmake-%CMAKE_VERSION%-windows-x86_64.zip
set CMAKE_URL=https://github.com/Kitware/CMake/releases/download/v%CMAKE_VERSION%/%CMAKE_ZIP%
set CMAKE_LOCAL_DIR=%~dp0cmake-portable

:: 1. Check for Config
if not exist config.json (
    echo [WARNING] config.json not found!
    echo Please rename config.example.json to config.json and fill in your details.
    exit /b 1
)

:: 2. Check for CMake (System or Local)
set CMAKE_EXE=cmake
where cmake >nul 2>nul
if %errorlevel% equ 0 (
    echo [INFO] System CMake found.
) else (
    if exist "%CMAKE_LOCAL_DIR%\bin\cmake.exe" (
        echo [INFO] Portable CMake found.
        set "CMAKE_EXE=%CMAKE_LOCAL_DIR%\bin\cmake.exe"
    ) else (
        echo [INFO] CMake not found. Downloading portable CMake %CMAKE_VERSION%...
        
        :: Download
        powershell -Command "Invoke-WebRequest -Uri '%CMAKE_URL%' -OutFile '%CMAKE_ZIP%'"
        if not exist "%CMAKE_ZIP%" (
            echo [ERROR] Failed to download CMake. Please install it manually.
            exit /b 1
        )
        
        :: Extract
        echo [INFO] Extracting CMake...
        powershell -Command "Expand-Archive -Path '%CMAKE_ZIP%' -DestinationPath '%~dp0' -Force"
        
        :: Rename/Move to clean folder
        move "cmake-%CMAKE_VERSION%-windows-x86_64" "%CMAKE_LOCAL_DIR%"
        del "%CMAKE_ZIP%"
        
        if exist "%CMAKE_LOCAL_DIR%\bin\cmake.exe" (
            set "CMAKE_EXE=%CMAKE_LOCAL_DIR%\bin\cmake.exe"
            echo [INFO] CMake installed locally to %CMAKE_LOCAL_DIR%
        ) else (
            echo [ERROR] Extraction failed.
            exit /b 1
        )
    )
)

:: 3. Check/Install vcpkg
if not exist "%VCPKG_ROOT%" (
    echo [INFO] vcpkg not found at %VCPKG_ROOT%. Cloning...
    git clone https://github.com/microsoft/vcpkg.git "%VCPKG_ROOT%"
    call "%VCPKG_ROOT%\bootstrap-vcpkg.bat"
)

:: 4. Configure CMake with vcpkg
echo [INFO] Installing dependencies via vcpkg (this may take a while)...
call "%VCPKG_ROOT%\vcpkg.exe" install --triplet x64-windows
if %errorlevel% neq 0 (
    echo [ERROR] vcpkg install failed.
    exit /b %errorlevel%
)

echo [INFO] Configuring CMake using %CMAKE_EXE%...
"%CMAKE_EXE%" -B %BUILD_DIR% -S . "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=x64-windows"
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    exit /b %errorlevel%
)

:: 5. Build
echo [INFO] Building project...
"%CMAKE_EXE%" --build %BUILD_DIR% --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    exit /b %errorlevel%
)

:: 6. Run
echo [INFO] Starting Bot...
"%BUILD_DIR%\Release\mcbot.exe"

endlocal
