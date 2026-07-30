#ifndef KV_PPM_IO_HPP
#define KV_PPM_IO_HPP

#include <cstdint>
#include <expected>
#include <iosfwd>
#include <string>
#include <vector>

struct Pixel {
    uint8_t r{}, g{}, b{};
};

struct Image {
    int32_t width{}, height{};
    uint16_t max_val{};
    std::vector<Pixel> pixels;
};

enum class PpmReadError {
    kEmptyInput,
    kBadMagic,
    kBadNumber,
    kChannelRange,
    kTooFewPixels,
    kTooManyPixels,
    kIOError,
};

struct PpmResult {
    std::expected<Image, PpmReadError> value;
    std::string diagnostic;
};

PpmResult ppm_read(std::istream& is);

class PpmWriter {
public:
    PpmWriter(std::ostream& os, int32_t width, int32_t height);
    void put(uint8_t r, uint8_t g, uint8_t b);

private:
    std::ostream& _os;
    int32_t _width;
    int32_t _col = 0;
};

#endif