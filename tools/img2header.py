# img2header.py
# (C) 2023 Instant Arcade (www.InstantArcade.com)
# Free to use and duplicate/modify (MIT license)
import argparse
from PIL import Image
import numpy as np

def rgb888_to_rgb565(r, g, b):
    # Convert (R, G, B) from 8 bits each to 5, 6, 5 bits respectively
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

def swapbytes( val_in ):
    return ((val_in&0xff)<<8) | (val_in >> 8)

def image_to_cpp_header(image_path, header_path, header_name, use_rgb565, swap, mono):
    # Load the image with Pillow
    with Image.open(image_path) as img:
        # Convert the image to RGB format
        if mono:
            img = img.convert('1')
        else:
            img = img.convert('RGB')
        
        # Get the pixel data
        pixel_data = np.array(img)
        height, width = pixel_data.shape[:2]
        
        # Generate the C++ header content
        cpp_header_content = f"#ifndef IMAGE_DATA_{header_name}_H\n#define IMAGE_DATA_{header_name}_H\n\n"

        if mono:
            width = int(width/8)

        cpp_header_content += f"const int image_width_{header_name} = {width};\nconst int image_height_{header_name} = {height};\n\n"

        if use_rgb565 or swap:
            cpp_header_content += f"const uint16_t image_data_{header_name}[] = "+"{\n"
        elif mono:
            cpp_header_content += f"const unsigned char image_data_{header_name}[] = "+"{\n"
        else:
            cpp_header_content += "struct RGB {\n    unsigned char r;\n    unsigned char g;\n    unsigned char b;\n};\n\n"
            cpp_header_content += f"const RGB image_data_{header_name}[] = "+"{\n"
        
        for y in range(height):
            # byte = 0
            # bytecount = 0
            for x in range(width):
                if mono:
                    byte = 0
                    for n in range(8):
                        b = pixel_data[y,x*8+n]
                        byte <<= 1
                        byte |= b
                        # bytecount += 1
                        # if bytecount == 8:
                    cpp_header_content += f"    0x{byte:02x},"
                            # bytecount = 0
                else:
                    r, g, b = pixel_data[y, x]
                    if use_rgb565 or swap:
                        rgb565 = rgb888_to_rgb565(r, g, b)
                        if swap:
                            rgb565 = swapbytes(rgb565)
                            
                        cpp_header_content += f"    0x{rgb565:04x},"
                    else:
                        cpp_header_content += f"    {{0x{r:02x}, 0x{g:02x}, 0x{b:02x}}},"
            cpp_header_content += "\n"
        
        cpp_header_content = cpp_header_content.rstrip(',\n') + "\n};\n\n"+f"#endif // IMAGE_DATA_{header_name}_H\n"
        
        # Write the header content to the specified header file path
        with open(header_path, 'w') as header_file:
            header_file.write(cpp_header_content)

def main():
    parser = argparse.ArgumentParser(description='Convert an image to a C++ header file with RGB pixel data.')
    parser.add_argument('input_image', help='Input image file path.')
    parser.add_argument('output_header', help='Output C++ header file path.')
    parser.add_argument('header_name', help='identifier for header name.')
    parser.add_argument('--mono', action='store_true', help='Output the pixel data as 1 bit monochrome.')
    parser.add_argument('--rgb565', action='store_true', help='Output the pixel data in RGB565 format instead of RGB888.')
    parser.add_argument('--rgb565swapped', action='store_true', help='Output the pixel data in RGB565 with swapped bytes instead of RGB888.')

    args = parser.parse_args()

    image_to_cpp_header(args.input_image, args.output_header, args.header_name, args.rgb565, args.rgb565swapped, args.mono)

if __name__ == '__main__':
    main()

