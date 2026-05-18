:: 1. 마스터 파티션 컴파일
"C:\Users\<사용자명>\.platformio\penv\Scripts\python.exe" "C:\Users\<사용자명>\.platformio\packages\tool-esptoolpy\gen_esp32part.py" pt_master.csv pt_master.bin

:: 2. 리피터 파티션 컴파일
"C:\Users\<사용자명>\.platformio\penv\Scripts\python.exe" "C:\Users\<사용자명>\.platformio\packages\tool-esptoolpy\gen_esp32part.py" pt_repeater.csv pt_repeater.bin
