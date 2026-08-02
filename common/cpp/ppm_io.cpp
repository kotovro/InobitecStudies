#include "ppm_io.hpp"

#include <cerrno>
#include <iostream>
#include <limits>
#include <memory>
#include <print>
#include <system_error>
#include <vector>

struct Image::Impl {
    int32_t width{};
    int32_t height{};
    uint16_t max_val{};
    std::vector<Pixel> pixels;
};

Image::Image() : _impl(std::make_unique<Impl>()) {}

Image::~Image() = default;

Image::Image(Image&&) noexcept = default;

Image& Image::operator=(Image&&) noexcept = default;

int32_t Image::width() const { return _impl->width; }

int32_t Image::height() const { return _impl->height; }

uint16_t Image::max_val() const { return _impl->max_val; }

std::size_t Image::pixel_count() const { return _impl->pixels.size(); }

std::span<Pixel> Image::pixels() { return _impl->pixels; }

std::span<const Pixel> Image::pixels() const { return _impl->pixels; }

PpmResult Image::read(std::istream& is) {
    Image img{};
    int line_num = 1;
    enum class Phase { kHeader, kData } phase = Phase::kHeader;

    auto err = [&](PpmReadError e, std::string msg) -> PpmResult {
        return PpmResult{std::unexpected(e), std::move(msg)};
    };

    auto skip_ws = [&]() -> bool {
        while (true) {
            is >> std::ws;
            if (is.eof())
                return false;
            if (phase == Phase::kHeader && is.peek() == '#') {
                is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                ++line_num;
                continue;
            }
            break;
        }
        return true;
    };

    // ---- 1. Magic ----
    is >> std::ws;
    if (is.eof())
        return err(PpmReadError::kEmptyInput, "нет входных данных");

    std::string magic;
    is >> magic;
    if (is.fail())
        return err(PpmReadError::kIOError,
                   std::format("сбой чтения: {}", std::generic_category().message(errno)));

    if (magic != "P3")
        return err(PpmReadError::kBadMagic,
                   std::format("строка {}: ожидалось 'P3', получено: '{}'", line_num, magic));

    // ---- 2. Width, Height, Maxval ----
    auto read_int = [&](long long& out) -> bool {
        if (!skip_ws())
            return false;
        is >> out;
        return !is.fail();
    };

    {
        long long w, h, m;
        if (!read_int(w))
            return err(PpmReadError::kBadNumber,
                       std::format("строка {}: не удалось прочитать ширину", line_num));
        if (w <= 0)
            return err(
                PpmReadError::kBadNumber,
                std::format("строка {}: ширина должна быть положительным числом; получено: {}",
                            line_num, w));
        if (!read_int(h))
            return err(PpmReadError::kBadNumber,
                       std::format("строка {}: не удалось прочитать высоту", line_num));
        if (h <= 0)
            return err(
                PpmReadError::kBadNumber,
                std::format("строка {}: высота должна быть положительным числом; получено: {}",
                            line_num, h));
        if (!read_int(m))
            return err(PpmReadError::kBadNumber,
                       std::format("строка {}: не удалось прочитать максимальное значение канала",
                                   line_num));
        if (m != 255)
            return err(PpmReadError::kBadNumber,
                       std::format("строка {}: максимальное значение канала должно быть 255; "
                                   "получено: {}",
                                   line_num, m));

        img._impl->width = static_cast<int32_t>(w);
        img._impl->height = static_cast<int32_t>(h);
        img._impl->max_val = static_cast<uint16_t>(m);
        img._impl->pixels.reserve(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    }

    // ---- 3. Pixel data ----
    phase = Phase::kData;
    long long total_pixels = static_cast<long long>(img._impl->width) * img._impl->height;

    for (long long i = 0; i < total_pixels; ++i) {
        skip_ws();
        if (is.eof())
            return err(PpmReadError::kTooFewPixels,
                       std::format("строка {}: получено только {} пикселей (ожидалось {})",
                                   line_num, img._impl->pixels.size(), total_pixels));

        if (is.peek() == '#')
            return err(PpmReadError::kBadNumber,
                       std::format("строка {}: символ '#' не допускается в данных", line_num));

        int r, g, b;
        is >> r >> g >> b;
        if (is.fail())
            return err(PpmReadError::kBadNumber,
                       std::format("строка {}: нечисловое значение", line_num));

        if (r < 0 || r > img._impl->max_val || g < 0 || g > img._impl->max_val || b < 0 ||
            b > img._impl->max_val)
            return err(PpmReadError::kChannelRange,
                       std::format("строка {}: значение канала должно быть в [0; {}]; "
                                   "получено: {} {} {}",
                                   line_num, img._impl->max_val, r, g, b));

        img._impl->pixels.push_back(
            Pixel{static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)});
    }

    // ---- 4. Check for trailing data ----
    is >> std::ws;
    if (!is.eof()) {
        if (is.peek() == '#')
            return err(PpmReadError::kBadNumber,
                       std::format("строка {}: символ '#' не допускается в данных", line_num));

        char extra;
        is >> extra;
        if (!is.eof())
            return err(PpmReadError::kTooManyPixels,
                       std::format("строка {}: лишние данные после {} пикселей", line_num,
                                   img._impl->pixels.size()));
    }

    if (is.bad())
        return err(PpmReadError::kIOError,
                   std::format("сбой чтения: {}", std::generic_category().message(errno)));

    return PpmResult{std::move(img), {}};
}

PpmWriter::PpmWriter(std::ostream& os, int32_t width, int32_t height) : _os(os), _width(width) {
    std::println(_os, "P3");
    std::println(_os, "{} {}", width, height);
    std::println(_os, "255");
}

void PpmWriter::put(uint8_t r, uint8_t g, uint8_t b) {
    if (_col == 0) {
        std::print(_os, "{:3d} {:3d} {:3d}", static_cast<int>(r), static_cast<int>(g),
                   static_cast<int>(b));
    } else {
        std::print(_os, " {:3d} {:3d} {:3d}", static_cast<int>(r), static_cast<int>(g),
                   static_cast<int>(b));
    }
    ++_col;
    if (_col >= _width) {
        std::println(_os);
        _col = 0;
    }
}