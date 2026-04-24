@echo off

REM === 설정 파일 경로 ===
set CONFIG_FILE="Doxyfile"

echo Doxygen Starts....
doxygen  %CONFIG_FILE%

echo Doxygen Finish

REM === HTML 결과 열기 ===
start "" "Output\html\index.html"

pause
exit