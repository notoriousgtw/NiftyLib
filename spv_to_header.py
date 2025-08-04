# spv_to_header.py

import sys
import os

def spv_to_header(spv_path, header_path):
    with open(spv_path, "rb") as f:
        data = f.read()
    file_name = os.path.basename(spv_path)
    var_name = os.path.splitext(file_name)[0].replace('.', '_')
    print (f"Converting {file_name} to header...")

    bytes_per_line = 19
    array_lines = [
        ', '.join(f'0x{b:02x}' for b in data[i:i+bytes_per_line])
        for i in range(0, len(data), bytes_per_line)
    ]
    array_elements = ',\n\t'.join(array_lines)
    array_len = len(data)

    header_guard = os.path.basename(header_path).replace('.', '_').upper()

    with open(header_path, "w") as f:
        f.write(f"#pragma once\n\n")
        f.write(f"// Generated from {file_name}\n")
        f.write(f"static const unsigned char {var_name}[] = {{\n\t{array_elements}\n}};\n")
        f.write(f"static const unsigned int {var_name}_len = {array_len};\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python spv_to_header.py <input.spv> <output.h> <var_name>")
        sys.exit(1)
    spv_to_header(sys.argv[1], sys.argv[2])
