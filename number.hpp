//Pseudocode

//concept required here to limit to contiguous containers
template<template <typename T, typename... args> typename C, size_t base = 2>
class Number{
    private:
    C<T, args...> mantissa;
    public:
    Number(size_t len) : mantissa(*(new C<T>(len))){} //does new throw bad_alloc? What would I even do if I caught it?
    Number(Number&&) //Do I need std::move()? Why is that not a language feature but a library feature
    
    operator[](size_t i)
    operator=(const Number& <C<T>>)

    trim();
    ~Number();//Does this do anything special?
    friends:
     operator+(const Number<C<T>> summand, const Number<C<T>> Summand)
     operator-(const Number<C<T>> minuend, const Number<C<T>> subtrahend)
     operator==(const Number<C<T>> comparand, const Number<C<T>> Comparand)
     operator<(const Number<C<T>> comparand, const Number<C<T>> Comparand)
     operator>(const Number<C<T>> comparand, const Number<C<T>> Comparand)
     operator*(const Number<C<T>> multiplicand, const Number<C<T>> multiplier)
     operator/(const Number<C<T>> divident, const Number<C<T>> divisor)
     operator^(const Number<C<T>> base, const Number<C<T>> exponent)
}
//check implicit conversion to span is possible, i.e. implicit conversion of templatized class is from memberwise implicit conversion
//find typetraits way to get integral type twice as large
//overflow behaviour is not defined correct?
//have metaprogramming without stupid #defines I don't think this language allows it
//no aliases for names no nothing this language broke
