#include "ppm_stats.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <ostream>
#include <print>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>

using namespace raster::common;
using namespace raster::stats;

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

void check_stats(const Stats& s, int32_t w, int32_t h, int64_t px, int64_t tr, int64_t tg,
                 int64_t tb) {
    check(s.width == w, "width");
    check(s.height == h, "height");
    check(s.pixel_count == px, "pixel_count");
    check(s.total_r == tr, "total_r");
    check(s.total_g == tg, "total_g");
    check(s.total_b == tb, "total_b");
}

std::optional<Stats> read_stats_from(std::string_view ppm) {
    auto ss = std::istringstream(std::string(ppm));
    auto r = Image::read(ss);
    if (!r.value.has_value())
        return std::nullopt;
    return compute_stats(*r.value);
}

class ControllableBuf : public std::streambuf {
  public:
    void fail_with(int e) {
        fail_ = true;
        err_ = e;
    }

  protected:
    int_type overflow(int_type c) override {
        if (fail_) {
            errno = err_;
            return traits_type::eof();
        }
        if (c != traits_type::eof())
            data_ += static_cast<char>(c);
        return traits_type::not_eof(c);
    }
    int sync() override {
        if (fail_) {
            errno = err_;
            return -1;
        }
        return 0;
    }

  private:
    bool fail_ = false;
    int err_ = 0;
    std::string data_;
};

class FlushFailBuf : public std::streambuf {
  protected:
    int_type overflow(int_type c) override {
        if (c != traits_type::eof())
            data_ += static_cast<char>(c);
        return traits_type::not_eof(c);
    }
    int sync() override {
        errno = EPIPE;
        return -1;
    }

  private:
    std::string data_;
};

class SilentFailBuf : public std::streambuf {
  protected:
    int_type overflow(int_type) override { return traits_type::eof(); }
    int sync() override { return -1; }
};

void check_io_diagnostic(const StatsWriteResult& r, std::string_view phase, int err,
                         std::string_view name) {
    check(!r.value.has_value() && r.value.error() == StatsWriteError::kIOError,
          std::string(name) + ": error");
    if (r.value.has_value())
        return;
    check(r.diagnostic.find(phase) != std::string::npos, std::string(name) + ": phase");
    check(r.diagnostic.find(std::string{"(errno "} + std::to_string(err) + ")") !=
              std::string::npos,
          std::string(name) + ": errno");
    check(r.diagnostic.find(std::generic_category().message(err)) != std::string::npos,
          std::string(name) + ": system text");
}

void check_fallback_diagnostic(const StatsWriteResult& r, std::string_view name) {
    check(!r.value.has_value() && r.value.error() == StatsWriteError::kIOError,
          std::string(name) + ": error");
    if (r.value.has_value())
        return;
    check(r.diagnostic.find("сбой записи отчёта") != std::string::npos,
          std::string(name) + ": phase");
    check(r.diagnostic.find("неизвестная ошибка ввода-вывода") != std::string::npos,
          std::string(name) + ": fallback");
}

// Statistics accuracy

void test_stats_known() {
    auto s = read_stats_from("P3\n2 2\n255\n"
                             "0 0 0 255 0 0\n"
                             "0 255 0 0 0 255\n");
    check(s.has_value(), "known image -> ok");
    if (!s.has_value())
        return;

    check_stats(*s, 2, 2, 4, 255, 255, 255);
    check(s->y_min >= 0.0 && s->y_min <= 0.1, "y_min ~ 0.0");
    check(s->y_max >= 149.5 && s->y_max <= 149.9, "y_max ~ 149.7");
    check(s->histogram[0] == 2, "hist[0] == 2");
    check(s->histogram[2] == 1, "hist[2] == 1");
    check(s->histogram[4] == 1, "hist[4] == 1");
}

void test_stats_comments() {
    auto s = read_stats_from("P3\n# width\n2 2\n# before maxval\n255\n"
                             "0 0 0\n0 0 0\n0 0 0\n0 0 0\n");
    check(s.has_value(), "comments in header -> ok");
    if (s.has_value())
        check_stats(*s, 2, 2, 4, 0, 0, 0);
}

void test_print_stats_success() {
    auto s = read_stats_from("P3\n2 2\n255\n"
                             "0 0 0 255 0 0\n"
                             "0 255 0 0 0 255\n");
    check(s.has_value(), "print stats: input ok");
    if (!s.has_value())
        return;

    std::ostringstream os;
    auto r = ppm_print_stats(*s, os);
    check(r.value.has_value(), "print stats: success");
    check(os.str().starts_with("2x2"), "print stats: dimensions written");
}

void test_print_stats_write_error() {
    auto s = read_stats_from("P3\n2 2\n255\n"
                             "0 0 0 255 0 0\n"
                             "0 255 0 0 0 255\n");
    check(s.has_value(), "print stats write error: input ok");
    if (!s.has_value())
        return;

    ControllableBuf sbuf;
    sbuf.fail_with(EBADF);
    std::ostream os(&sbuf);
    auto r = ppm_print_stats(*s, os);
    check_io_diagnostic(r, "сбой записи отчёта", EBADF, "print stats write error");
}

void test_print_stats_flush_error() {
    auto s = read_stats_from("P3\n2 2\n255\n"
                             "0 0 0 255 0 0\n"
                             "0 255 0 0 0 255\n");
    check(s.has_value(), "print stats flush error: input ok");
    if (!s.has_value())
        return;

    FlushFailBuf sbuf;
    std::ostream os(&sbuf);
    auto r = ppm_print_stats(*s, os);
    check_io_diagnostic(r, "сбой записи отчёта", EPIPE, "print stats flush error");
}

void test_print_stats_without_errno() {
    auto s = read_stats_from("P3\n2 2\n255\n"
                             "0 0 0 255 0 0\n"
                             "0 255 0 0 0 255\n");
    check(s.has_value(), "print stats without errno: input ok");
    if (!s.has_value())
        return;

    SilentFailBuf sbuf;
    std::ostream os(&sbuf);
    auto r = ppm_print_stats(*s, os);
    check_fallback_diagnostic(r, "print stats without errno");
}

void test_run_stats_success() {
    std::istringstream in("P3\n1 1\n255\n0 0 0\n");
    std::ostringstream out;
    check(run_stats(in, out) == 0, "run stats: success");
}

void test_run_stats_write_error() {
    ControllableBuf sbuf;
    sbuf.fail_with(EBADF);
    std::ostream out(&sbuf);
    std::istringstream in("P3\n1 1\n255\n0 0 0\n");
    check(run_stats(in, out) == 74, "run stats: write error -> 74");
}

// ---- main ----

} // namespace

int main() {
    std::println("--- compute_stats tests ---");
    test_stats_known();
    test_stats_comments();

    std::println("--- write error tests ---");
    test_print_stats_success();
    test_print_stats_write_error();
    test_print_stats_flush_error();
    test_print_stats_without_errno();
    test_run_stats_success();
    test_run_stats_write_error();

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}