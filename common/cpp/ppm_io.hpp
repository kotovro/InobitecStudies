#ifndef KV_PPM_IO_HPP
#define KV_PPM_IO_HPP

#include <cstdint>
#include <expected>
#include <iosfwd>
#include <memory>
#include <span>
#include <string>

#if defined(_WIN32)
#if defined(KV_DYNAMIC_LINK)
#define KV_API __declspec(dllexport)
#else
#define KV_API
#endif
#else
#define KV_API
#endif

namespace raster::common {

inline constexpr uint16_t kMaxChannel = 255;

struct Pixel {
    uint8_t r{}, g{}, b{};
};

enum class PpmReadError {
    kEmptyInput,
    kBadMagic,
    kBadNumber,
    kChannelRange,
    kTooFewPixels,
    kTooManyPixels,
    kIOError,
    kAllocError,
};

struct PpmResult;

class Image {
  public:
    KV_API static PpmResult read(std::istream& is);

    KV_API Image();
    KV_API ~Image();
    KV_API Image(Image&&) noexcept;
    KV_API Image& operator=(Image&&) noexcept;
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    KV_API int32_t width() const;
    KV_API int32_t height() const;
    KV_API uint16_t max_val() const;
    KV_API std::size_t pixel_count() const;

    KV_API std::span<Pixel> pixels();
    KV_API std::span<const Pixel> pixels() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

struct PpmResult {
    std::expected<Image, PpmReadError> value;
    int32_t line = 0;
    std::string diagnostic;
};

enum class PpmWriteError {
    kIOError,
    kNotEnoughPixels,
    kTooManyPixels,
};

struct PpmWriteResult {
    std::expected<void, PpmWriteError> value;
    std::string diagnostic;
};

class KV_API PpmWriter {
  public:
    PpmWriter(std::ostream& os, int32_t width, int32_t height, uint16_t max_val = kMaxChannel);
    PpmWriteResult putHeader();
    PpmWriteResult putAll(std::span<const Pixel> pixels, bool finalize = false);
    PpmWriteResult flush();

  private:
    PpmWriteResult put(uint8_t r, uint8_t g, uint8_t b);
    std::ostream& _os;
    int32_t _width;
    int32_t _height;
    uint16_t _max_val;
    std::int64_t _capacity = 0;
    std::int64_t _total = 0;
    bool _header_written = false;
    int32_t _col = 0;
};

} // namespace raster::common

#endif