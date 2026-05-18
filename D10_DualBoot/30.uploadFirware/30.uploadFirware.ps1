# 1. 칩 전체 초기화 (기존 잔여 NVS 데이터 등 충돌 방지)
esptool.py --chip esp32s3 erase_flash

# 2. 커스텀 부트로더 업로드 (S3는 0x0 번지)
esptool.py --chip esp32s3 write_flash 0x0 bootloader.bin

# 3. 마스터 펌웨어 및 리피터 펌웨어를 지정된 절대 주소에 직접 업로드
# (파티션 테이블은 부트로더가 부팅 시 자동 생성하므로 따로 업로드 생략 가능하거나 초기 A버전 업로드)
esptool.py --chip esp32s3 write_flash 0x20000 firmware_master.bin
esptool.py --chip esp32s3 write_flash 0x200000 firmware_repeater.bin
