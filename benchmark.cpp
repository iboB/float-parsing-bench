#define PICOBENCH_IMPLEMENT
#include <picobench/picobench.hpp>

#include <splat/inline.h>

#include <random>
#include <iostream>
#include <vector>
#include <string_view>

#include <msstl/charconv.hpp>

uint32_t get_seed() {
    // uint32_t seed = std::random_device{}();
    uint32_t seed = 42;
    std::cout << "random seed:" << seed << std::endl;
    return seed;
}

template <typename F>
size_t hash_vec(const std::vector<F>& vec) {
    std::string_view sv(reinterpret_cast<const char*>(vec.data()), vec.size() * sizeof(F));
    return std::hash<std::string_view>{}(sv);
}

template <typename F>
std::vector<F> get_random_values(std::mt19937& rng, size_t size) {
    std::uniform_real_distribution<F> dist(-100, 100);
    std::vector<F> ret(size);
    for (auto& v : ret) {
        v = dist(rng);
    }
    return ret;
}

template <typename F>
std::string to_string(const std::vector<F>& vec) {
    std::string ret = "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != 0) {
            ret += ", ";
        }

        char buf[128] = {};
        msstl::to_chars(buf, buf + sizeof(buf), vec[i]);
        ret += buf;
    }
    ret += "]";
    return ret;
}

bool is_numeric(char c) {
    return (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+';
}

template <auto from_chars, typename F>
void parse_json(std::string_view str, std::vector<F>& out) {
    const char* cur = str.data();
    const char* end = str.data() + str.size();
    while (true) {
        while (cur < end && !is_numeric(*cur)) {
            ++cur;
        }
        if (cur >= end) {
            break;
        }

        double d;
        auto res = from_chars(cur, end, d);
        if (res.ec != std::errc()) {
            throw std::runtime_error("from_chars failed");
        }
        out.push_back(F(d));
        cur = res.ptr;
    }
}

struct benchmark_input {
    std::string json;
    size_t expected_size;
    size_t expected_hash;

    template <typename F>
    explicit benchmark_input(const std::vector<F>& vec)
        : json(to_string(vec))
        , expected_size(vec.size())
        , expected_hash(hash_vec(vec))
    {}

    picobench::state::input to_input() const {
        return {int(expected_size), reinterpret_cast<uintptr_t>(this)};
    }

    static const benchmark_input* from_input_data(uintptr_t ud) {
        return reinterpret_cast<const benchmark_input*>(ud);
    }
};

template <typename F>
benchmark_input make_benchmark_input(std::mt19937& rng, size_t size) {
    auto vec = get_random_values<F>(rng, size);
    return benchmark_input{vec};
}

template <auto from_chars, typename F>
void bench_charconv(picobench::state& s) {
    auto input = benchmark_input::from_input_data(s.input_data());

    std::vector<F> result;
    result.reserve(s.iterations());

    {
        picobench::scope scope(s);
        parse_json<from_chars>(input->json, result);
    }

    size_t hash = hash_vec(result);
    if (hash != input->expected_hash) {
        throw std::runtime_error("hash mismatch");
    }
    s.set_result(hash);
}

#if HAVE_STD_CHARCONV
#include <charconv>
FORCE_INLINE auto std_from_chars(const char* first, const char* last, double& value) {
    return std::from_chars(first, last, value);
}
#endif

FORCE_INLINE auto msstl_from_chars(const char* first, const char* last, double& value) {
    return msstl::from_chars(first, last, value);
}

int main(int argc, char** argv) {
    std::mt19937 rng(get_seed());
    picobench::runner r;

    r.set_suite("float");

    auto input_float_a = make_benchmark_input<float>(rng, 10000);
    auto input_float_b = make_benchmark_input<float>(rng, 100000);

    std::vector<picobench::state::input> inputs_float = {
        input_float_a.to_input(),
        input_float_b.to_input()
    };

    r.add_benchmark("msstl", bench_charconv<msstl_from_chars, float>).inputs(inputs_float);
#if HAVE_STD_CHARCONV
    r.add_benchmark("std", bench_charconv<std_from_chars, float>).inputs(inputs_float);
#endif

    r.set_suite("double");

    auto input_double_a = make_benchmark_input<double>(rng, 10000);
    auto input_double_b = make_benchmark_input<double>(rng, 100000);

    std::vector<picobench::state::input> inputs_double = {
        input_double_a.to_input(),
        input_double_b.to_input()
    };

    r.add_benchmark("msstl", bench_charconv<msstl_from_chars, double>).inputs(inputs_double);
#if HAVE_STD_CHARCONV
    r.add_benchmark("std", bench_charconv<std_from_chars, double>).inputs(inputs_double);
#endif

    r.set_compare_results_across_samples(true);
    r.set_compare_results_across_benchmarks(true);
    r.parse_cmd_line(argc, argv);
    return r.run();
}
