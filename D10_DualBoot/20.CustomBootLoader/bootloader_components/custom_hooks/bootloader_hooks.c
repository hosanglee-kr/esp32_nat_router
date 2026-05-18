#include <string.h>
#include "bootloader_flash_priv.h"
#include "esp_rom_gpio.h"
#include "soc/gpio_reg.h"
#include "soc/io_mux_reg.h"
#include "esp_log.h"

// Phase 1에서 생성된 헥사 배열 대입
const uint8_t pt_master_bin[3072] = {
    /* [여기에 파이썬이 출력한 pt_master_bin 데이터 붙여넣기] */
};

const uint8_t pt_repeater_bin[3072] = {
    /* [여기에 파이썬이 출력한 pt_repeater_bin 데이터 붙여넣기] */
};

// ESP32-S3 부팅 시 가장 먼저 호출되는 훅 함수
void bootloader_before_init(void) {
    // 1. GPIO 4번 초기화 및 풀업 설정 (ESP32-S3 저수준 레지스터 제어)
    esp_rom_gpio_pad_select_gpio(4);
    
    // Input Enable 활성화 및 Pull-Up 인게이지
    REG_SET_BIT(IO_MUX_GPIO4_REG, FUN_IE);
    REG_SET_BIT(IO_MUX_GPIO4_REG, FUN_PU);
    REG_CLR_BIT(IO_MUX_GPIO4_REG, FUN_PD);
    
    esp_rom_delay_us(5000); // 전위 안정화 대기

    // 2. GPIO 4번 핀의 입력 상태 읽기
    uint32_t gpio_level = (REG_READ(GPIO_IN_REG) >> 4) & 0x1;

    // 3. 타겟 파티션 결정
    const uint8_t* target_pt = (gpio_level == 0) ? pt_master_bin : pt_repeater_bin;

    // 4. 플래시 메모리 0x8000(파티션 테이블 주소) 데이터 판독
    uint8_t current_pt[3072];
    bootloader_flash_read(0x8000, current_pt, 3072, true);

    // 5. 현재 플래시 상태와 매칭할 타겟이 다른 경우에만 섹터 삭제 후 라이팅
    if (memcmp(current_pt, target_pt, 3072) != 0) {
        // 0x8000 번지는 섹터 번호로 8번 섹터임 (0x8000 / 4096 = 8)
        bootloader_flash_erase_sector(8); 
        bootloader_flash_write(0x8000, (void*)target_pt, 3072, true);
    }
}

void bootloader_after_init(void) {
    // 빈 상태로 유지 (필수 인터페이스 훅)
}

