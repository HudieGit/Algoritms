@echo off
chcp 65001 >nul
setlocal EnableExtensions
cd /d "%~dp0"

rem ==================================================================
rem  run_all.bat - сборка программ и все замеры 
rem ==================================================================

echo ============================================================
echo сборка и все замеры
echo ============================================================

for %%f in (search.h verify.cpp experiment.cpp input.txt) do (
    if not exist "%%f" (
        echo [ОШИБКА] Не найден файл %%f - положите run_all.bat в папку с исходниками.
        goto fail
    )
)

rem ---------- Поиск компилятора ----------
rem 1) cl уже доступен (запуск из Developer Command Prompt)
set "CXX="
where cl >nul 2>nul
if not errorlevel 1 set "CXX=cl"
if defined CXX goto have_compiler

rem 2) ищем Visual Studio через vswhere и подключаем её окружение
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto try_gpp
set "VSPATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH goto try_gpp
if not exist "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" goto try_gpp
echo Подключение окружения Visual Studio...
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0"
where cl >nul 2>nul
if not errorlevel 1 set "CXX=cl"
if defined CXX goto have_compiler

rem 3) запасной вариант - g++ (MinGW / MSYS2)
:try_gpp
where g++ >nul 2>nul
if not errorlevel 1 set "CXX=g++"
if defined CXX goto have_compiler

echo [ОШИБКА] Не найден компилятор C++: нет ни Visual Studio с компонентом C++, ни g++.
goto fail

rem ---------- 1. Сборка ----------
:have_compiler
echo.
echo [1/4] Сборка программ, компилятор: %CXX%
if /i "%CXX%"=="cl" goto build_cl
g++ -O2 -std=c++17 verify.cpp -o verify.exe || goto fail
g++ -O2 -std=c++17 experiment.cpp -o experiment.exe || goto fail
goto built
:build_cl
cl /nologo /O2 /EHsc /std:c++17 /utf-8 verify.cpp || goto fail
cl /nologo /O2 /EHsc /std:c++17 /utf-8 experiment.cpp || goto fail
del /q verify.obj experiment.obj 2>nul
:built

rem ---------- 2. Программа верификации ----------
echo.
echo [2/4] Программа верификации: verify.exe input.txt
verify.exe input.txt || goto fail
echo.
echo --- out_rabin_karp.txt ---
type out_rabin_karp.txt
echo --- out_kmp.txt ---
type out_kmp.txt

rem ---------- 3. Серии экспериментов 3.1-3.3 ----------
echo.
echo [3/4] Серии экспериментов. Серия 3.1 идёт около 5-10 минут.
experiment.exe series 3.1 series_3.1.csv 5 || goto fail
experiment.exe series 3.2 series_3.2.csv 7 || goto fail
experiment.exe series 3.3 series_3.3.csv 7 || goto fail

rem ---------- 4. Влияние мощности алфавита ----------
rem Интерактивный режим experiment.exe, ответы на вопросы подаются через конвейер:
rem 1 = свой эксперимент, %%q = мощность алфавита,
rem 2 = X случайный, 1000001 = длина X, 2 = Y случайный, 8 = длина Y
echo.
echo [4/4] Влияние мощности алфавита: 2, 4 и 26 букв
if exist alphabet.txt del alphabet.txt
for %%q in (2 4 26) do (
    echo ===== Алфавит из %%q букв, m = 1000001, n = 8 =====>>alphabet.txt
    (echo 1& echo %%q& echo 2& echo 1000001& echo 2& echo 8) | experiment.exe >>alphabet.txt
    echo.>>alphabet.txt
)
type alphabet.txt

echo.
echo ============================================================
echo  Готово. Созданы файлы:
echo    out_rabin_karp.txt, out_kmp.txt  - результаты верификации
echo    series_3.1.csv, series_3.2.csv, series_3.3.csv - данные для графиков
echo    alphabet.txt - замеры влияния мощности алфавита
echo ============================================================
pause
exit /b 0

:fail
echo.
echo [ОШИБКА] Выполнение прервано, подробности - в сообщениях выше.
pause
exit /b 1
