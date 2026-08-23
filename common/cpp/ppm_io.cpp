#include "ppm_io.hpp"

#include <cerrno>
#include <iostream>
#include <limits>
#include <memory>
#include <print>
#include <span>
#include <system_error>
#include <vector>

namespace raster::common {

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

    auto err = [&](PpmReadError e, int32_t line, std::string msg) -> PpmResult {
        return PpmResult{std::unexpected(e), line, std::move(msg)};
    };

    auto skip_ws = [&]() -> bool {
        while (true) {
            int c = is.peek();
            if (c == '\n') {
                is.get();
                ++line_num;
                continue;
            }
            if (c == ' ' || c == '\t' || c == '\r') {
                is.get();
                continue;
            }
            if (phase == Phase::kHeader && c == '#') {
                is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                ++line_num;
                continue;
            }
            break;
        }
        return !is.eof();
    };

    // ---- 1. Magic ----
    if (!skip_ws())
        return err(PpmReadError::kEmptyInput, line_num, "нет входных данных");

    std::string magic;
    is >> magic;
    if (is.fail())
        return err(PpmReadError::kIOError, line_num,
                   std::format("сбой чтения: {}", std::generic_category().message(errno)));

    if (magic != "P3")
        return err(PpmReadError::kBadMagic, line_num,
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
            return err(PpmReadError::kBadNumber, line_num,
                       std::format("строка {}: не удалось прочитать ширину", line_num));
        if (w <= 0)
            return err(
                PpmReadError::kBadNumber, line_num,
                std::format("строка {}: ширина должна быть положительным числом; получено: {}",
                            line_num, w));
        if (!read_int(h))
            return err(PpmReadError::kBadNumber, line_num,
                       std::format("строка {}: не удалось прочитать высоту", line_num));
        if (h <= 0)
            return err(
                PpmReadError::kBadNumber, line_num,
                std::format("строка {}: высота должна быть положительным числом; получено: {}",
                            line_num, h));
        if (!read_int(m))
            return err(PpmReadError::kBadNumber, line_num,
                       std::format("строка {}: не удалось прочитать максимальное значение канала",
                                   line_num));
        if (m != kMaxChannel)
            return err(PpmReadError::kBadNumber, line_num,
                       std::format("строка {}: максимальное значение канала должно быть {}; "
                                   "получено: {}",
                                   line_num, kMaxChannel, m));

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
            return err(PpmReadError::kTooFewPixels, line_num,
                       std::format("строка {}: получено только {} пикселей (ожидалось {})",
                                   line_num, img._impl->pixels.size(), total_pixels));

        if (is.peek() == '#')
            return err(PpmReadError::kBadNumber, line_num,
                       std::format("строка {}: символ '#' не допускается в данных", line_num));

        int r, g, b;
        is >> r;
        if (is.fail())
            return err(PpmReadError::kBadNumber, line_num,
                       std::format("строка {}: нечисловое значение", line_num));
        skip_ws();
        is >> g;
        if (is.fail())
            return err(PpmReadError::kBadNumber, line_num,
                       std::format("строка {}: нечисловое значение", line_num));
        skip_ws();
        is >> b;
        if (is.fail())
            return err(PpmReadError::kBadNumber, line_num,
                       std::format("строка {}: нечисловое значение", line_num));
        if (r < 0 || r > img._impl->max_val || g < 0 || g > img._impl->max_val || b < 0 ||
            b > img._impl->max_val)
            return err(PpmReadError::kChannelRange, line_num,
                       std::format("строка {}: значение канала должно быть в [0; {}]; "
                                   "получено: {} {} {}",
                                   line_num, img._impl->max_val, r, g, b));

        img._impl->pixels.push_back(
            Pixel{static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)});
    }

    // ---- 4. Check for trailing data ----
    skip_ws();
    if (!is.eof()) {
        if (is.peek() == '#')
            return err(PpmReadError::kBadNumber, line_num,
                       std::format("строка {}: символ '#' не допускается в данных", line_num));

        char extra;
        is >> extra;
        if (!is.eof())
            return err(PpmReadError::kTooManyPixels, line_num,
                       std::format("строка {}: лишние данные после {} пикселей", line_num,
                                   img._impl->pixels.size()));
    }

    if (is.bad())
        return err(PpmReadError::kIOError, line_num,
                   std::format("сбой чтения: {}", std::generic_category().message(errno)));

    return PpmResult{std::move(img), 0, {}};
}

PpmWriter::PpmWriter(std::ostream& os, int32_t width, int32_t height, uint16_t max_val)
    : _os(os), _width(width), _height(height), _max_val(max_val),
      _capacity(static_cast<std::int64_t>(width) * height) {}

PpmWriteResult PpmWriter::putHeader() {
    std::println(_os, "P3");
    std::println(_os, "{} {}", _width, _height);
    std::println(_os, "{}", _max_val);
    if (_os.bad() || _os.fail())
        return PpmWriteResult{std::unexpected(PpmWriteError::kIOError),
                              "сбой записи заголовка в поток"};
    _header_written = true;
    return PpmWriteResult{};
}

PpmWriteResult PpmWriter::putAll(std::span<const Pixel> pixels, bool finalize) {
    for (const auto& p : pixels) {
        auto r = put(p.r, p.g, p.b);
        if (!r.value)
            return r;
    }
    if (finalize && _total < _capacity)
        return PpmWriteResult{
            std::unexpected(PpmWriteError::kNotEnoughPixels),
            std::format("записано {} пикселей при размере {}x{}", _total, _width, _height)};
    return PpmWriteResult{};
}

PpmWriteResult PpmWriter::put(uint8_t r, uint8_t g, uint8_t b) {
    if (!_header_written)
        return PpmWriteResult{std::unexpected(PpmWriteError::kIOError), "вызван put до putHeader"};

    if (_total >= _capacity)
        return PpmWriteResult{std::unexpected(PpmWriteError::kTooManyPixels),
                              std::format("попытка записать {} пикселей при размере {}x{}",
                                          _total + 1, _width, _height)};

    if (_col == 0) {
        std::print(_os, "{:3d} {:3d} {:3d}", static_cast<int>(r), static_cast<int>(g),
                   static_cast<int>(b));
    } else {
        std::print(_os, " {:3d} {:3d} {:3d}", static_cast<int>(r), static_cast<int>(g),
                   static_cast<int>(b));
    }
    ++_col;
    ++_total;
    if (_col >= _width) {
        std::println(_os);
        _col = 0;
    }
    if (_os.bad() || _os.fail())
        return PpmWriteResult{std::unexpected(PpmWriteError::kIOError),
                              "сбой записи пикселя в поток"};

    return PpmWriteResult{};
}

} // namespace raster::common