#include "ppm_io.hpp"

#include <cstdint>
#include <cstdlib>
#include <locale>
#include <print>
#include <sstream>

static int failed = 0;

static void check(bool cond, const char* test_name) {
    if (!cond) {
        std::println(stderr, "FAIL: {}", test_name);
        ++failed;
    } else {
        std::println("PASS: {}", test_name);
    }
}

static void check_pixel(const Pixel& pixel, uint8_t expected_r, uint8_t expected_g,
                        uint8_t expected_b, const char* test_name) {
    if (pixel.r != expected_r || pixel.g != expected_g || pixel.b != expected_b) {
        std::println(stderr, "FAIL: {} -- got ({},{},{}) expected ({},{},{})",
                     test_name, pixel.r, pixel.g, pixel.b,
                     expected_r, expected_g, expected_b);
        ++failed;
    } else {
        std::println("PASS: {}", test_name);
    }
}

// -------------------------------------------------------------------
// Read tests
// -------------------------------------------------------------------

static void test_empty_input() {
    auto input_stream = std::istringstream("");
    auto read_result = ppm_read(input_stream);
    check(!read_result.value.has_value(), "empty input -> error");
    if (!read_result.value.has_value())
        check(read_result.value.error() == PpmReadError::kEmptyInput,
              "empty input -> kEmptyInput");
}

static void test_magic_bad() {
    auto input_stream = std::istringstream("P5\n1 1\n255\n0 0 0\n");
    auto read_result = ppm_read(input_stream);
    check(!read_result.value.has_value(), "P5 -> error");
    if (!read_result.value.has_value())
        check(read_result.value.error() == PpmReadError::kBadMagic, "P5 -> kBadMagic");
}

static void test_magic_truncated() {
    auto input_stream = std::istringstream("P");
    auto read_result = ppm_read(input_stream);
    check(!read_result.value.has_value(), "just P -> error");
    if (!read_result.value.has_value())
        check(read_result.value.error() == PpmReadError::kBadMagic, "just P -> kBadMagic");
}

static void test_valid_1x1() {
    auto input_stream = std::istringstream("P3\n1 1\n255\n128 128 128\n");
    auto read_result = ppm_read(input_stream);
    check(read_result.value.has_value(), "valid 1x1 -> ok");
    if (read_result.value.has_value()) {
        const auto& image = *read_result.value;
        check(image.width == 1, "1x1: width");
        check(image.height == 1, "1x1: height");
        check(image.max_val == 255, "1x1: max_val");
        check(image.pixels.size() == 1, "1x1: pixel count");
        check_pixel(image.pixels[0], 128, 128, 128, "1x1: pixel value");
    }
}

static void test_valid_2x2() {
    auto input_stream = std::istringstream("P3\n2 2\n255\n"
                                           "0 0 0 255 0 0\n"
                                           "0 255 0 0 0 255\n");
    auto read_result = ppm_read(input_stream);
    check(read_result.value.has_value(), "valid 2x2 -> ok");
    if (read_result.value.has_value()) {
        const auto& image = *read_result.value;
        check(image.width == 2, "2x2: width");
        check(image.height == 2, "2x2: height");
        check(image.pixels.size() == 4, "2x2: pixel count");
        check_pixel(image.pixels[0], 0, 0, 0, "2x2: pixel 0");
        check_pixel(image.pixels[1], 255, 0, 0, "2x2: pixel 1");
        check_pixel(image.pixels[2], 0, 255, 0, "2x2: pixel 2");
        check_pixel(image.pixels[3], 0, 0, 255, "2x2: pixel 3");
    }
}

static void test_comments_in_header() {
    auto input_stream = std::istringstream("P3\n# width height\n2 2\n# before maxval\n255\n"
                                           "0 0 0\n0 0 0\n0 0 0\n0 0 0\n");
    auto read_result = ppm_read(input_stream);
    check(read_result.value.has_value(), "comments in header -> ok");
    if (read_result.value.has_value()) {
        const auto& image = *read_result.value;
        check(image.width == 2, "comments: width");
        check(image.height == 2, "comments: height");
        check(image.pixels.size() == 4, "comments: pixel count");
    }
}

static void test_maxval_not_255() {
    auto input_stream = std::istringstream("P3\n1 1\n100\n0 0 0\n");
    auto read_result = ppm_read(input_stream);
    check(!read_result.value.has_value(), "maxval=100 -> error");
    if (!read_result.value.has_value())
        check(read_result.value.error() == PpmReadError::kBadNumber,
              "maxval=100 -> kBadNumber");
}

static void test_width_zero() {
    auto input_stream = std::istringstream("P3\n0 1\n255\n0 0 0\n");
    auto read_result = ppm_read(input_stream);
    check(!read_result.value.has_value(), "width=0 -> error");
    if (!read_result.value.has_value())
        check(read_result.value.error() == PpmReadError::kBadNumber, "width=0 -> kBadNumber");
}

static void test_channel_out_of_range() {
    auto input_stream = std::istringstream("P3\n1 1\n255\n256 0 0\n");
    auto read_result = ppm_read(input_stream);
    check(!read_result.value.has_value(), "channel 256 -> error");
    if (!read_result.value.has_value())
        check(read_result.value.error() == PpmReadError::kChannelRange,
              "channel 256 -> kChannelRange");
}

static void test_channel_negative() {
    auto input_stream = std::istringstream("P3\n1 1\n255\n-1 0 0\n");
    auto read_result = ppm_read(input_stream);
    check(!read_result.value.has_value(), "channel -1 -> error");
    if (!read_result.value.has_value())
        check(read_result.value.error() == PpmReadError::kChannelRange,
              "channel -1 -> kChannelRange");
}

static void test_not_a_number() {
    auto input_stream = std::istringstream("P3\n1 1\n255\nx 0 0\n");
    auto read_result = ppm_read(input_stream);
    check(!read_result.value.has_value(), "not a number -> error");
    if (!read_result.value.has_value())
        check(read_result.value.error() == PpmReadError::kBadNumber,
              "not a number -> kBadNumber");
}

static void test_hash_in_data() {
    auto input_stream = std::istringstream("P3\n1 1\n255\n0 0 0 # trailing\n");
    auto read_result = ppm_read(input_stream);
    check(!read_result.value.has_value(), "hash in data -> error");
    if (!read_result.value.has_value())
        check(read_result.value.error() == PpmReadError::kBadNumber,
              "hash in data -> kBadNumber");
}

static void test_too_many_pixels() {
    auto input_stream = std::istringstream("P3\n1 1\n255\n0 0 0 255 255 255\n");
    auto read_result = ppm_read(input_stream);
    check(!read_result.value.has_value(), "extra pixel -> error");
    if (!read_result.value.has_value())
        check(read_result.value.error() == PpmReadError::kTooManyPixels,
              "extra -> kTooManyPixels");
}

static void test_too_few_pixels() {
    auto input_stream = std::istringstream("P3\n2 2\n255\n0 0 0 255 0 0 0 255 0\n");
    auto read_result = ppm_read(input_stream);
    check(!read_result.value.has_value(), "missing pixel -> error");
    if (!read_result.value.has_value())
        check(read_result.value.error() == PpmReadError::kTooFewPixels,
              "missing -> kTooFewPixels");
}

// -------------------------------------------------------------------
// Writer tests
// -------------------------------------------------------------------

static void test_writer_basic() {
    auto output_stream = std::ostringstream();
    {
        PpmWriter writer(output_stream, 2, 2);
        writer.put(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(0));
        writer.put(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0));
        writer.put(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(0));
        writer.put(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
    }
    auto output = output_stream.str();
    check(!output.empty(), "writer produced output");
    check(output.find("P3") == 0, "writer starts with P3");
    check(output.find("2 2") != std::string::npos, "writer contains dimensions");
    check(output.find("255") != std::string::npos, "writer contains maxval");
}

static void test_writer_read_roundtrip() {
    auto output_stream = std::ostringstream();
    {
        PpmWriter writer(output_stream, 2, 2);
        writer.put(static_cast<uint8_t>(10), static_cast<uint8_t>(20), static_cast<uint8_t>(30));
        writer.put(static_cast<uint8_t>(40), static_cast<uint8_t>(50), static_cast<uint8_t>(60));
        writer.put(static_cast<uint8_t>(70), static_cast<uint8_t>(80), static_cast<uint8_t>(90));
        writer.put(static_cast<uint8_t>(100), static_cast<uint8_t>(110), static_cast<uint8_t>(120));
    }
    auto serialized_ppm = output_stream.str();
    auto roundtrip_input = std::istringstream(serialized_ppm);
    auto read_result = ppm_read(roundtrip_input);

    check(read_result.value.has_value(), "roundtrip: read back ok");
    if (read_result.value.has_value()) {
        const auto& image = *read_result.value;
        check(image.width == 2, "roundtrip: width");
        check(image.height == 2, "roundtrip: height");
        check(image.pixels.size() == 4, "roundtrip: pixel count");
        check_pixel(image.pixels[0], 10, 20, 30, "roundtrip: pixel 0");
        check_pixel(image.pixels[1], 40, 50, 60, "roundtrip: pixel 1");
        check_pixel(image.pixels[2], 70, 80, 90, "roundtrip: pixel 2");
        check_pixel(image.pixels[3], 100, 110, 120, "roundtrip: pixel 3");
    }
}

static void test_writer_format_no_trailing_space() {
    auto output_stream = std::ostringstream();
    {
        PpmWriter writer(output_stream, 3, 1);
        writer.put(static_cast<uint8_t>(1), static_cast<uint8_t>(2), static_cast<uint8_t>(3));
        writer.put(static_cast<uint8_t>(4), static_cast<uint8_t>(5), static_cast<uint8_t>(6));
        writer.put(static_cast<uint8_t>(7), static_cast<uint8_t>(8), static_cast<uint8_t>(9));
    }
    auto output = output_stream.str();
    auto newline_pos = output.rfind('\n');
    auto last_row = (newline_pos != std::string::npos)
                        ? output.substr(output.rfind('\n', newline_pos - 1) + 1)
                        : output;
    check(!last_row.empty() && last_row.back() == '\n', "last row ends with newline");
    if (!last_row.empty() && last_row.back() == '\n')
        last_row.pop_back();

    check(last_row.find("  ") == std::string::npos ||
          last_row.find("  1   2   3   4   5   6   7   8   9") != std::string::npos,
          "format: aligned values");
}

// -------------------------------------------------------------------
// main
// -------------------------------------------------------------------

int main() {
    std::setlocale(LC_ALL, "Russian_Russia.1251");

    std::println("--- ppm_read tests ---");

    std::println("-- header errors --");
    test_empty_input();
    test_magic_bad();
    test_magic_truncated();
    test_valid_1x1();
    test_maxval_not_255();
    test_width_zero();
    test_comments_in_header();

    std::println("-- pixel data errors --");
    test_channel_out_of_range();
    test_channel_negative();
    test_not_a_number();
    test_hash_in_data();
    test_too_many_pixels();
    test_too_few_pixels();
    test_valid_2x2();

    std::println("-- writer --");
    test_writer_basic();
    test_writer_read_roundtrip();
    test_writer_format_no_trailing_space();

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}
