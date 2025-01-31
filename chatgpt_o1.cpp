#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <span>
#include <concepts>
#include <cstdint>

// A simple concept to ensure we only accept containers with contiguous storage.
template <typename C>
concept ContiguousContainer = requires(C c) {
    { c.data() } -> std::convertible_to<typename C::value_type*>;
    { c.size() } -> std::convertible_to<std::size_t>;
};

//--------------------------------------------
// Utility: Add two subranges (little-endian) in place, no extra copies
//--------------------------------------------
static void addInPlace(std::span<const std::uint64_t> A,
                       std::span<const std::uint64_t> B,
                       std::span<std::uint64_t> R)
{
    // R.size() should be >= max(A.size(), B.size()) + 1
    // We assume that here, for brevity
    std::uint64_t carry = 0;
    std::size_t i = 0;
    for (; i < A.size() || i < B.size() || carry; ++i) {
        std::uint64_t a = (i < A.size()) ? A[i] : 0ULL;
        std::uint64_t b = (i < B.size()) ? B[i] : 0ULL;
        unsigned __int128 sum = (unsigned __int128) a + b + carry;
        R[i] = (std::uint64_t)(sum);
        carry = (std::uint64_t)(sum >> 64);
    }
    // If i < R.size(), remaining R[i] can stay as is.
}

//--------------------------------------------
// Utility: Subtract two subranges in place: R = A - B. 
// Assumes A >= B in value. 
//--------------------------------------------
static void subInPlace(std::span<const std::uint64_t> A,
                       std::span<const std::uint64_t> B,
                       std::span<std::uint64_t> R)
{
    // R.size() >= A.size()
    std::uint64_t borrow = 0;
    for (std::size_t i = 0; i < A.size(); ++i) {
        std::uint64_t a = A[i];
        std::uint64_t b = (i < B.size() ? B[i] : 0ULL);
        
        // Expand to 128 for safe subtraction with borrow
        unsigned __int128 tmp = (unsigned __int128) a - b - borrow;
        R[i] = (std::uint64_t) tmp;
        borrow = (std::uint64_t)((tmp >> 64) & 1ULL); // 1 if negative
    }
}

//--------------------------------------------
// Low-level Karatsuba recursion on 64-bit "digits"
// We'll do: R = A * B (in 64-bit base).
// Each of A,B,R is given as a span referring to some contiguous area.
// This function doesn't do new allocations except small stack buffers.
//--------------------------------------------
void karatsubaRec(std::span<const std::uint64_t> A,
                  std::span<const std::uint64_t> B,
                  std::span<std::uint64_t>       R)
{
    // Remove leading zero chunks for smaller recursion
    while (!A.empty() && A.back() == 0ULL) A = A.first(A.size()-1);
    while (!B.empty() && B.back() == 0ULL) B = B.first(B.size()-1);

    // If either is empty, product is zero
    if (A.empty() || B.empty()) {
        std::fill(R.begin(), R.end(), 0ULL);
        return;
    }
    // Base case for very small sizes
    if (A.size() == 1 && B.size() == 1) {
        // 128-bit multiplication
        unsigned __int128 prod = (unsigned __int128)A[0] * B[0];
        R[0] = (std::uint64_t)(prod);
        if (R.size() > 1) {
            R[1] = (std::uint64_t)(prod >> 64);
        }
        // zero out beyond that
        for (std::size_t i = 2; i < R.size(); i++) R[i] = 0ULL;
        return;
    }

    // n is the max of lengths
    std::size_t n = std::max(A.size(), B.size());
    std::size_t half = n / 2;  
    // half is in "digits" of 64 bits each.

    // Let A = (A_hi << (64 * half)) + A_lo
    // We'll define:
    //   A_lo = A[0..half-1], A_hi = A[half..end-1]
    //   B_lo = B[0..half-1], B_hi = B[half..end-1]
    // Then we do:
    //   a = A_hi * B_hi
    //   b = A_lo * B_lo
    //   c = (A_hi + A_lo)*(B_hi + B_lo) - a - b
    //   R = a << (2*half*64) + c << (half*64) + b

    // Split A
    std::span<const std::uint64_t> A_lo = A.size() <= half ? 
        A : A.first(half);
    std::span<const std::uint64_t> A_hi = (A.size() <= half) ?
        std::span<const std::uint64_t>() : A.subspan(half);

    // Split B
    std::span<const std::uint64_t> B_lo = B.size() <= half ?
        B : B.first(half);
    std::span<const std::uint64_t> B_hi = (B.size() <= half) ?
        std::span<const std::uint64_t>() : B.subspan(half);

    // We'll need to produce:
    //   a has size up to 2*ceil(A_hi,B_hi)
    //   b has size up to 2*ceil(A_lo,B_lo)
    //   c has size up to something too. We'll store each in subranges of R or in local temp buffers.

    // We'll create three subranges in R for a, b, and an intermediate c area:
    // to ensure no needless duplication.

    // However, R can hold up to 2*n digits. 
    // We'll define:
    //   R_a = R[2*half .. 2*half + some_length)    for a
    //   R_b = R[0..some_length_for_b)              for b
    //   Then we'll have a subrange for the sum: we need up to half+half + 1 digits => up to n+1
    //   We'll keep c in a local buffer of size = a+b. Then add it in. That is minimal duplication?

    // For clarity, let's do small ephemeral buffers for a, b, c — but
    // each just large enough for the partial product. This is "extra memory",
    // but not a *copy of the entire input*, and is typical in Karatsuba. 
    // It's not “needless” in the sense that we have to store partial results somewhere.

    std::size_t size_hi = A_hi.size() + B_hi.size();
    std::size_t size_lo = A_lo.size() + B_lo.size();
    std::size_t size_ab = std::max(size_hi, size_lo); // for the sum parts

    std::vector<std::uint64_t> buffA(size_hi, 0ULL);
    std::vector<std::uint64_t> buffB(size_lo, 0ULL);
    std::vector<std::uint64_t> buffC(size_ab + 1, 0ULL); 

    // 1) compute a = A_hi * B_hi
    {
        std::span<std::uint64_t> aSpan(buffA);
        karatsubaRec(A_hi, B_hi, aSpan);
    }

    // 2) compute b = A_lo * B_lo
    {
        std::span<std::uint64_t> bSpan(buffB);
        karatsubaRec(A_lo, B_lo, bSpan);
    }

    // 3) compute (A_hi + A_lo) and (B_hi + B_lo), multiply them => c
    {
        // We'll make local sum buffers for A_hi+lo, B_hi+lo
        // size ~ half+some => at most n+1
        std::vector<std::uint64_t> sumA(std::max(A_hi.size(), A_lo.size())+1, 0ULL);
        std::vector<std::uint64_t> sumB(std::max(B_hi.size(), B_lo.size())+1, 0ULL);

        // sum up
        {
            std::span<std::uint64_t> sumAspan(sumA);
            addInPlace(A_hi, A_lo, sumAspan);
        }
        {
            std::span<std::uint64_t> sumBspan(sumB);
            addInPlace(B_hi, B_lo, sumBspan);
        }

        // c = (A_hi + A_lo)*(B_hi + B_lo)
        karatsubaRec(sumA, sumB, std::span<std::uint64_t>(buffC));
    }

    // c -= a
    {
        std::span<const std::uint64_t> aSpan(buffA);
        subInPlace(buffC, aSpan, buffC);
    }
    // c -= b
    {
        std::span<const std::uint64_t> bSpan(buffB);
        subInPlace(buffC, bSpan, buffC);
    }

    // Now combine into R:
    // R = a << (2*half*64) + c << (half*64) + b
    // We'll clear R first
    std::fill(R.begin(), R.end(), 0ULL);

    // Add b into R (lowest part)
    {
        std::span<const std::uint64_t> bSpan(buffB);
        addInPlace(bSpan, {}, R);
    }
    // Add c shifted by half*64 bits => c starts at index half in R
    {
        std::span<const std::uint64_t> cSpan(buffC);
        if (!cSpan.empty()) {
            std::span<std::uint64_t> cDest = (half < R.size())
                ? R.subspan(half) : std::span<std::uint64_t>();
            addInPlace(cSpan, {}, cDest);
        }
    }
    // Add a shifted by 2*half*64 bits => a starts at index 2*half in R
    {
        std::span<const std::uint64_t> aSpan(buffA);
        if (!aSpan.empty()) {
            std::size_t idx = 2ULL * half;
            std::span<std::uint64_t> aDest = (idx < R.size())
                ? R.subspan(idx) : std::span<std::uint64_t>();
            addInPlace(aSpan, {}, aDest);
        }
    }
}

//--------------------------------------------
// A templated Number class that uses any ContiguousContainer of uint64_t
// for storage. Provides fromHex(), toHex(), and a static karatsuba() for multiplication.
//--------------------------------------------
template <ContiguousContainer Container = std::vector<std::uint64_t>>
class Number {
public:
    using DigitT = std::uint64_t;
    Container digits; // Little-endian: digits[0] is the least significant 64 bits

    Number() = default;
    explicit Number(std::size_t size) : digits(size, 0ULL) {}

    // Construct from a Container directly
    explicit Number(const Container& c) : digits(c) {}
    explicit Number(Container&& c) : digits(std::move(c)) {}

    //----------------------------------------
    // Convert from a hex string
    // Reads an arbitrary-length hex string into 64-bit "digits"
    // Each digit is up to 16 hex chars => 64 bits
    //----------------------------------------
    static Number fromHex(const std::string& hexStr) {
        // Remove any leading 0x or so, and ignore spaces
        std::string s;
        s.reserve(hexStr.size());
        for (char c: hexStr) {
            if (std::isspace((unsigned char)c)) continue;
            if (c == 'x' || c == 'X') continue;
            if (c == '0' && s.empty()) continue; // skip leading zero?
            s.push_back(std::tolower((unsigned char)c));
        }
        if (s.empty()) {
            // Means it was "0" or blank
            Number num;
            num.digits.resize(1, 0ULL);
            return num;
        }

        // We'll parse in chunks of up to 16 hex digits at a time (64 bits).
        Number num;
        for (std::size_t i = s.size(); i > 0; ) {
            std::size_t chunkSize = (i >= 16) ? 16 : i;
            std::size_t start = i - chunkSize;
            std::uint64_t chunkVal = 0ULL;
            for (std::size_t j = start; j < i; ++j) {
                char c = s[j];
                chunkVal <<= 4;
                if (c >= '0' && c <= '9') {
                    chunkVal |= (c - '0');
                } else {
                    chunkVal |= ((c - 'a') + 10);
                }
            }
            num.digits.push_back(chunkVal);
            i -= chunkSize;
        }

        // Remove leading zeros if any got introduced
        while (num.digits.size() > 1 && num.digits.back() == 0ULL) {
            num.digits.pop_back();
        }
        return num;
    }

    //----------------------------------------
    // Convert to hex string
    //----------------------------------------
    std::string toHex() const {
        // If all digits are zero
        if (digits.empty()) return "0";
        bool isAllZero = true;
        for (auto d: digits) {
            if (d != 0ULL) {
                isAllZero = false;
                break;
            }
        }
        if (isAllZero) {
            return "0";
        }

        // Find the highest non-zero digit
        std::size_t idx = digits.size() - 1;
        // Convert it without leading zeros
        std::uint64_t val = digits[idx];
        // Count how many hex digits we actually need
        int leadingHexDigits = 0;
        {
            std::uint64_t tmp = val;
            while (tmp != 0ULL) {
                tmp >>= 4;
                leadingHexDigits++;
            }
        }
        if (leadingHexDigits == 0) leadingHexDigits = 1; // at least 1 digit

        // Write that part
        static const char* hexdigits = "0123456789abcdef";
        std::string result;
        result.reserve(digits.size() * 16);

        for (int shift = 4*(leadingHexDigits-1); shift >= 0; shift -= 4) {
            int nib = (int)((val >> shift) & 0xF);
            result.push_back(hexdigits[nib]);
        }
        // Then handle remaining digits
        while (idx > 0) {
            --idx;
            val = digits[idx];
            // Always print 16 hex digits for these
            for (int shift = 60; shift >= 0; shift -= 4) {
                int nib = (int)((val >> shift) & 0xF);
                result.push_back(hexdigits[nib]);
            }
        }
        // Remove any leading zeros introduced in the last chunk
        // Actually not needed because we only remove them from the top chunk.
        // But if you want to strip them:
        // while (result.size() > 1 && result[0] == '0') {
        //     result.erase(result.begin());
        // }

        return result;
    }

    //----------------------------------------
    // Multiply using Karatsuba, producing a new Number
    // This sets up the destination and calls karatsubaRec.
    //----------------------------------------
    static Number karatsuba(const Number& A, const Number& B) {
        // Max size = A.size()+B.size()
        Number result(A.digits.size() + B.digits.size());
        // zero the result
        std::fill(result.digits.begin(), result.digits.end(), 0ULL);

        std::span<const std::uint64_t> Aspan(A.digits.data(), A.digits.size());
        std::span<const std::uint64_t> Bspan(B.digits.data(), B.digits.size());
        std::span<std::uint64_t>       Rspan(result.digits.data(), result.digits.size());

        karatsubaRec(Aspan, Bspan, Rspan);

        // Remove leading zero digits
        while (result.digits.size() > 1 && result.digits.back() == 0ULL) {
            result.digits.pop_back();
        }
        return result;
    }

    //----------------------------------------
    // For convenience, print with toHex()
    //----------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const Number& num) {
        os << num.toHex();
        return os;
    }
};

//--------------------------------------------
// Main demo
//--------------------------------------------
int main() {
    using BigNumber = Number<std::vector<std::uint64_t>>;
    std::string hexA, hexB;

    std::cout << "Enter first number in hex: ";
    std::cin >> hexA;
    std::cout << "Enter second number in hex: ";
    std::cin >> hexB;

    BigNumber A = BigNumber::fromHex(hexA);
    BigNumber B = BigNumber::fromHex(hexB);

    BigNumber C = BigNumber::karatsuba(A, B);

    std::cout << "Karatsuba product (hex): " << C << std::endl;
    return 0;
}
