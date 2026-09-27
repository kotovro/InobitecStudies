#include "ppm_io.hpp"

#include <cerrno>
#include <charconv>
#include <iostream>
#include <limits>
#include <memory>
#include <memory_resource>
#include <new>
#include <print>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace raster::common {

namespace {

std::string system_error_text(int e) {
    return std::format("{} (errno {})", std::error_code(e, std::generic_category()).message(), e);
}

} // namespace

struct Image::Impl {
    explicit Impl(std::pmr::memory_resource* resource) : pixels(resource) {}

    int32_t width{};
    int32_t height{};
    uint16_t max_val{};
    std::pmr::vector<Pixel> pixels;
};

Image::Image() : _impl(std::make_unique<Impl>(std::pmr::get_default_resource())) {}

Image::Image(std::pmr::memory_resource* mr) : _impl(std::make_unique<Impl>(mr)) {}

Image::~Image() = default;

Image::Image(Image&&) noexcept = default;

Image& Image::operator=(Image&&) noexcept = default;

int32_t Image::width() const { return _impl->width; }

int32_t Image::height() const { return _impl->height; }

uint16_t Image::max_val() const { return _impl->max_val; }

std::size_t Image::pixel_count() const { return _impl->pixels.size(); }

std::span<Pixel> Image::pixels() { return _impl->pixels; }

std::span<const Pixel> Image::pixels() const { return _impl->pixels; }

PpmResult Image::read(std::istream& is) { return read(is, std::pmr::get_default_resource()); }

PpmResult Image::read(std::istream& is, std::pmr::memory_resource* mr) {
    int line_num = 1;
    long long pixels_stored = 0;
    std::string diag;

    try {
        Image img{mr};
        enum class Phase { kHeader, kData } phase = Phase::kHeader;

        auto err = [&](PpmReadError e, int32_t line, std::string msg) -> PpmResult {
            return PpmResult{std::unexpected(e), line, std::move(msg)};
        };

        auto io_error = [&]() {
            int e = errno;
            return err(PpmReadError::kIOError, line_num,
                       std::format("сбой чтения: {}", system_error_text(e)));
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
            return io_error();

        if (magic != "P3")
            return err(PpmReadError::kBadMagic, line_num,
                       std::format("строка {}: ожидалось 'P3', получено: '{}'", line_num, magic));

        // ---- 2. Width, Height, Maxval ----
        enum class IntToken { kOk, kEof, kIoError, kError };

        auto read_int = [&](long long& out, std::string& diag) -> IntToken {
            if (!skip_ws())
                return is.bad() ? IntToken::kIoError : IntToken::kEof;
            if (is.peek() == '#') {
                diag = std::format("строка {}: символ '#' не допускается в данных", line_num);
                return IntToken::kError;
            }

            std::string token;
            for (;;) {
                int c = is.peek();
                if (c == EOF || c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '#')
                    break;
                token.push_back(static_cast<char>(is.get()));
            }

            if (is.bad())
                return IntToken::kIoError;
            if (token.empty())
                return IntToken::kEof;

            if (token.find('.') != std::string::npos) {
                diag = std::format("строка {}: значение должно быть целым числом; получено: {}",
                                   line_num, token);
                return IntToken::kError;
            }

            long long v{};
            auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), v);
            if (ec == std::errc::result_out_of_range || (ec == std::errc{} && v > 0x7FFFFFFF)) {
                diag = std::format("строка {}: число превышает допустимый диапазон", line_num);
                return IntToken::kError;
            }
            if (ec != std::errc{} || ptr != token.data() + token.size()) {
                diag = std::format("строка {}: нечисловое значение, получено: {}", line_num, token);
                return IntToken::kError;
            }

            out = v;
            return IntToken::kOk;
        };

        {
            long long w, h, m;

            auto tw = read_int(w, diag);
            if (tw == IntToken::kIoError)
                return io_error();
            if (tw == IntToken::kEof)
                return err(PpmReadError::kBadNumber, line_num,
                           std::format("строка {}: неожиданный конец файла", line_num));
            if (tw == IntToken::kError)
                return err(PpmReadError::kBadNumber, line_num, std::move(diag));
            if (w <= 0)
                return err(
                    PpmReadError::kBadNumber, line_num,
                    std::format("строка {}: ширина должна быть положительным числом; получено: {}",
                                line_num, w));

            auto th = read_int(h, diag);
            if (th == IntToken::kIoError)
                return io_error();
            if (th == IntToken::kEof)
                return err(PpmReadError::kBadNumber, line_num,
                           std::format("строка {}: неожиданный конец файла", line_num));
            if (th == IntToken::kError)
                return err(PpmReadError::kBadNumber, line_num, std::move(diag));
            if (h <= 0)
                return err(
                    PpmReadError::kBadNumber, line_num,
                    std::format("строка {}: высота должна быть положительным числом; получено: {}",
                                line_num, h));

            auto tm = read_int(m, diag);
            if (tm == IntToken::kIoError)
                return io_error();
            if (tm == IntToken::kEof)
                return err(PpmReadError::kBadNumber, line_num,
                           std::format("строка {}: неожиданный конец файла", line_num));
            if (tm == IntToken::kError)
                return err(PpmReadError::kBadNumber, line_num, std::move(diag));
            if (m != kMaxChannel)
                return err(PpmReadError::kBadNumber, line_num,
                           std::format("строка {}: максимальное значение канала должно быть {}; "
                                       "получено: {}",
                                       line_num, kMaxChannel, m));

            img._impl->width = static_cast<int32_t>(w);
            img._impl->height = static_cast<int32_t>(h);
            img._impl->max_val = static_cast<uint16_t>(m);
        }

        // ---- 3. Pixel data ----
        phase = Phase::kData;
        long long total_pixels = static_cast<long long>(img._impl->width) * img._impl->height;

        auto too_few = [&]() {
            return err(PpmReadError::kTooFewPixels, line_num,
                       std::format("строка {}: получено только {} пикселей (ожидалось {})",
                                   line_num, img._impl->pixels.size(), total_pixels));
        };

        for (long long i = 0; i < total_pixels; ++i) {
            pixels_stored = i;

            skip_ws();
            if (is.eof())
                return too_few();

            if (is.peek() == '#')
                return err(PpmReadError::kBadNumber, line_num,
                           std::format("строка {}: символ '#' не допускается в данных", line_num));

            long long r, g, b;
            auto tr = read_int(r, diag);
            if (tr == IntToken::kIoError)
                return io_error();
            if (tr == IntToken::kEof)
                return too_few();
            if (tr == IntToken::kError)
                return err(PpmReadError::kBadNumber, line_num, std::move(diag));

            auto tg = read_int(g, diag);
            if (tg == IntToken::kIoError)
                return io_error();
            if (tg == IntToken::kEof)
                return too_few();
            if (tg == IntToken::kError)
                return err(PpmReadError::kBadNumber, line_num, std::move(diag));

            auto tb = read_int(b, diag);
            if (tb == IntToken::kIoError)
                return io_error();
            if (tb == IntToken::kEof)
                return too_few();
            if (tb == IntToken::kError)
                return err(PpmReadError::kBadNumber, line_num, std::move(diag));

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
            return io_error();

        return PpmResult{std::move(img), 0, {}};
    } catch (const std::bad_alloc&) {
        std::format_to(std::back_inserter(diag),
                       "строка {}: не удалось выделить память; размещено пикселей: {}", line_num,
                       pixels_stored);
        return PpmResult{std::unexpected(PpmReadError::kAllocError), line_num, std::move(diag)};
    } catch (const std::length_error&) {
        std::format_to(std::back_inserter(diag),
                       "строка {}: требуемый объём превышает предел контейнера", line_num);
        return PpmResult{std::unexpected(PpmReadError::kAllocError), line_num, std::move(diag)};
    } catch (const std::ios_base::failure&) {
        int e = errno;
        std::format_to(std::back_inserter(diag), "сбой чтения: {}", system_error_text(e));
        return PpmResult{std::unexpected(PpmReadError::kIOError), line_num, std::move(diag)};
    }
}

PpmWriter::PpmWriter(std::ostream& os, int32_t width, int32_t height, uint16_t max_val)
    : _os(os), _width(width), _height(height), _max_val(max_val),
      _capacity(static_cast<std::int64_t>(width) * height) {}

PpmWriteResult PpmWriter::putHeader() {
    errno = 0;
    std::println(_os, "P3");
    std::println(_os, "{} {}", _width, _height);
    std::println(_os, "{}", _max_val);
    if (_os.bad() || _os.fail()) {
        int e = errno;
        return PpmWriteResult{
            std::unexpected(PpmWriteError::kIOError),
            std::format("сбой записи заголовка в поток: {}", system_error_text(e))};
    }
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

PpmWriteResult PpmWriter::flush() {
    errno = 0;
    _os.flush();
    if (_os.bad() || _os.fail()) {
        int e = errno;
        return PpmWriteResult{
            std::unexpected(PpmWriteError::kIOError),
            std::format("сбой записи при сбросе потока: {}", system_error_text(e))};
    }
    return PpmWriteResult{};
}

PpmWriteResult PpmWriter::put(uint8_t r, uint8_t g, uint8_t b) {
    if (!_header_written)
        return PpmWriteResult{std::unexpected(PpmWriteError::kIOError), "вызван put до putHeader"};

    if (_total >= _capacity)
        return PpmWriteResult{std::unexpected(PpmWriteError::kTooManyPixels),
                              std::format("попытка записать {} пикселей при размере {}x{}",
                                          _total + 1, _width, _height)};

    errno = 0;
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
    if (_os.bad() || _os.fail()) {
        int e = errno;
        return PpmWriteResult{std::unexpected(PpmWriteError::kIOError),
                              std::format("сбой записи пикселя в поток: {}", system_error_text(e))};
    }

    return PpmWriteResult{};
}

} // namespace raster::common