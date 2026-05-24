#include "string_generator.hpp"

#include "algorithms/alphabet.hpp"
#include "char_compare.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace {

int lexicographicLess(const std::string& a, const std::string& b) {
    return CharCompare::compare(a, b) < 0;
}

}  // namespace

StringGenerator::StringGenerator(unsigned seed) : rng_(seed) {}

void StringGenerator::generateMaster(DataType type) {
    master_.clear();

    switch (type) {
        case DataType::Random:
            generateRandomMaster();
            break;
        case DataType::PrefixShared:
            generatePrefixSharedMaster();
            break;
        case DataType::ReverseSorted:
        case DataType::AlmostSorted:
            generateRandomMaster();
            std::sort(master_.begin(), master_.end(), lexicographicLess);
            if (type == DataType::ReverseSorted) {
                std::reverse(master_.begin(), master_.end());
            } else {
                std::uniform_int_distribution<std::size_t> indexDist(0, master_.size() - 1);
                const std::size_t swaps = std::max<std::size_t>(1, master_.size() / 50);
                for (std::size_t i = 0; i < swaps; ++i) {
                    const std::size_t a = indexDist(rng_);
                    const std::size_t b = indexDist(rng_);
                    if (a != b) {
                        std::swap(master_[a], master_[b]);
                    }
                }
            }
            break;
    }
}

std::vector<std::string> StringGenerator::getSubarray(std::size_t size) const {
    if (size > master_.size()) {
        throw std::out_of_range("Requested subarray size exceeds master array size");
    }
    return std::vector<std::string>(master_.begin(), master_.begin() + static_cast<std::ptrdiff_t>(size));
}

std::string StringGenerator::randomString(std::size_t len) {
    std::uniform_int_distribution<std::size_t> symbolDist(0, alphabet::SIZE - 1);
    std::string s;
    s.reserve(len);
    for (std::size_t i = 0; i < len; ++i) {
        s.push_back(alphabet::SYMBOLS[symbolDist(rng_)]);
    }
    return s;
}

void StringGenerator::generateRandomMaster() {
    master_.reserve(MAX_ARRAY_SIZE);
    std::uniform_int_distribution<std::size_t> lenDist(MIN_STRING_LEN, MAX_STRING_LEN);
    for (std::size_t i = 0; i < MAX_ARRAY_SIZE; ++i) {
        master_.push_back(randomString(lenDist(rng_)));
    }
}

void StringGenerator::generatePrefixSharedMaster() {
    master_.reserve(MAX_ARRAY_SIZE);

    std::uniform_int_distribution<std::size_t> lenDist(MIN_STRING_LEN, MAX_STRING_LEN);
    std::uniform_int_distribution<std::size_t> prefixLenDist(5, 30);
    std::uniform_int_distribution<int> groupFlip(0, 1);

    std::size_t i = 0;
    while (i < MAX_ARRAY_SIZE) {
        const std::size_t groupSize = std::min<std::size_t>(
            MAX_ARRAY_SIZE - i,
            std::uniform_int_distribution<std::size_t>(20, 80)(rng_));

        const std::size_t prefixLen = prefixLenDist(rng_);
        const std::string prefix = randomString(prefixLen);

        for (std::size_t j = 0; j < groupSize; ++j) {
            const std::size_t totalLen = lenDist(rng_);
            const std::size_t suffixLen = totalLen > prefixLen ? totalLen - prefixLen : 0;
            master_.push_back(prefix + randomString(suffixLen));
            ++i;
        }

        // Иногда добавляем «шумовую» строку без общего префикса с группой.
        if (i < MAX_ARRAY_SIZE && groupFlip(rng_) == 1) {
            master_.push_back(randomString(lenDist(rng_)));
            ++i;
        }
    }
}
