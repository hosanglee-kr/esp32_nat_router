## 1. Context Alignment (요구사항 요약)
 * **환경:** Windows OS, VSCode, PlatformIO (PIO)
 * **목적:** 두 개의 무수정 펌웨어(Master, Repeater)를 선택 부팅하고 NVS를 격리하기 위한 **커스텀 부트로더 컴파일 환경 구축 및 빌드 방법** 상세 가이드.
 * **기술적 제약:** PlatformIO의 기본 아두이노(Arduino) 프레임워크는 부트로더 소스 컴파일을 숨겨두고 기컴파일된 바이너리를 사용함. 따라서 커스텀 부트로더를 빌드하기 위해서는 **PlatformIO 내에서 ESP-IDF 프레임워크를 사용하는 별도의 부트로더 전용 빌드 프로젝트**를 구성해야 함.
## 2. Sequential Reasoning Process (단계별 분석)
### [Thought #1] 컴파일 환경 구성 전략 (Environment Strategy)
 * **Thought:** 윈도우 환경의 PlatformIO에서 부트로더 소스코드를 수정하고 컴파일하는 가장 직관적인 방법 정의.
 * **MECE Analysis:**
   * *방안 A: 네이티브 ESP-IDF 도구창(CMD/PowerShell) 설치.* -> PlatformIO 외에 별도 툴체인을 중복 설치해야 하므로 윈도우 환경이 복잡해짐. (제외)
   * *방안 B: PlatformIO 내에 framework = espidf 프로젝트 신설.* -> 이미 설치된 PlatformIO의 컴파일러와 패키지 자산을 그대로 재활용하므로 환경 오염이 없고 가장 깔끔함. (채택)
 * **Reasoning:** 사용자가 이미 VSCode + PlatformIO 환경을 완비하고 있으므로, 별도의 툴체인 설치 없이 PIO 내에서 ESP-IDF 모드로 부트로더만 뽑아내는 구조가 최선임.
 * **Potential Risk:** 프로젝트 생성 시 ESP-IDF 버전에 따라 코드 매크로나 레지스터명이 소폭 다를 수 있으므로 ESP32-S3 표준 레지스터 제어 코드로 락인(Lock-in) 필요.
 * **Next Step:** 파티션 테이블 CSV를 바이너리(.bin)로 컴파일하는 도구 식별.
### [Thought #2] 파티션 테이블 컴파일 도구 확보 (CSV to BIN)
 * **Thought:** 윈도우 PlatformIO 패키지 디렉토리 내에서 파티션 변환 스크립트(gen_esp32part.py)의 정확한 위치 포인팅.
 * **MECE Analysis:**
   * PlatformIO의 패키지 경로는 일반적으로 C:\Users\<사용자명>\.platformio\packages\ 아래에 집중됨.
   * tool-esptoolpy/gen_esp32part.py 툴을 사용하여 커맨드라인에서 수동 변환 가이드라인 작성.
 * **Reasoning:** 부트로더 소스코드에 파티션 테이블을 바이너리 배열(C Array)로 임베딩해야 하므로, 이 변환이 선행되어야 함.
 * **Potential Risk:** 윈도우 사용자명에 공백이 있을 경우 커맨드라인 경로 인식 오류 가능성 -> 큰따옴표("") 처리 필수 안내.
 * **Next Step:** 바이너리를 C 언어 배열로 바꾸는 윈도우용 파이썬 스크립트 제공.
### [Thought #3] 부트로더 후킹(Hooking) 구조 설계
 * **Thought:** ESP-IDF v5.x 구조에서 시스템이 부팅되기 직전 가로채는 표준 훅(Hook) 메커니즘을 프로젝트 컴포넌트로 주입하는 방법 설계.
 * **MECE Analysis:**
   * 프로젝트 루트에 bootloader_components 폴더를 생성하면 ESP-IDF 빌드 시스템(CMake)이 부트로더 컴파일 시 이를 자동으로 병합함.
   * sdkconfig 설정을 통해 커스텀 부트로더 훅(CONFIG_BOOTLOADER_HOOKS=y) 활성화.
 * **Reasoning:** 이 방식을 쓰면 ESP-IDF 내부 핵심 코드를 직접 더럽히지 않고 완전히 독립된 커스텀 부트로더 구현 파일을 유지할 수 있음.
 * **Potential Risk:** ESP32-S3의 GPIO 4번 레지스터 주소 및 제어 코드가 구형 ESP32와 다르므로 S3 전용 매크로 사용 검증.
 * **Next Step:** 종합 솔루션 단계별 프로시저 구현.

## 3. Final Solution (상세 구축 및 빌드 방법)
전체 공정은 **[Phase 1: 파티션 바이너리 변환] ➔ [Phase 2: 부트로더 프로젝트 생성] ➔ [Phase 3: 후킹 코드 구현 및 빌드]** 순으로 진행됩니다.


### Phase 1. 파티션 테이블 바이너리 및 헥사 배열 생성
앞서 설계한 2종의 파티션 테이블 CSV 파일을 바이너리로 빌드한 후, C 코드에 삽입할 수 있는 헥사(Hex) 데이터로 변환해야 합니다.

**1. 임시 작업 폴더 생성**
 * 윈도우 바탕화면이나 편한 곳에 pt_build 폴더를 만들고 아래 두 파일을 작성합니다.
pt_master.csv (마스터 펌웨어용)
```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0x15000, 0x1000,
factory,  app,  factory, 0x20000, 0x1E0000,

```
pt_repeater.csv (리피터 펌웨어용)
```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0xF000,  0x6000,
phy_init, data, phy,     0x15000, 0x1000,
factory,  app,  factory, 0x200000,0x1E0000,

```
**2. 바이너리 컴파일 (Windows 커맨드 프롬프트 실행)**
PlatformIO가 설치한 파이썬과 툴을 활용하여 컴파일합니다. (명령어 입력 전 사용자명 부분 변경 필수)
```cmd
:: 1. 마스터 파티션 컴파일
"C:\Users\<사용자명>\.platformio\penv\Scripts\python.exe" "C:\Users\<사용자명>\.platformio\packages\tool-esptoolpy\gen_esp32part.py" pt_master.csv pt_master.bin

:: 2. 리피터 파티션 컴파일
"C:\Users\<사용자명>\.platformio\penv\Scripts\python.exe" "C:\Users\<사용자명>\.platformio\packages\tool-esptoolpy\gen_esp32part.py" pt_repeater.csv pt_repeater.bin

```
**3. 헥사 배열 변환 스크립트 실행**
동일 폴더에 bin_to_hex.py라는 이름으로 아래 파이썬 코드를 저장하고 실행(python bin_to_hex.py)합니다. 소스코드에 붙여넣을 데이터 배열이 텍스트로 자동 생성됩니다.
```python
def bin_to_c_array(filename, array_name):
    with open(filename, "rb") as f:
        data = f.read()
    # 3072바이트(패딩 포함 표준 파티션 크기) 맞추기
    data = data + b'\xFF' * (3072 - len(data))
    
    hex_str = ", ".join([f"0x{b:02X}" for b in data])
    formatted = ",\n    ".join([hex_str[i:i+96] for i in range(0, len(hex_str), 96)])
    
    print(f"const uint8_t {array_name}[3072] = {{\n    {formatted}\n}};\n")

bin_to_c_array("pt_master.bin", "pt_master_bin")
bin_to_c_array("pt_repeater.bin", "pt_repeater_bin")

```
*화면에 출력되는 구문을 메모장에 복사해 둡니다.*


### Phase 2. PlatformIO 부트로더 전용 프로젝트 생성
 1. VSCode를 열고 **PlatformIO Home** ➔ **New Project**를 클릭합니다.
 2. 설정을 다음과 같이 지정합니다:
   * **Name:** ESP32S3_Custom_Bootloader
   * **Board:** Espressif ESP32-S3-DevKitC-1-N8 (8MB Flash) 또는 보유하신 S3 보드 선택
   * **Framework:** **Espressif IoT Development Framework (ESP-IDF)**  *(※주의: Arduino 아님)*
 3. 프로젝트가 생성되면 platformio.ini 파일을 열어 아래와 같이 수정합니다.
platformio.ini
```ini
[env:esp32s3]
platform = espressif32
board = esp32s3
framework = espidf
monitor_speed = 115200

; 커스텀 부트로더 훅 구동을 위한 최적화 비활성화 및 설정 매칭
build_flags = 
    -DCONFIG_BOOTLOADER_HOOKS=1

```

### Phase 3. 소스코드 트리 구성 및 구현
프로젝트 구조를 커스텀 부트로더 빌드가 가능하도록 폴더를 수동 배치해야 합니다. 프로젝트 루트 디렉토리에 **bootloader_components** 폴더를 만드십시오.
**최종 디렉토리 구조:**
```text
ESP32S3_Custom_Bootloader/
├── .pio/
├── bootloader_components/
│   └── custom_hooks/
│       ├── CMakeLists.txt
│       └── bootloader_hooks.c
├── src/
│   └── main.c (비어있어도 됨, 컴파일 통과용)
├── platformio.ini
└── sdkconfig.defaults

```
**1. bootloader_components/custom_hooks/CMakeLists.txt 작성**
```cmake
idf_component_register(SRCS "bootloader_hooks.c"
                       INCLUDE_DIRS "."
                       BOOTLOADER_SUPPORT)

```
**2. bootloader_components/custom_hooks/bootloader_hooks.c 작성**
*여기에 Phase 1단계에서 파이썬으로 출력한 헥사 배열 데이터를 변수 자리에 치환하여 삽입합니다.*
```c
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

```
**3. sdkconfig.defaults 작성**
프로젝트 루트 폴더에 sdkconfig.defaults 파일을 만들고 아래 코드를 넣어 빌드 시스템에 훅 기능 활성화를 강제합니다.
```text
CONFIG_BOOTLOADER_HOOKS=y

```


### Phase 4. 컴파일 및 바이너리 추출
 1. 모든 파일이 준비되면 VSCode 하단 바의 **Check 아이콘(PlatformIO: Build)**을 클릭하여 빌드를 수행합니다.
 2. 컴파일이 성공하면 프로젝트 내 다음 경로에 목적 파일인 커스텀 부트로더가 생성됩니다.
   * **생성 경로:** [프로젝트폴더]/.pio/build/esp32s3/bootloader.bin
 3. 이 bootloader.bin 파일과 컴파일해 둔 2종의 무수정 펌웨어 바이너리를 준비하여 최종적으로 배포 시스템 하드웨어에 주입합니다.


##########

### Phase 5. bootload 및 펌웨어 업로드


```
### 3.3 ESP32-S3 배포 절차 (Flashing Workflow)
**주의:** ESP32-S3는 기존 ESP32와 달리 부트로더 오프셋이 0x1000이 아닌 0x0000입니다.
```bash
# 1. 칩 전체 초기화 (기존 잔여 NVS 데이터 등 충돌 방지)
esptool.py --chip esp32s3 erase_flash

# 2. 커스텀 부트로더 업로드 (S3는 0x0 번지)
esptool.py --chip esp32s3 write_flash 0x0 bootloader.bin

# 3. 마스터 펌웨어 및 리피터 펌웨어를 지정된 절대 주소에 직접 업로드
# (파티션 테이블은 부트로더가 부팅 시 자동 생성하므로 따로 업로드 생략 가능하거나 초기 A버전 업로드)
esptool.py --chip esp32s3 write_flash 0x20000 firmware_master.bin
esptool.py --chip esp32s3 write_flash 0x200000 firmware_repeater.bin

```
