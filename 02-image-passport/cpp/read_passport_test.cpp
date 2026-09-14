#include <cerrno>
#include <cstdlib>
#include <format>
#include <istream>
#include <print>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "pixel_word.hpp"
#include "read_passport.hpp"

using namespace raster::passport;

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

// Input buffer that fails immediately with `err` (errno) via ios_base::failure,
// which puts the stream into the bad state.
class FailingInputBuf : public std::streambuf {
  public:
    explicit FailingInputBuf(int err) : err_(err) {}

  protected:
    int_type underflow() override {
        errno = err_;
        throw std::ios_base::failure("simulated io error");
    }

  private:
    int err_;
};

void test_read_valid() {
    auto is = std::istringstream("морской закат\n1920\n");
    auto r = read_passport(is);
    check(r.has_value(), "valid input -> ok");
    if (r.has_value()) {
        check(r->name == "морской закат", "valid: name");
        check(r->count == 1920, "valid: count");
    }
}

void test_read_no_input() {
    auto is = std::istringstream("");
    auto r = read_passport(is);
    check(!r.has_value(), "no input -> error");
    if (!r.has_value())
        check(r.error().kind == PassportErrorKind::kNoInput, "no input -> kNoInput");
}

void test_read_empty_name() {
    auto is = std::istringstream("\n1920\n");
    auto r = read_passport(is);
    check(!r.has_value(), "empty name -> error");
    if (!r.has_value())
        check(r.error().kind == PassportErrorKind::kEmptyName, "empty name -> kEmptyName");
}

void test_read_bad_count() {
    auto is = std::istringstream("тест\nabc\n");
    auto r = read_passport(is);
    check(!r.has_value(), "bad count -> error");
    if (!r.has_value()) {
        check(r.error().kind == PassportErrorKind::kBadCount, "bad count -> kBadCount");
        check(r.error().bad_value == "abc", "bad count -> bad_value");
    }
}

void test_read_negative_count() {
    auto is = std::istringstream("тест\n-5\n");
    auto r = read_passport(is);
    check(!r.has_value(), "negative count -> error");
    if (!r.has_value())
        check(r.error().kind == PassportErrorKind::kNegativeCount, "negative -> kNegativeCount");
}

void test_read_io_error() {
    FailingInputBuf sbuf(EIO);
    std::istream is(&sbuf);
    auto r = read_passport(is);
    check(!r.has_value(), "io error -> error");
    if (!r.has_value()) {
        check(r.error().kind == PassportErrorKind::kIOError, "io error -> kIOError");
        check(r.error().system_error == std::error_code(EIO, std::generic_category()),
              "io error -> system_error");
    }
}

void test_error_messages() {
    check(passport_error_message(PassportError{PassportErrorKind::kNoInput, {}}) == "Нет ввода",
          "msg: no input");
    check(passport_error_message(PassportError{PassportErrorKind::kEmptyName, {}}) ==
              "Название изображения не может быть пустым",
          "msg: empty name");
    check(passport_error_message(PassportError{PassportErrorKind::kBadCount, "abc"}) ==
              "количество пикселей должно быть числом; получено: abc",
          "msg: bad count");
    check(passport_error_message(PassportError{PassportErrorKind::kNegativeCount, "-5"}) ==
              "количество пикселей должно быть положительным; получено: -5",
          "msg: negative count");
    auto ec = std::error_code(EIO, std::generic_category());
    check(passport_error_message(PassportError{PassportErrorKind::kIOError, {}, ec}) ==
              std::format("Сбой ввода: {} (errno {})", ec.message(), ec.value()),
          "msg: io error");
}

} // namespace

int main() {
    std::println("--- pixel_word tests ---");

    check(pixel_word(0) == "пикселей", "0 пикселей");
    check(pixel_word(1) == "пиксель", "1 пиксель");
    check(pixel_word(2) == "пикселя", "2 пикселя");
    check(pixel_word(3) == "пикселя", "3 пикселя");
    check(pixel_word(4) == "пикселя", "4 пикселя");
    check(pixel_word(5) == "пикселей", "5 пикселей");
    check(pixel_word(6) == "пикселей", "6 пикселей");
    check(pixel_word(10) == "пикселей", "10 пикселей");
    check(pixel_word(11) == "пикселей", "11 пикселей");
    check(pixel_word(12) == "пикселей", "12 пикселей");
    check(pixel_word(13) == "пикселей", "13 пикселей");
    check(pixel_word(14) == "пикселей", "14 пикселей");
    check(pixel_word(20) == "пикселей", "20 пикселей");
    check(pixel_word(21) == "пиксель", "21 пиксель");
    check(pixel_word(22) == "пикселя", "22 пикселя");
    check(pixel_word(23) == "пикселя", "23 пикселя");
    check(pixel_word(24) == "пикселя", "24 пикселя");
    check(pixel_word(25) == "пикселей", "25 пикселей");
    check(pixel_word(101) == "пиксель", "101 пиксель");
    check(pixel_word(102) == "пикселя", "102 пикселя");
    check(pixel_word(111) == "пикселей", "111 пикселей");
    check(pixel_word(114) == "пикселей", "114 пикселей");
    check(pixel_word(121) == "пиксель", "121 пиксель");
    check(pixel_word(122) == "пикселя", "122 пикселя");
    check(pixel_word(1000) == "пикселей", "1000 пикселей");
    check(pixel_word(1001) == "пиксель", "1001 пиксель");
    check(pixel_word(2002) == "пикселя", "2002 пикселя");

    std::println("--- read_passport tests ---");
    test_read_valid();
    test_read_no_input();
    test_read_empty_name();
    test_read_bad_count();
    test_read_negative_count();
    test_read_io_error();

    std::println("--- error messages ---");
    test_error_messages();

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}