#include "string_sort_tester.hpp"

#include "algorithms/sort_algorithms.hpp"
#include "char_compare.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace {

using SortFunction = void (*)(std::vector<std::string>&);

const SortFunction kAlgorithms[] = {
    sort_algorithms::quickSort,
    sort_algorithms::mergeSort,
    sort_algorithms::stringQuickSort,
    sort_algorithms::stringMergeSort,
    sort_algorithms::msdRadixSort,
    sort_algorithms::msdRadixSortCutoff,
};

constexpr DataType kDataTypes[] = {
    DataType::Random,
    DataType::ReverseSorted,
    DataType::AlmostSorted,
    DataType::PrefixShared,
};

bool isSorted(const std::vector<std::string>& a) {
    for (std::size_t i = 1; i < a.size(); ++i) {
        if (a[i - 1] > a[i]) {
            return false;
        }
    }
    return true;
}

}  // namespace

StringSortTester::StringSortTester(BenchmarkConfig config) : config_(config) {}

Measurement StringSortTester::measure(
    const std::vector<std::string>& input,
    const std::function<void(std::vector<std::string>&)>& sortFn) const {
    std::vector<std::string> data = input;

    for (std::size_t i = 0; i < config_.warmupRuns; ++i) {
        sortFn(data);
        data = input;
    }

    double totalTimeMs = 0.0;
    std::size_t totalComparisons = 0;

    for (std::size_t run = 0; run < config_.measuredRuns; ++run) {
        data = input;
        CharCompare::reset();

        const auto start = std::chrono::steady_clock::now();
        sortFn(data);
        const auto end = std::chrono::steady_clock::now();

        if (!isSorted(data)) {
            throw std::runtime_error("Sort verification failed for algorithm output");
        }

        totalTimeMs += std::chrono::duration<double, std::milli>(end - start).count();
        totalComparisons += CharCompare::count();
    }

    return {totalTimeMs / static_cast<double>(config_.measuredRuns),
            totalComparisons / config_.measuredRuns};
}

std::vector<BenchmarkResult> StringSortTester::runAll(StringGenerator& generator) const {
    std::vector<BenchmarkResult> results;

    for (const DataType type : kDataTypes) {
        generator.generateMaster(type);
        const std::string typeName = dataTypeName(type);

        for (std::size_t size = StringGenerator::MIN_ARRAY_SIZE;
             size <= StringGenerator::MAX_ARRAY_SIZE;
             size += StringGenerator::ARRAY_SIZE_STEP) {
            const std::vector<std::string> input = generator.getSubarray(size);

            for (std::size_t alg = 0; alg < sizeof(kAlgorithms) / sizeof(kAlgorithms[0]); ++alg) {
                const auto sortFn = kAlgorithms[alg];
                const Measurement m = measure(input, sortFn);

                results.push_back(
                    {algorithmName(static_cast<int>(alg)), typeName, size, m.timeMs, static_cast<double>(m.comparisons)});
            }
        }
    }

    return results;
}

void StringSortTester::saveCsv(const std::vector<BenchmarkResult>& results, const std::string& path) const {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Failed to open output file: " + path);
    }

    out << "algorithm,data_type,array_size,avg_time_ms,avg_comparisons\n";
    out << std::fixed << std::setprecision(6);

    for (const auto& r : results) {
        out << r.algorithm << ','
            << r.dataType << ','
            << r.arraySize << ','
            << r.avgTimeMs << ','
            << r.avgComparisons << '\n';
    }
}

std::string StringSortTester::dataTypeName(DataType type) {
    switch (type) {
        case DataType::Random:
            return "random";
        case DataType::ReverseSorted:
            return "reverse_sorted";
        case DataType::AlmostSorted:
            return "almost_sorted";
        case DataType::PrefixShared:
            return "prefix_shared";
    }
    return "unknown";
}

std::string StringSortTester::algorithmName(int index) {
    static const char* names[] = {
        "quicksort",
        "mergesort",
        "string_quicksort",
        "string_mergesort",
        "msd_radix",
        "msd_radix_cutoff",
    };
    return names[index];
}
