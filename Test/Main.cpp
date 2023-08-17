#include "Decoder.h"
#include <jpeglib.h>

const std::string kBasePath = IMAGE_DIR;

Image ReadJpg(const std::string& filename)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr         err;
    FILE*                         infile = fopen(filename.c_str(), "rb");

    if (!infile)
    {
        throw std::runtime_error("can't open " + filename);
    }

    cinfo.err = jpeg_std_error(&err);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);

    (void)jpeg_read_header(&cinfo, static_cast<boolean>(true));
    (void)jpeg_start_decompress(&cinfo);

    int        row_stride = cinfo.output_width * cinfo.output_components;
    JSAMPARRAY buffer     = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);  // NOLINT

    Image  result(cinfo.output_width, cinfo.output_height);
    size_t y = 0;

    while (cinfo.output_scanline < cinfo.output_height)
    {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
        for (size_t x = 0; x < result.Width(); ++x)
        {
            RGB pixel;
            if (cinfo.output_components == 3)
            {
                pixel.r = buffer[0][x * 3];
                pixel.g = buffer[0][x * 3 + 1];
                pixel.b = buffer[0][x * 3 + 2];
            }
            else
            {
                pixel.r = pixel.g = pixel.b = buffer[0][x];
            }
            result.SetPixel(y, x, pixel);
        }
        ++y;
    }

    (void)jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return result;
}

double Distance(const RGB& lhs, const RGB& rhs)
{
    return sqrt((lhs.r - rhs.r) * (lhs.r - rhs.r) + (lhs.g - rhs.g) * (lhs.g - rhs.g) + (lhs.b - rhs.b) * (lhs.b - rhs.b));
}

bool Compare(const Image& actual, const Image& expected)
{
    double max  = 0;
    double mean = 0;
    if (actual.Width() != expected.Width() || actual.Height() != expected.Height())
    {
        return false;
    }
    for (size_t y = 0; y < actual.Height(); ++y)
    {
        for (size_t x = 0; x < actual.Width(); ++x)
        {
            auto actual_data   = actual.GetPixel(y, x);
            auto expected_data = expected.GetPixel(y, x);
            auto diff          = Distance(actual_data, expected_data);
            max                = std::max(max, diff);
            mean += diff;
        }
    }

    mean /= actual.Width() * actual.Height();
    return mean <= 5;
}

bool CheckImage(const std::string& filename, const std::string& expected_comment = "")
{
    std::ifstream fin(kBasePath + filename);
    if (!fin.is_open())
    {
        throw std::invalid_argument("Cannot open a file");
    }
    auto image = Decode(fin);
    fin.close();
    if (image.GetComment() != expected_comment)
    {
        return false;
    }
    auto ok_image = ReadJpg(kBasePath + filename);
    return Compare(image, ok_image);
}

bool TestImage(const std::string& filename, const std::string& expected_comment = "", bool expect_error = false)
{
    std::chrono::steady_clock::time_point begin, end;
    bool                                  result;
    std::string                           error = "Error in image was not found";
    begin                                       = std::chrono::steady_clock::now();
    try
    {
        if (!CheckImage(filename, expected_comment))
        {
            throw std::logic_error("Decoded image is not same as expected");
        }
        result = true;
    } catch (std::exception& e)
    {
        result = false;
        error  = e.what();
    }
    end = std::chrono::steady_clock::now();
    if (result ^ expect_error)
    {
        std::cout << "Test passed: (" << filename << ")"
                  << " Time: (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms)" << std::endl;
    }
    else
    {
        std::cout << "Test failed: (" << filename << ")"
                  << " Because: (" << error << ")" << std::endl;
    }
    return result ^ expect_error;
}

struct TestCase
{
    std::string file;
    std::string expected_comment = "";
    bool        expect_error     = false;
};

int main()
{
    std::vector<TestCase> test_cases = {
        {        "small.jpg",         ":)"},
        {        "lenna.jpg",           ""},
        {  "bad_quality.jpg", "so quality"},
        {         "tiny.jpg",           ""},
        {"chroma_halfed.jpg",           ""},
        {    "grayscale.jpg",           ""},
        {         "test.jpg",           ""},
        {       "colors.jpg",           ""},
        { "save_for_web.jpg",           ""},
        {   "prostitute.jpg",           ""},
        { "architecture.jpg",           ""},
        {        "witch.jpg",           ""},
        {         "huge.jpg",           ""},
    };
    const size_t tests_count = 24;
    for (size_t i = 1; i <= tests_count; ++i)
    {
        test_cases.push_back({"bad" + std::to_string(i) + ".jpg", "", true});
    }
    int failed = 0;
    for (const auto& test_case : test_cases)
    {
        if (!TestImage(test_case.file, test_case.expected_comment, test_case.expect_error))
        {
            ++failed;
        }
    }
    if (failed)
    {
        std::cout << failed << "/" << test_cases.size() << " test cases failed" << std::endl;
    }
    else
    {
        std::cout << "All " << test_cases.size() << " test cases passed" << std::endl;
    }
}
