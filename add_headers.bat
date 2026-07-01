bat_code = """@echo off
setlocal enabledelayedexpansion

set "HEADER_FILE=%~dp0temp_header_file.txt"
set "TEMP_FILE=%~dp0temp_combined_file.txt"

:: Write header to a temporary file
echo // Copyright 2026 FlowCompute LLC> "%HEADER_FILE%"
echo //>> "%HEADER_FILE%"
echo // This file is part of FlowCompute.>> "%HEADER_FILE%"
echo //>> "%HEADER_FILE%"
echo // FlowCompute is free software: you can redistribute it and/or modify>> "%HEADER_FILE%"
echo // it under the terms of the GNU Lesser General Public License as published by>> "%HEADER_FILE%"
echo // the Free Software Foundation, either version 3 of the License, or>> "%HEADER_FILE%"
echo // ^(at your option^) any later version.>> "%HEADER_FILE%"
echo //>> "%HEADER_FILE%"
echo // FlowCompute is distributed in the hope that it will be useful,>> "%HEADER_FILE%"
echo // but WITHOUT ANY WARRANTY; without even the implied warranty of>> "%HEADER_FILE%"
echo // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the>> "%HEADER_FILE%"
echo // GNU Lesser General Public License for more details.>> "%HEADER_FILE%"
echo //>> "%HEADER_FILE%"
echo // You should have received a copy of the GNU Lesser General Public License>> "%HEADER_FILE%"
echo // along with FlowCompute. If not, see ^<https://www.gnu.org/licenses/^>.>> "%HEADER_FILE%"
echo.>> "%HEADER_FILE%"

echo Applying headers to .cpp and .h files...

:: Loop through all .cpp and .h files recursively
for /r %%F in (*.cpp *.h) do (
    :: Check if the path contains \third_party\
    echo %%F | findstr /i "\\third_party\\" >nul
    if errorlevel 1 (
        echo Processing: %%F
        :: Combine header and original file
        type "%HEADER_FILE%" > "%TEMP_FILE%"
        type "%%F" >> "%TEMP_FILE%"
        :: Replace original with combined
        move /y "%TEMP_FILE%" "%%F" >nul
    ) else (
        echo Skipping: %%F
    )
)

:: Clean up
if exist "%HEADER_FILE%" del "%HEADER_FILE%"
if exist "%TEMP_FILE%" del "%TEMP_FILE%"

echo Complete!
pause
"""

with open("add_lgpl_headers.bat", "w", newline='\r\n') as f:
    f.write(bat_code)