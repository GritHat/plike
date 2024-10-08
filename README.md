A simple plike to C converter.
Initial release, very limited conversion and syntax support.
It contains a plike_code.txt to showcase syntax and available features.

requirements:
gcc
cmake

installation:
git clone https://github.com/GritHat/plike
cd plike
make

usage:
./plike2c < input_plike_code.anyextension > output_c_file.c

it can also be manually compiled because it still is one single file:
git clone https://github.com/GritHat/plike
cd plike/src
gcc plike.c -o plike
./plike < input_plike_code.anyextension > output_c_file.c

