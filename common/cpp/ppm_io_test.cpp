#include "ppm_io.hpp"

#include <cstdlib>
#include <locale>
#include <print>
#include <sstream>
#include <string_view>

static int failed = 0;

static void check(bool cond, std::string_view name) {
    if (!cond) {
        std::println(stderr, "FAIL: {}", name);
        ++failed;
    } else {
        std::println("PASS: {}", name);
    }
}

static void check_pixel(const Pixel& p, uint8_t er, uint8_t eg, uint8_t eb, std::string_view name) {
    if (p.r != er || p.g != eg || p.b != eb) {
        std::println(stderr, "FAIL: {} -- got ({},{},{}) expected ({},{},{})", name, p.r, p.g, p.b,
                     er, eg, eb);
        ++failed;
    } else {
        std::println("PASS: {}", name);
    }
}

// -------------------------------------------------------------------
// Read tests
// -------------------------------------------------------------------

static void test_empty_input() {
    auto ss = std::istringstream("");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "empty input -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kEmptyInput, "empty input -> kEmptyInput");
}

static void test_magic_bad() {
    auto ss = std::istringstream("P5\n1 1\n255\n0 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "P5 -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadMagic, "P5 -> kBadMagic");
}

static void test_magic_truncated() {
    auto ss = std::istringstream("P");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "just P -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadMagic, "just P -> kBadMagic");
}

static void test_valid_1x1() {
    auto ss = std::istringstream("P3\n1 1\n255\n128 128 128\n");
    auto r = Image::read(ss);
    check(r.value.has_value(), "valid 1x1 -> ok");
    if (r.value.has_value()) {
        const auto& img = *r.value;
        check(img.width() == 1, "1x1: width");
        check(img.height() == 1, "1x1: height");
        check(img.max_val() == 255, "1x1: max_val");
        check(img.pixel_count() == 1, "1x1: pixel count");
        check_pixel(img.pixels()[0], 128, 128, 128, "1x1: pixel value");
    }
}

static void test_valid_2x2() {
    auto ss = std::istringstream("P3\n2 2\n255\n"
                                 "0 0 0 255 0 0\n"
                                 "0 255 0 0 0 255\n");
    auto r = Image::read(ss);
    check(r.value.has_value(), "valid 2x2 -> ok");
    if (r.value.has_value()) {
        const auto& img = *r.value;
        check(img.width() == 2, "2x2: width");
        check(img.height() == 2, "2x2: height");
        check(img.pixel_count() == 4, "2x2: pixel count");
        check_pixel(img.pixels()[0], 0, 0, 0, "2x2: pixel 0");
        check_pixel(img.pixels()[1], 255, 0, 0, "2x2: pixel 1");
        check_pixel(img.pixels()[2], 0, 255, 0, "2x2: pixel 2");
        check_pixel(img.pixels()[3], 0, 0, 255, "2x2: pixel 3");
    }
}

static void test_comments_in_header() {
    auto ss = std::istringstream("P3\n# width height\n2 2\n# before maxval\n255\n"
                                 "0 0 0\n0 0 0\n0 0 0\n0 0 0\n");
    auto r = Image::read(ss);
    check(r.value.has_value(), "comments in header -> ok");
    if (r.value.has_value()) {
        const auto& img = *r.value;
        check(img.width() == 2, "comments: width");
        check(img.height() == 2, "comments: height");
        check(img.pixel_count() == 4, "comments: pixel count");
    }
}

static void test_maxval_not_255() {
    auto ss = std::istringstream("P3\n1 1\n100\n0 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "maxval=100 -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadNumber, "maxval=100 -> kBadNumber");
}

static void test_width_zero() {
    auto ss = std::istringstream("P3\n0 1\n255\n0 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "width=0 -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadNumber, "width=0 -> kBadNumber");
}

static void test_channel_out_of_range() {
    auto ss = std::istringstream("P3\n1 1\n255\n256 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "channel 256 -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kChannelRange, "channel 256 -> kChannelRange");
}

static void test_channel_negative() {
    auto ss = std::istringstream("P3\n1 1\n255\n-1 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "channel -1 -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kChannelRange, "channel -1 -> kChannelRange");
}

static void test_not_a_number() {
    auto ss = std::istringstream("P3\n1 1\n255\nx 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "not a number -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadNumber, "not a number -> kBadNumber");
}

static void test_hash_in_data() {
    auto ss = std::istringstream("P3\n1 1\n255\n0 0 0 # trailing\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "hash in data -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadNumber, "hash in data -> kBadNumber");
}

static void test_too_many_pixels() {
    auto ss = std::istringstream("P3\n1 1\n255\n0 0 0 255 255 255\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "extra pixel -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kTooManyPixels, "extra -> kTooManyPixels");
}

static void test_too_few_pixels() {
    auto ss = std::istringstream("P3\n2 2\n255\n0 0 0 255 0 0 0 255 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "missing pixel -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kTooFewPixels, "missing -> kTooFewPixels");
}

// -------------------------------------------------------------------
// Writer tests
// -------------------------------------------------------------------

static void test_writer_basic() {
    auto ss = std::ostringstream();
    {
        PpmWriter pw(ss, 2, 2);
        pw.put(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(0));
        pw.put(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0));
        pw.put(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(0));
        pw.put(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
    }
    auto out = ss.str();
    check(!out.empty(), "writer produced output");
    check(out.find("P3") == 0, "writer starts with P3");
    check(out.find("2 2") != std::string::npos, "writer contains dimensions");
    check(out.find("255") != std::string::npos, "writer contains maxval");
}

static void test_writer_read_roundtrip() {
    auto ss = std::ostringstream();
    {
        PpmWriter pw(ss, 2, 2);
        pw.put(static_cast<uint8_t>(10), static_cast<uint8_t>(20), static_cast<uint8_t>(30));
        pw.put(static_cast<uint8_t>(40), static_cast<uint8_t>(50), static_cast<uint8_t>(60));
        pw.put(static_cast<uint8_t>(70), static_cast<uint8_t>(80), static_cast<uint8_t>(90));
        pw.put(static_cast<uint8_t>(100), static_cast<uint8_t>(110), static_cast<uint8_t>(120));
    }
    auto written = ss.str();

    auto is = std::istringstream(written);
    auto r = Image::read(is);
    check(r.value.has_value(), "roundtrip: read back ok");
    if (r.value.has_value()) {
        const auto& img = *r.value;
        check(img.width() == 2, "roundtrip: width");
        check(img.height() == 2, "roundtrip: height");
        check(img.pixel_count() == 4, "roundtrip: pixel count");
        check_pixel(img.pixels()[0], 10, 20, 30, "roundtrip: pixel 0");
        check_pixel(img.pixels()[1], 40, 50, 60, "roundtrip: pixel 1");
        check_pixel(img.pixels()[2], 70, 80, 90, "roundtrip: pixel 2");
        check_pixel(img.pixels()[3], 100, 110, 120, "roundtrip: pixel 3");
    }
}

static void test_writer_format_no_trailing_space() {
    auto ss = std::ostringstream();
    {
        PpmWriter pw(ss, 3, 1);
        pw.put(static_cast<uint8_t>(1), static_cast<uint8_t>(2), static_cast<uint8_t>(3));
        pw.put(static_cast<uint8_t>(4), static_cast<uint8_t>(5), static_cast<uint8_t>(6));
        pw.put(static_cast<uint8_t>(7), static_cast<uint8_t>(8), static_cast<uint8_t>(9));
    }
    auto out = ss.str();
    auto nl = out.rfind('\n');
    auto last_line = (nl != std::string::npos) ? out.substr(out.rfind('\n', nl - 1) + 1) : out;
    check(!last_line.empty() && last_line.back() == '\n', "last line ends with newline");
    // Remove trailing newline for format check
    if (!last_line.empty() && last_line.back() == '\n')
        last_line.pop_back();

    check(last_line.find("  ") == std::string::npos ||
              last_line.find("  1   2   3   4   5   6   7   8   9") != std::string::npos,
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
