#include <stdexcept>
#include <cstdint>
#include <limits>
#include <string>
using namespace std;
#ifndef NUM_H_INCLUDED
#define NUM_H_INCLUDED
typedef uint32_t digit;
typedef int32_t signed_digit;
const unsigned int digit_bits=32;
typedef uint64_t two_digit;
enum retain{HIGH,LOW};
enum ignoreCarry{TRUE,FALSE};
const digit MAX=numeric_limits<digit>::max();
const digit MIN=numeric_limits<digit>::min();
const signed_digit MNN=numeric_limits<signed_digit>::min();
const signed_digit MPN=numeric_limits<signed_digit>::max();
const digit MAX_SIZE=SIZE_MAX/sizeof(digit);
class unsigned_num{
    protected:
    digit* ptr;
    digit len;

    public:
    unsigned_num():ptr(nullptr),len(0){};//default constructor
    explicit unsigned_num(const digit length);
    unsigned_num(digit* PTR,digit LEN):ptr(PTR),len(LEN){};
    unsigned_num(const digit length,const digit val);//remove this, it was for ease of testing only, make sure to replace all uses of it with the code
    unsigned_num(const string s,const digit base);
    unsigned_num(const unsigned_num& original);//copy constructor
    unsigned_num(unsigned_num&& source);//move constructor
    unsigned_num& operator=(unsigned_num assignment);//assignment operator, dynamically chooses between move and copy constructor depending on whether source is rvalue or lvalue
    ~unsigned_num(){free(ptr);}//destructor

    digit length() const{return len;}//to prevent unintentional modification of len
    //ptr should be kept abstracted
    //digit* ptr()(return ptr);
    signed_digit is() const;
    bool isZero() const;
    bool isMNN() const;
    signed_digit& MSW();
    signed_digit MSW() const;
    digit& operator[](const digit i);
    digit operator[](const digit i) const;
    void reallocate(const digit length,const retain r=HIGH);
    unsigned_num& shift_and_add(const unsigned_num& addend,const digit LEFT, const ignoreCarry C=FALSE);
    unsigned_num sub_digits(const digit start,const digit SIZE) const;

    friend unsigned_num operator-(const unsigned_num& subtrahend);
    friend unsigned_num operator-(const unsigned_num& minuend, const unsigned_num& subtrahend);//returns the difference, does not give a wrap around result like builtin integer types when minuend is larger
    friend unsigned_num operator+(const unsigned_num& Augend, const unsigned_num& Addend);//calls copy constructor or move constructor according to lvalue or rvalue
    friend bool operator<(const unsigned_num& op1,const unsigned_num& op2);
    friend unsigned_num multiply_shift_add(const unsigned_num& multiplicand,const digit multiplier);
    friend unsigned_num multiply_shift_add(const unsigned_num& multiplier, const unsigned_num& multiplicand);
    friend unsigned_num operator*(const unsigned_num& multiplicand,const unsigned_num& multiplier);
    friend unsigned_num multKaratsuba(const unsigned_num& multiplicand,const unsigned_num& multiplier);

};
class num:public unsigned_num
{
public:

    num(){};
    num(const digit length):unsigned_num(length){};
    num(digit* PTR, digit LEN):unsigned_num(PTR,LEN){};
    num(const num& original):unsigned_num(original){};
    num(num&& source):unsigned_num(source){};
    num& operator=(num assignment);
    num(const unsigned_num& original):unsigned_num(original){};
    num(const unsigned_num&& source):unsigned_num(source){};

    friend num operator-(const num& subtrahend);
    friend bool operator<(const num& op1, const num& op2);
    friend num operator*(const num& multiplicand, const num& multiplier);
    friend num signed_multiply_shift_add(const num& multiplicand,const num& multiplier);
    friend num operator-(const num& minuend, const num& subtrahend);
    friend num operator+(const num& Augend,const num& Addend);
    friend num operator*(const num& Multiplicand, const num& Multiplier);
};

#endif // NUM_H_INCLUDED

