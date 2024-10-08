A simple plike to C converter.
Initial release, very limited conversion and syntax support.
It contains a plike_code.txt to showcase syntax and available features.

## Requirements:
gcc

cmake

## Installation:
```
git clone https://github.com/GritHat/plike
cd plike
make
```

## Usage:
```
./plike2c < input_plike_code.anyextension > output_c_file.c
```
it can also be manually compiled because it still is one single file:
```
git clone https://github.com/GritHat/plike
cd plike/src
gcc plike.c -o plike
./plike < input_plike_code.anyextension > output_c_file.c
```
or you can compile it with any compiler you want, but redirect the input_plike_code file to the stdin, and output_c_code file to stdout


## Conventions:
<details>
  <summary>P-like syntax conversion</summary>
    ## Operators:
    NOT !
    AND &&
    OR ||
    ASSIGNMENT :=
    EQUALITY ==
    DISEQUALITY !=
    
    ## Arrays:
    0-indexed
    [n] to access the element at position n starting from position 0

    ## Program, Function, Procedure:
    program will be translated to "int main()"
    function will be translated to int function(), char function(), bool function() or float function() depending on the return type
    procedure will be translated to void procedure()

    ## Declaration, Definition
    we will declare all the variables before the actual logic


</details>