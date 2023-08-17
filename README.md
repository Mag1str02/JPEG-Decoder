# JPEG Decoder

JPEG Decoder is a small C++ library for fast jpeg decoding


## Usage

Library provides one main function:
* `Decode` - takes path to image file and returns Image class instance, that contains all info about decoded image (size, comment and RGB pixel values)

Usage example:

```c++
Image image;
try
{
    std::ifstream image_file("image_path");
    image = Decode(image_file);
} catch (const std::exception& e)
{
    std::cerr << "Failed to decode image due: " << e.what() << std::endl;
}   
```

## Build

Build process is very simple and will add 2 targets to your cmake project: 
* `jpeg_decoder` - library of jpeg decoder to link against
* `test_jpeg_decoder` - executable for jpeg decoder testing

To build library:

1. Start by installing library dependencies by `sudo apt install libjpeg-dev libfftw3-dev`
2. Then clone the repository with `git clone https://github.com/Mag1str02/JPEG-Decoder` or add it as a submodule by `git submodule add https://github.com/Mag1str02/JPEG-Decoder`

To run tests:

3. Create build directory by `mkdir build`
4. Move to build dir by `cd build`
5. Configure by `cmake ..`
6. Build by `cmake --build .` 

To use library:

3. Add library directory to your cmake project by `add_subdirectory([PATH_TO_JPEG_DECODER])`
4. Add library includes to your cmake target by `target_include_directories([YOUR_TARGET] ${JPEG_DECODER_INCLUDES})`
5. Link library to your cmake target by `target_link_libraries([YOUR_TARGET] jpeg_decoder)`