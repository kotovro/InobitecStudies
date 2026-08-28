#include "ppm_io.hpp"

#include <array>
#include <cstdlib>
#include <ostream>
#include <print>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>

#include "luma.hpp"

using namespace raster::common;

namespace {

int failed = 0;

void check(bool cond, std::string_view name) {
    if (!cond) {
        std::println(stderr, "FAIL: {}", name);
        ++failed;
    } else {
        std::println("PASS: {}", name);
    }
}

void check_pixel(const Pixel& p, uint8_t er, uint8_t eg, uint8_t eb, std::string_view name) {
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

void test_empty_input() {
    auto ss = std::istringstream("");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "empty input -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kEmptyInput, "empty input -> kEmptyInput");
}

void test_magic_bad() {
    auto ss = std::istringstream("P5\n1 1\n255\n0 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "P5 -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadMagic, "P5 -> kBadMagic");
}

void test_magic_truncated() {
    auto ss = std::istringstream("P");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "just P -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadMagic, "just P -> kBadMagic");
}

void test_valid_1x1() {
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

void test_valid_2x2() {
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

void test_comments_in_header() {
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

void test_maxval_not_255() {
    auto ss = std::istringstream("P3\n1 1\n100\n0 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "maxval=100 -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadNumber, "maxval=100 -> kBadNumber");
}

void test_width_zero() {
    auto ss = std::istringstream("P3\n0 1\n255\n0 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "width=0 -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadNumber, "width=0 -> kBadNumber");
}

void test_channel_out_of_range() {
    auto ss = std::istringstream("P3\n1 1\n255\n256 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "channel 256 -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kChannelRange, "channel 256 -> kChannelRange");
}

void test_channel_negative() {
    auto ss = std::istringstream("P3\n1 1\n255\n-1 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "channel -1 -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kChannelRange, "channel -1 -> kChannelRange");
}

void test_not_a_number() {
    auto ss = std::istringstream("P3\n1 1\n255\nx 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "not a number -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadNumber, "not a number -> kBadNumber");
}

void test_hash_in_data() {
    auto ss = std::istringstream("P3\n1 1\n255\n0 0 0 # trailing\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "hash in data -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kBadNumber, "hash in data -> kBadNumber");
}

void test_too_many_pixels() {
    auto ss = std::istringstream("P3\n1 1\n255\n0 0 0 255 255 255\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "extra pixel -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kTooManyPixels, "extra -> kTooManyPixels");
}

void test_too_few_pixels() {
    auto ss = std::istringstream("P3\n2 2\n255\n0 0 0 255 0 0 0 255 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "missing pixel -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kTooFewPixels, "missing -> kTooFewPixels");
}

void test_error_line() {
    auto ss = std::istringstream("P3\n1 1\n255\nx 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "not a number -> error");
    check(r.line == 4, "bad number reported on line 4");

    ss = std::istringstream("P3\n1 1\n255\n256 0 0\n");
    r = Image::read(ss);
    check(!r.value.has_value(), "channel 256 -> error");
    check(r.line == 4, "channel range reported on line 4");

    ss = std::istringstream("P3\n1 1\n100\n0 0 0\n");
    r = Image::read(ss);
    check(!r.value.has_value(), "maxval=100 -> error");
    check(r.line == 3, "maxval reported on line 3");
}

void test_alloc_error() {
    auto ss = std::istringstream("P3\n3000000000 3000000000\n255\n0 0 0\n");
    auto r = Image::read(ss);
    check(!r.value.has_value(), "huge dims -> error, not crash");
    if (!r.value.has_value())
        check(r.value.error() == PpmReadError::kAllocError, "huge dims -> kAllocError");
}

// -------------------------------------------------------------------
// luma tests
// -------------------------------------------------------------------

void test_luma() {
    auto y = luma(255, 0, 0);
    check(y >= 76.2 && y <= 76.3, "luma(255,0,0) ~ 76.245");
    y = luma(0, 255, 0);
    check(y >= 149.6 && y <= 149.7, "luma(0,255,0) ~ 149.685");
    y = luma(0, 0, 255);
    check(y >= 29.0 && y <= 29.1, "luma(0,0,255) ~ 29.07");
    y = luma(255, 255, 255);
    check(y >= 254.9 && y <= 255.1, "luma(255,255,255) == 255.0");
    y = luma(0, 0, 0);
    check(y >= -0.1 && y <= 0.1, "luma(0,0,0) == 0.0");
    y = luma(128, 128, 128);
    check(y >= 127.9 && y <= 128.1, "luma(128,128,128) ~ 128.0");
}

// -------------------------------------------------------------------
// Writer tests
// -------------------------------------------------------------------

void test_writer_basic() {
    auto ss = std::ostringstream();
    {
        PpmWriter pw(ss, 2, 2);
        check(pw.putHeader().value.has_value(), "writer header ok");
        auto pixels = std::to_array<Pixel>({{0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255}});
        auto r = pw.putAll(pixels, /*finalize=*/true);
        check(r.value.has_value(), "writer basic putAll ok");
    }
    auto out = ss.str();
    check(!out.empty(), "writer produced output");
    check(out.find("P3") == 0, "writer starts with P3");
    check(out.find("2 2") != std::string::npos, "writer contains dimensions");
    check(out.find("255") != std::string::npos, "writer contains maxval");
}

void test_writer_read_roundtrip() {
    auto ss = std::ostringstream();
    {
        PpmWriter pw(ss, 2, 2);
        check(pw.putHeader().value.has_value(), "roundtrip: header ok");
        auto pixels =
            std::to_array<Pixel>({{10, 20, 30}, {40, 50, 60}, {70, 80, 90}, {100, 110, 120}});
        auto r = pw.putAll(pixels, /*finalize=*/true);
        check(r.value.has_value(), "roundtrip: putAll ok");
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

void test_writer_format_no_trailing_space() {
    auto ss = std::ostringstream();
    {
        PpmWriter pw(ss, 3, 1);
        check(pw.putHeader().value.has_value(), "format: header ok");
        auto pixels = std::to_array<Pixel>({{1, 2, 3}, {4, 5, 6}, {7, 8, 9}});
        auto r = pw.putAll(pixels, /*finalize=*/true);
        check(r.value.has_value(), "format: putAll ok");
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

void test_writer_put_before_header() {
    auto ss = std::ostringstream();
    PpmWriter pw(ss, 2, 2);
    auto pixels = std::to_array<Pixel>({{0, 0, 0}});
    auto r = pw.putAll(pixels, /*finalize=*/true);
    check(!r.value.has_value(), "putAll before header -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmWriteError::kIOError, "putAll before header -> kIOError");
    check(ss.str().empty(), "putAll before header -> nothing written");
}

void test_writer_too_many_pixels() {
    auto ss = std::ostringstream();
    PpmWriter pw(ss, 2, 1);
    check(pw.putHeader().value.has_value(), "overflow: header ok");
    auto pixels = std::to_array<Pixel>({{0, 0, 0}, {1, 1, 1}, {2, 2, 2}});
    auto r = pw.putAll(pixels, /*finalize=*/true);
    check(!r.value.has_value(), "overflow -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmWriteError::kTooManyPixels, "overflow -> kTooManyPixels");
}

void test_writer_1x1() {
    auto ss = std::ostringstream();
    PpmWriter pw(ss, 1, 1);
    check(pw.putHeader().value.has_value(), "1x1: header ok");

    auto pixels = std::to_array<Pixel>({{7, 8, 9}});
    auto first = pw.putAll(pixels, /*finalize=*/true);
    check(first.value.has_value(), "1x1: first pixel ok");
    check(ss.str().find("  7   8   9") != std::string::npos, "1x1: pixel written");

    auto second = pw.putAll(pixels, /*finalize=*/true);
    check(!second.value.has_value(), "1x1: second pixel -> error");
    if (!second.value.has_value())
        check(second.value.error() == PpmWriteError::kTooManyPixels,
              "1x1: second pixel -> kTooManyPixels");
}

void test_writer_not_enough_pixels() {
    auto ss = std::ostringstream();
    PpmWriter pw(ss, 2, 2);
    check(pw.putHeader().value.has_value(), "not enough: header ok");
    auto pixels = std::to_array<Pixel>({{0, 0, 0}, {1, 1, 1}, {2, 2, 2}});
    auto r = pw.putAll(pixels, /*finalize=*/true);
    check(!r.value.has_value(), "not enough -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmWriteError::kNotEnoughPixels, "not enough -> kNotEnoughPixels");
}

void test_writer_finalize_false_no_check() {
    auto ss = std::ostringstream();
    PpmWriter pw(ss, 2, 2);
    check(pw.putHeader().value.has_value(), "finalize=false: header ok");
    auto pixels = std::to_array<Pixel>({{0, 0, 0}, {1, 1, 1}, {2, 2, 2}});
    auto r = pw.putAll(pixels, /*finalize=*/false);
    check(r.value.has_value(), "finalize=false -> no error");
}

void test_writer_stream_error() {
    auto ss = std::ostringstream();
    ss.setstate(std::ios::badbit);
    PpmWriter pw(ss, 2, 2);
    auto r = pw.putHeader();
    check(!r.value.has_value(), "bad stream header -> error");
    if (!r.value.has_value())
        check(r.value.error() == PpmWriteError::kIOError, "bad stream header -> kIOError");
}

// Buffers all writes but fails at flush (sync() -> -1): emulates a pipe that
// closed while the output was still buffered.
class BrokenPipeBuf : public std::streambuf {
  public:
    int_type overflow(int_type c) override {
        if (c != traits_type::eof())
            data_ += static_cast<char>(c);
        return traits_type::not_eof(c);
    }
    int sync() override { return -1; }

  private:
    std::string data_;
};

void test_writer_flush_error() {
    BrokenPipeBuf sbuf;
    std::ostream os(&sbuf);
    PpmWriter pw(os, 1, 1);
    auto h = pw.putHeader();
    check(h.value.has_value(), "flush error: header ok");
    auto pixels = std::to_array<Pixel>({{1, 2, 3}});
    auto r = pw.putAll(pixels, /*finalize=*/true);
    check(r.value.has_value(), "flush error: putAll ok while buffered");
    auto f = pw.flush();
    check(!f.value.has_value(), "flush error: flush detects broken stream");
    if (!f.value.has_value())
        check(f.value.error() == PpmWriteError::kIOError, "flush error -> kIOError");
}

// -------------------------------------------------------------------
// main
// -------------------------------------------------------------------

} // namespace

int main() {
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
    test_error_line();
    test_alloc_error();

    std::println("-- luma --");
    test_luma();

    std::println("-- writer --");
    test_writer_basic();
    test_writer_read_roundtrip();
    test_writer_format_no_trailing_space();
    test_writer_put_before_header();
    test_writer_too_many_pixels();
    test_writer_1x1();
    test_writer_not_enough_pixels();
    test_writer_finalize_false_no_check();
    test_writer_stream_error();
    test_writer_flush_error();

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}