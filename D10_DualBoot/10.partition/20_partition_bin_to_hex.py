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
