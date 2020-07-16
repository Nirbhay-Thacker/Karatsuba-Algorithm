//Acknowledgements to Dhaval Bothra for introducing me to the Karatsuba Algorithm and to Jinay Shah for helping me debug, identifying sub_digits as the cause of errors in operator* and suggesting a fix.
#ifndef NUM_HPP_INCLUDED
#define NUM_HPP_INCLUDED

#include<iostream>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include <string>

using namespace std;

typedef uint32_t digit;
typedef int32_t signed_digit;
typedef uint64_t two_digit;

const unsigned int digit_bits=32;
const digit MAX=numeric_limits<digit>::max();
const digit MIN=numeric_limits<digit>::min();
const signed_digit MNN=numeric_limits<signed_digit>::min();
const signed_digit MPN=numeric_limits<signed_digit>::max();
const digit MAX_SIZE=SIZE_MAX/sizeof(digit);

//class num;//Forward declaration is necessary as identifier num is used inside unsigned_num

class unsigned_num{
    protected:
    digit* ptr;
    digit len;

    public:
    //constructors
    unsigned_num():ptr(nullptr),len(0){};
    unsigned_num(digit length,bool zero_init=false);
    unsigned_num(digit* PTR,digit LEN):ptr(PTR),len(LEN){};
    unsigned_num(const unsigned_num& original);//copy constructor
    unsigned_num(unsigned_num&& source);//move constructor
    unsigned_num& operator=(unsigned_num assignment);

    unsigned_num(string s, unsigned int base=16);
    //destructor
    ~unsigned_num(){free(ptr);};
    //Is this gonna work
    //I/O
    digit& operator[](const digit i);   digit operator[](const digit i) const;
    friend ostream& operator<<(ostream& os,const unsigned_num& output);
    friend istream& operator>>(istream& is,unsigned_num& input);
    signed_digit& MSW();    signed_digit MSW() const;
    //Properties
    digit length()const{return len;}
    signed_digit is() const;
    bool isZero() const;
    bool isMNN() const;
    friend bool operator<(const unsigned_num& op1,const unsigned_num& op2);
    //Resize
    void reallocate(digit new_len,bool retain_HIGH=false, bool signExtend=false);
    void shrink_to_fit(bool signed_shrink=false);//num objects call this function with signed_num=true
    unsigned_num sub_digits(const digit start, const digit SIZE) const;

    unsigned_num& shift_and_add(const unsigned_num& addend, const digit LEFT,bool ignoreCarry=false);
    friend unsigned_num operator+(const unsigned_num& Augend,const unsigned_num& Addend);
    friend unsigned_num operator-(const unsigned_num& subtrahend);
    friend unsigned_num operator-(const unsigned_num& minuend, const unsigned_num& subtrahend);
    friend unsigned_num multiply_shift_add(const unsigned_num& multiplicand, const digit multiplier);
    friend unsigned_num multiply_shift_add(const unsigned_num& multiplicand, const unsigned_num& multiplier);
    friend unsigned_num operator*(const unsigned_num& multiplicand,const unsigned_num& multiplier);
};
#endif // NUM_HPP_INCLUDED
