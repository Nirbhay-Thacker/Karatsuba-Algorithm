#ifndef NUM_CPP_INCLUDED
#define NUM_CPP_INCLUDED
#include "num.hpp"
#include<string>
#include<iostream>
#include<cstdint>
#include<stdexcept>
#include<regex>

using namespace std;
//********************Constructors********************

//Parametrized Constructor (allocates memory and optionally initialises to zero)
unsigned_num::unsigned_num(const digit length,bool zero_init):len(length)
{
    if(zero_init)
        ptr=static_cast<digit*>(calloc(length,sizeof(digit)));
    else
        ptr=static_cast<digit*>(malloc(len*sizeof(digit)));
    if(ptr==nullptr)
    {
        bad_alloc b;
        throw b;
    }
}

//Copy constructor
unsigned_num::unsigned_num(const unsigned_num& original):unsigned_num(original.len)
{
    for(digit i=0;i<original.len;i++) ptr[i]=original.ptr[i];
}

//Move constructor
unsigned_num::unsigned_num(unsigned_num&& source):ptr(source.ptr),len(source.len)
{
    source.ptr=nullptr;//free(nullptr) is safe
    source.len=0;
}

//operator= performs a deep copy if source is an lvalue, else it uses the move constructor. It is dangerous to call unsigned_num x(1,1); x=move(x); So I added a check to prevent UB
unsigned_num& unsigned_num::operator=(unsigned_num assignment)
{
    if(ptr==assignment.ptr) throw invalid_argument("unsigned_num::operator= : Is this a personal attack or something? Because I'm too robust for such failure");
    swap(ptr,assignment.ptr);//swap might be useful if I was to implement this with STL vector
    swap(len,assignment.len);
    return *this;
}
//String Conversion Constructor
//expects no separators and in contrast to the actual storage, which is little endian, each letter and word(8 non-whitespace characters) is interpreted as big endian to maintain left to right and top to bottom ordering
unsigned_num::unsigned_num(const string s,const digit base):unsigned_num(s.length()/8+(bool)(s.length()%8))
{
    if(base>36) throw invalid_argument("initialize_from_string: base cannot be greater than 36, digits can be represented 0 to 9 and then A to Z");
    if(base!=16) throw invalid_argument("Sorry this is a work in progress, please input in hex only");
    signed_digit temp,word=0;
    for(digit i=0;i<s.length();i++)
    {
        temp=((s[i]>='A')?(10+s[i]-'A'):(s[i]-'0'));
        if(temp>16 || temp<0) throw invalid_argument("unsigned num(const string s, const digit base): s contains non-hex characters");
        word+=temp<<4*(7-i%8);//note that while left shifts are ok in 2's complement, right shifts are not equivalent to arithmetic division, even with sign extension
        if((i+1)%8==0)
        {
            ptr[len-(i+1)/8]=word;
            word=0;
        }
    }
    if(s.length()%8)
    {
        ptr[0]=word;
    }
}

//********************I/O********************

digit& unsigned_num::operator[](const digit i){
    if(ptr==nullptr) throw invalid_argument("unsigned_num::operator[]: unsigned_num is empty");
    if(len<=i)
    {
        invalid_argument e("unsigned_num::operator[]: index out of bounds");
        throw e;
    }
    return ptr[i];
}
//const version
digit unsigned_num::operator[](const digit i) const
{
    if(ptr==nullptr) throw invalid_argument("unsigned_num::operator[]: unsigned_num is empty");
    if(len<=i)
    {
        invalid_argument e("unsigned_num::operator[]: index out of bounds");
        throw e;
    }
    return ptr[i];
}

//Allows overloading cin and cout for HEX input and output
//Assumes hex input separated by '\n' and expects input until input is full or the '&' is reached, a bit unconventional (opposed to whitespace) but more readable
istream& operator>>(istream& is,unsigned_num& input)
{
    string input_string;
    getline(is,input_string,'&');
    //Acknowledgments to https://www.techiedelight.com/remove-whitespaces-string-cpp/#:~:text=Since%20std%3A%3Aremove_if%20algorithm,call%20to%20std%3A%3Aerase%20. for the following two lines
    regex r("\\s+");//These two lines are a delete of any whitespace character in input_string
	input_string =regex_replace(input_string, r, "");

    input=unsigned_num(input_string);
    return is;
}

string to_HEXstring(digit x)
{
    string output;
    output.resize(digit_bits/4);
    unsigned int y;
    char c;
    for(auto i=output.rbegin(); i!=output.rend(); i++)
    {
        y=x%16;
        c=y<10?'0'+y:'A'+y-10;
        *i=c;
        x/=16;
    }
    return output;
}

ostream& operator<<(ostream& os,const unsigned_num& output)
{
    for(digit i=output.len;i--;) //Because digit is an unsigned type, for(digit i=output.len-1;i>=0;i--) will never terminate due to wrap-around, there are many versions of this statement, this is a personal preference as it exploits the unsafe bool
        os<<to_HEXstring(output[i])<<endl;
    return os;
}

//returns Most Significant Word
signed_digit& unsigned_num::MSW()
{
    if(ptr==nullptr) throw invalid_argument("unsigned_num::MSW():Array is empty");
    return ((signed_digit*)ptr)[len-1];//The second expression I encountered throughout this project that needed parentheses to correct for operator precedence
}
//const version
signed_digit unsigned_num::MSW() const
{
    if(ptr==nullptr) throw invalid_argument("unsigned_num::MSW():Array is empty");
    return ((signed_digit*)ptr)[len-1];
}

//********************Properties********************

//is():
//INPUT (unsigned_num)    `MNN 0   1   MPN  (:.MNN and MPN of the word size of the unsigned_num)
//RETURN (signed_digit)    MNN 0   1   MPN
signed_digit unsigned_num::is() const
{
    if(ptr==nullptr || len==0) throw invalid_argument("unsigned_num::is() : calling object is empty");
    bool zero1MNN=true;
    for(digit i=1;zero1MNN && i<len-1;i++) zero1MNN=(ptr[i]==0);
    if(zero1MNN)
    {
        if(MSW()==MNN && (*ptr==0 ||len==1)) return MNN;
        if(MSW()==0 && *ptr==0) return 0;
        if((MSW()==0||len==1) &&*ptr==1) return 1;
    }
    bool MPN_or_MinusOne=true;
    for(digit i=0;MPN_or_MinusOne && (i<len-1);i++) MPN_or_MinusOne=(ptr[i]==MAX);
    if(MPN_or_MinusOne)
    {
        if(MSW()==MPN) return MPN;
        if(MSW()==-1) return -1;
    }
    return 2;//Default return value
}

bool unsigned_num::isZero() const
{
    bool zero=true;
    for(digit i=0;zero && i<len;i++)
        zero=(!ptr[i]);
    return zero;
}

bool unsigned_num::isMNN() const
{
    bool _isMNN=true;
    if(MSW()==MNN) {for(digit i=0;MNN && i<len-1;i++) _isMNN=!ptr[i];}
    else _isMNN=false;
    return _isMNN;
}

bool operator<(const unsigned_num& op1,const unsigned_num& op2)
{
    bool conforms=op1.len<=op2.len;
    const unsigned_num& OP=conforms?op2:op1;
    const unsigned_num& op=conforms?op1:op2;

    digit i=OP.len-1;
    for(;!OP[i] && i;i--);//--i prevents i from wrapping being wrapped around on the final decrement, side effect is that prevents the for loop running after i=1 (i.e. i=0 after execution of loop), which is no problem here
    digit j=op.len-1;
    for(;!op[j] && j;j--);
    if(i>j)
    {
        return conforms;
    }
    else
    {
        if(i<j)
        {
            return !conforms;
        }
        else
        {
            for(;op[i]==OP[i] && i;i--);//This version prevents i from wrap around error even if i starts out at 0, this safety is not there in the other methods, the speed benefit in coding or implementation behind these funky loops is not worth the input time to decide and make these comments, it is fun though
            return op[i]!=OP[i] && !(conforms^(op[i]<OP[i]));//brackets necessary to prevent undesired operator precedence, op[i]==OP[i] can happen as for loops does not check equality once i=0
        }
    }
}
//********************Resize********************

void unsigned_num::reallocate(const digit new_len,bool retain_HIGHside,bool signExtend){
    if(!new_len)
    {
            free(ptr);
            return;
    }
    if(len==new_len) return;
	//Moves the HIGH bits to the LOW side (i.e. end to start) before reallocating
	if(new_len<len && retain_HIGHside)
        for(digit i=0;i<new_len;i++)
            ptr[i]=ptr[len-new_len+i];
    ptr=static_cast<digit*>(realloc(ptr,new_len*sizeof(digit)));
    if(ptr==nullptr)
    {
        bad_alloc e;
        throw e;
    }
    if(signExtend)
    {
        digit sign=MSW()<0?-1:0;
        for(digit i=len;i<new_len;i++) ptr[i]=sign;
    }
    else for(digit i=len;i<new_len;i++) ptr[i]=0;
    len=new_len;
}
void unsigned_num::shrink_to_fit(bool signed_shrink)
{
    if(!len || ptr==nullptr) return;
    digit i=len-1;
    if(signed_shrink)
    {
        for(;(ptr[i]==static_cast<digit>(-1) || ptr[i]==0) && i;i--);
    }
    else
    {
        for(;ptr[i]==0 && i;i--);
    }
    reallocate(i+1);
}

//********************Arithmetic********************

unsigned_num& unsigned_num::shift_and_add(const unsigned_num& addend,const digit LEFT, bool ignoreCarry)
{
    bool carry=false;
    if(len<LEFT+addend.len)
    {
    digit old_len=len;
    reallocate(LEFT+addend.len);//There is no overflow expected in the Karatsuba Algorithm
    for(digit i=old_len;i<LEFT+addend.len;i++) ptr[i]=0;
    }
    for(digit i=LEFT;i<LEFT+addend.len;i++)
    {
        carry=__builtin_add_overflow(carry,ptr[i],ptr+i)+__builtin_add_overflow(ptr[i],addend[i-LEFT],ptr+i);//operator || is short circuiting, + retains the same characteristic
    }
    for(digit i=LEFT+addend.len;carry && i<len;i++) carry=__builtin_add_overflow(ptr[i],carry,ptr+i);
    if(carry && !ignoreCarry)
    {
        reallocate(len+1);
        MSW()=1;
    }
    return *this;
}

unsigned_num operator+(const unsigned_num& Augend, const unsigned_num& Addend)
{
    if(!Augend.is() || !Addend.is()) return unsigned_num(1,true);
    unsigned_num result(Augend);
    result.shift_and_add(Addend,0,false);
    result.shrink_to_fit();
    return result;
}

unsigned_num operator-(const unsigned_num& subtrahend)
{
    //if unsigned_number is 0, this is faster than normal method
    if(!subtrahend.is())
    {
        unsigned_num result(1,true);
        return result;
    }
    bool extend=(subtrahend.MSW()<0);//The input is assumed to be unsigned, as we are using type unsigned_num, if it exceeds MPN it needs an extra word
    unsigned_num result(extend?subtrahend.len+1:subtrahend.len);
    for(digit i=0;i<subtrahend.len;i++)
        result[i]=~subtrahend.ptr[i];
    if(extend) result.MSW()=-1;
    bool carry=true;
    for(digit i=0;carry && i<result.len;i++)
        carry=__builtin_add_overflow(result[i],1,(result.ptr+i));
    //-0001=1110+1=1111
    //-1001=0110+1=0111
    return result;
}
//Return the (positive) difference between two numbers, equivalent to |a-b|
unsigned_num operator-(const unsigned_num& minuend,const unsigned_num& subtrahend)
{
     unsigned_num result=(-subtrahend);
     if(result.len<minuend.len) result.reallocate(minuend.len, false, true);
     //note that minuend.len+1 isn't used as minuend and subtrahend are expected to be unsigned, i.e. no signed overflow possible for
     //minuend-subtrahend, even the negative result is negated again to return as a positive difference
     result.shift_and_add(minuend,0,true);
     if(!(result<minuend) && !subtrahend.isZero()) result=-result;//condition for negative answer with unsigned minuend and signed result
     result.shrink_to_fit();
     return result;
}

//Multiplication

//Shift and add multiplication
//digit * num
unsigned_num multiply_shift_add(const unsigned_num& multiplicand,const digit multiplier)
{
    signed_digit multiplicand_is=multiplicand.is();
    if(multiplier==0 || !multiplicand_is) return unsigned_num(1,true);
    if(multiplicand_is==1) {unsigned_num temp(1); temp[0]=multiplier; return temp;}
    if(multiplier==1) {return unsigned_num(multiplicand);}

    if(multiplicand_is==-1 && multiplier==MAX)//maximum unsigned unsigned_number is -1 in the two's complement system
    {
        unsigned_num result(multiplicand.len+1);
        for(digit i=1;i<result.len-1;i++) result[i]=-1;
        result[0]=1;
        result.MSW()=-2;
        return result;
    }
    //Are there any more such EASY patterns?
    two_digit product;
    unsigned_num result(multiplicand.len+1);
    result[0]=0;
    for(digit i=0;i<multiplicand.len;i++)
    {
        product=static_cast<two_digit>(multiplicand[i])*static_cast<two_digit>(multiplier);
        result[i+1]=__builtin_add_overflow(static_cast<digit>(product),result[i],(result.ptr+i));
        result[i+1]+=(product>>digit_bits);//This does not overflow, as max value of MAX*MAX is 11111111111111111111111111111110 00000000000000000000000000000000
    }
    return result;
}
//num*num
unsigned_num multiply_shift_add(const unsigned_num& Multiplier,const unsigned_num& Multiplicand)
{
    //All cases of unsigned multiplicative identity
    if(!Multiplier.is() || !Multiplicand.is()) return unsigned_num(1,true);
    if(Multiplicand.is()==1) {if(Multiplier.is()==1) {unsigned_num temp(1);temp[0]=1;return temp;} else {return unsigned_num(Multiplier);}}
    if(Multiplier.is()==1) return unsigned_num(Multiplicand);
    //Add the MAX*MAX case
    const unsigned_num& multiplier=Multiplicand.len<Multiplier.len?Multiplicand:Multiplier;
    const unsigned_num& multiplicand=Multiplicand.len<Multiplier.len?Multiplier:Multiplicand;
    unsigned_num product(multiplicand.len+multiplier.len,true);
    for(digit i=0;i<multiplier.len;i++) product.shift_and_add(multiply_shift_add(multiplicand,multiplier[i]),i);
    return product;
}

//creating a new unsigned_num for sub_digits is a major performance kill (so this does not). Copying increases the space requirements log2(n) times, not to mention CPU time. However as log2(n)*n^log2(3) grows slower than n^2, KaratsubaMultiplication using duplication is still asymptotically faster
unsigned_num unsigned_num::sub_digits(const digit start,const digit SIZE) const
{
    if(len<start+SIZE) throw invalid_argument("sub_digits(digit start, digit SIZE) : parameters (start+SIZE) exceed unsigned_number.len");
    if(!SIZE) throw invalid_argument("sub_digits(digit start, digit SIZE) : SIZE cannot be zero");
    //return unsigned_num(ptr+start,SIZE);
    unsigned_num a(SIZE);
    for(digit i=start; i<start+SIZE; i++) a[i-start]=ptr[i];
    return a;
}

unsigned_num operator*(const unsigned_num& Multiplicand,const unsigned_num& Multiplier)
{
    //choose dynamically between multiply_shift_add and Karatsuba's Algorithm
    if(!Multiplicand.is() || !Multiplier.is())
    {
        unsigned_num a(1,true);
        // cout<<"Z\n"<<a;
        return a;
    }
    const unsigned_num& multiplier=Multiplicand.len<Multiplier.len?Multiplicand:Multiplier;
    const unsigned_num& multiplicand=Multiplicand.len<Multiplier.len?Multiplier:Multiplicand;

    if(!(multiplier.len-1))
    {
        unsigned_num a(multiply_shift_add(multiplicand,multiplier[0]));
        // cout<<"Z\n"<<a;
        return a;
    }
    unsigned_num Z(multiplicand.len+multiplier.len,true);
    digit Q=multiplier.len/2;
    //beware: unsigned_num sub_digits(const size_t start,const size_t SIZE) not unsigned_num sub_digits(const size_t start, const size_t end)
    const unsigned_num M_l(multiplicand.sub_digits(0,Q));
    const unsigned_num M_h(multiplicand.sub_digits(Q,multiplicand.len-Q));//M={0 1 2 3 4 5}, Q=3,len=6, multiplicand.sub_digits(0,3) returns {0 1 2} and multiplicand.sub_digits(3,3) returns {3 4 5}
    const unsigned_num m_l(multiplier.sub_digits(0,Q));
    const unsigned_num m_h(multiplier.sub_digits(Q,multiplier.len-Q));
    // cout<<"Multiplicand high\n"<<M_h<<endl;
    // cout<<"Multiplicand low\n"<<M_l<<endl;
    // cout<<"Multiplier high\n"<<m_h<<endl;
    // cout<<"Multiplier low\n"<<m_l<<endl;
    const unsigned_num z0(M_l*m_l);
    const unsigned_num z2(M_h*m_h);
    const unsigned_num z1((M_l+M_h)*(m_l+m_h)-z0-z2);
    // cout<<"z2\n"<<z2<<endl;
    // cout<<"z1\n"<<z1<<endl;
    // cout<<"z0\n"<<z0<<endl;
    Z.shift_and_add(z0,0);
    Z.shift_and_add(z1,Q);
    Z.shift_and_add(z2,2*Q);
    // cout<<"Z\n"<<Z;
    return Z;
}
//unsigned_num dummy(const unsigned_num& A, const unsigned_num& b);
#endif // BIGNUM_CPP_INCLUDED
