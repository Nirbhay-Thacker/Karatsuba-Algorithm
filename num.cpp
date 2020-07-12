#include "num.hpp"
#include<string>
#include<iostream>
//This is an implementation of the Karatsuba multiplication algorithm.
//B=2^32, little endian system

//returns product of two unsigned unsigned_numbers
/*

num num::operator+(const num& operand2) const
{
    const num& temp=len<operand2.len?*this:operand2;
    const num& augend=len<operand2.len?operand2:*this;
    const num& addend=temp;
    if(!augend.is()) return num(addend);
    if(!addend.is()) return num(augend);
    bool operandlengths_equal=(augend.len==addend.len);
    digit sign_extension;//in case length is equal, sign_extension is assigned MSW of addend
    if(!operandlengths_equal)
    {
        sign_extension=static_cast<signed_digit>(addend[addend.len-1])<0 ? -1:0;
    }
    else sign_extension=addend[addend.len-1];

    num sum(augend.len+1);
    sum[0]=0;
    for(digit i=0;i<(operandlengths_equal?(addend.len-1):addend.len);i++)
        sum[i+1]=(bool)(__builtin_add_overflow(augend[i],sum[i],(sum.ptr+i))+__builtin_add_overflow(addend[i],sum[i],(sum.ptr+i)));//operator + is used instead of || as || is short circuiting while + is not. + has the same effect as || for boolean types
    for(digit i=addend.len;i<augend.len-1;i++)
        sum[i+1]=(bool)(__builtin_add_overflow(augend[i],sum[i],(sum.ptr+i))+__builtin_add_overflow(sign_extension,sum[i],(sum.ptr+i)));

    if(__builtin_add_overflow(static_cast<signed_digit>(augend[augend.len-1]),sum[augend.len-1],(signed_digit*)(sum.ptr+augend.len-1))+__builtin_add_overflow(static_cast<signed_digit>(sign_extension),static_cast<signed_digit>(sum[augend.len-1]),(signed_digit*)(sum.ptr+augend.len-1)))
        sum[augend.len]=static_cast<signed_digit>(augend[augend.len-1])<0?-1:0;
    else sum.reallocate(augend.len, LOW);
    return sum;
}
*/

unsigned_num::unsigned_num(const digit length):len(length)
{
    ptr=static_cast<digit*>(malloc(sizeof(digit[len])));
    if(ptr==nullptr)
    {
        bad_alloc b;
        throw b;
    }
}

unsigned_num::unsigned_num(const digit length,const digit val)
{
    switch(val)
    {
    case 0:
        ptr=static_cast<digit*>(calloc(length,sizeof(digit)));
        if(ptr==nullptr)
        {
            bad_alloc b;
            throw b;
        }
        break;
    case numeric_limits<signed_digit>::min():
        ptr=static_cast<digit*>(calloc(length,sizeof(digit)));
        if(ptr==nullptr)
        {
            bad_alloc b;
            throw b;
        }
        break;
    default:
        new(this) unsigned_num(length);
        for(digit i=0;i<length;i++) ptr[i]=val;
        break;
    }
    len=length;
}
//Copy constructor (makes a deep copy ; is also (and ALWAYS for lvalues) implicitly called by operator=)
unsigned_num::unsigned_num(const unsigned_num& original):len(original.len)
{
    new (this) unsigned_num(original.len);
    for(digit i=0;i<original.len;i++) ptr[i]=original.ptr[i];
}
//Move constructor performs the swap idiom with temporaries (rvalues) to prevent destructor (of unsigned_num assignment) from freeing
//memory transferred without making deep copy. Move constructor is also implicitly called by operator= for rvalues
unsigned_num::unsigned_num (unsigned_num&& source):ptr(source.ptr),len(source.len)
{
    source.ptr=nullptr;//free(nullptr) is safe
    source.len=0;
    //swap is not used, because I don't know if ptr could have some random initial value
}
//operator= performs a deep copy if source is an lvalue, else it uses the move constructor. It is dangerous to call unsigned_num x(1,1); x=move(x); So I added a check to prevent UB
unsigned_num& unsigned_num::operator=(unsigned_num assignment)
{
    if(ptr==assignment.ptr) throw invalid_argument("unsigned_num::operator= : Is this a personal attack or something? Because I'm too robust for such failure");
    swap(ptr,assignment.ptr);//swap might be useful if I was to implement this with STL vector
    swap(len,assignment.len);
    return *this;
}

unsigned_num::unsigned_num(const string s,const digit base=16):len(s.length()/8+(bool)(s.length()%8))
{
    new(this) unsigned_num(len,0);//reconstruct calling object with zero initialization
    if(base>36) throw invalid_argument("initialize_from_string: base cannot be greater than 36, digits can be represented 0 to 9 and then A to Z");
    if(base!=16) throw invalid_argument("Sorry this is a work in progress, please input in hex only");
    digit temp,word=0;
    for(digit i=0;i<s.length();i++)
    {
        temp=s[i]-'0';
        if(temp>9) temp=10+s[i]-'A';
        word+=temp<<4*(i%8);
        if((i+1)%8==0)
        {
            ptr[(i+1)/8-1]=word;
            word=0;
        }
    }
    if(s.length()%8!=0) shift_and_add(unsigned_num(1,word),(s.length()/8));
}
signed_digit& unsigned_num::MSW()
{
    if(ptr==nullptr) throw invalid_argument("unsigned_num::MSW():Array is empty");
    return ((signed_digit*)ptr)[len-1];//The expression I encountered throughout this project that needed parentheses to correct for operator precedence
}
signed_digit unsigned_num::MSW() const
{
    if(ptr==nullptr) throw invalid_argument("unsigned_num::MSW():Array is empty");
    return ((signed_digit*)ptr)[len-1];
}
signed_digit unsigned_num::is() const
{
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

digit& unsigned_num::operator[](const digit i){
    if(ptr==nullptr) throw invalid_argument("unsigned_num::operator[]: unsigned_num is empty");
    if(len<=i)
    {
        invalid_argument e("unsigned_num::operator[]: index out of bounds");
        throw e;
    }

    return ptr[i];
}
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
void unsigned_num::reallocate(const digit new_len,const retain r){
    if(!new_len)
    {
            free(ptr);
            exit(0);
    }
    if(len==new_len) exit(0);
	//Moves the HIGH bits to the LOW side (i.e. end to start) before reallocating
	if(new_len<len && r==HIGH)
        for(digit i=0;i<new_len;i++)
            ptr[i]=ptr[len-new_len+i];
    ptr=static_cast<digit*>(realloc(ptr,sizeof(digit[new_len])));
    if(ptr==nullptr)
    {
        bad_alloc e;
        throw e;
    }
    //Sign extends
    if(len<new_len)
    {
        digit append_MSB=static_cast<signed_digit>(ptr[len-1])<0?-1:0;
        for(digit i=len;i<new_len;i++)ptr[i]=append_MSB;
    }
    len=new_len;
}

bool operator<(const unsigned_num& op1,const unsigned_num& op2)
{
    const unsigned_num& OP=op1.len<op2.len?op2:op1;
    const unsigned_num& op=op1.len<op2.len?op1:op2;
    bool conforms=op1.len<op2.len;
    digit i=OP.len;
    while(!OP[i] && i>0) i--;
    digit j=op.len;
    while(!op[i] && j>0) j--;
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
            digit k=i;
            while(OP[k]==op[k] && k>0) k--;
            return conforms^(op[k]<OP[k]);
        }
    }
}
unsigned_num operator+(const unsigned_num& Augend, const unsigned_num& Addend)
{
    const unsigned_num& temp=Addend.len<Augend.len?Addend:Augend;
    const unsigned_num& augend=Addend.len<Augend.len?Augend:Addend;
    const unsigned_num& addend=temp;
    if(!augend.is()) return unsigned_num(addend);
    if(!addend.is()) return unsigned_num(augend);
    unsigned_num result(augend.len+1);
    result[0]=0;
    for(digit i=0;i<addend.len;i++)
        result[i+1]=(bool)(__builtin_add_overflow(addend[i],result[i],result.ptr+i)+__builtin_add_overflow(augend[i],result[i],result.ptr+i));
    for(digit i=addend.len;i<augend.len;i++)
        result[i+1]=__builtin_add_overflow(augend[i],result[i],result.ptr+i);
    if(result.MSW()==0) result.reallocate(result.len-1,LOW);
    return result;
}
//Unary Negation returns 2's complement of unsigned_number
unsigned_num operator-(const unsigned_num& subtrahend)
{
    //if unsigned_number is MNN, result needs to be one bit (in this case an additional 32-bit digit is allocated) larger than unsigned_number
    //-1000=8=1000
    if(subtrahend.is()==MNN)
    {
        unsigned_num result(subtrahend.len+1,0);
        result[subtrahend.len-1]=MNN;
        result[subtrahend.len]=-1;
        return result;
    }
    //if unsigned_number is 0, this is faster than normal method
    if(!subtrahend.is())
    {
        unsigned_num result(1,0);
        return result;
    }
    unsigned_num result(subtrahend.len);
    for(digit i=0;i<subtrahend.len;i++)
        result[i]=~subtrahend.ptr[i];
    bool carry=true;
    for(digit i=0;carry && i<subtrahend.len;i++)
        carry=__builtin_add_overflow(result[i],1,(result.ptr+i));
    //-0001=1110+1=1111
    //-1001=0110+1=0111
    return result;
}
//returns the positive difference
unsigned_num operator-(const unsigned_num& minuend,const unsigned_num& subtrahend)
{
     unsigned_num result=(-subtrahend);
     digit negative_addend_length=result.len;
     unsigned_num minusone(1,-1);
     result=result+minuend;
     for(digit i=negative_addend_length;i<minuend.len-1;i++) result.shift_and_add(minusone,i);
     if(negative_addend_length<minuend.len) result.shift_and_add(minusone,minuend.len-1,TRUE);
     return result;
}
unsigned_num& unsigned_num::shift_and_add(const unsigned_num& addend,const digit LEFT, const ignoreCarry C)
{
    bool carry=false;
    if(len<LEFT+addend.len)
    {
    digit old_len=len;
    reallocate(LEFT+addend.len);//There is no overflow expected in the Karatsuba Algorithm. reallocate may wrongly sign extend the unsigned unsigned_number, KaratsubaMultiplication is unaffected by this(MAKE SURE TO ADD Z0, THEN Z1 AND FINALLY Z2 IN THIS ORDER)
    for(digit i=old_len;i<LEFT+addend.len;i++) ptr[i]=0;
    }
    for(digit i=0;i<addend.len;i++)
    {
        carry=__builtin_add_overflow(ptr[i+LEFT],carry,ptr+i+LEFT)+__builtin_add_overflow(ptr[i+LEFT],addend[i],ptr+i+LEFT);//operator || is short circuiting, + retains the same characteristic
    }

    for(digit i=LEFT+addend.len;carry && i<len;i++) carry=__builtin_add_overflow(ptr[i],carry,ptr+i);
    if(carry && C==FALSE)
    {
        reallocate(len+1);
        MSW()=1;
    }
    /*
    for sign extension and overflow, if implemented for it
    if(carry)
    {
        if(!__builtin_add_overflow(static_cast<signed_digit>(ptr[len-2]),1,(signed_digit*)(ptr+len-2)))//this performs the increment irrespective of overflow
            reallocate(len-1, LOW);//Perhaps it may be easier to de-allocate the MSW than to allocate an MSW
        else ptr.MSW()++;
    }
    */
    return *this;
}
unsigned_num multiply_shift_add(const unsigned_num& multiplicand,const digit multiplier)
{
    signed_digit multiplicand_is=multiplicand.is();
    if(multiplier==0 || !multiplicand_is) return unsigned_num(1,0);
    if(multiplicand_is==1) return unsigned_num(1,multiplier);
    if(multiplier==1) return unsigned_num(multiplicand);//while -(-multiplicand) achieves the same result as unsigned_num_copy, it takes CPU time
    two_digit product;
    if(multiplicand_is==-1 && multiplier==MAX)//maximum unsigned unsigned_number is -1 in the two's complement system
    {
        unsigned_num result(multiplicand.len+1, static_cast<digit>(-1));
        result[0]=1;
        result.MSW()=-2;
        return result;
    }
    //Are there any more such EASY patterns?
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

unsigned_num multiply_shift_add(const unsigned_num& multiplier,const unsigned_num& multiplicand)
{
    //All cases of unsigned multiplicative identity
    if(multiplier.is()==0 || !multiplicand.is()) return unsigned_num(1,0);
    if(multiplicand.is()==1) {if(multiplier.is()==1) {return unsigned_num(1,1);} else {return unsigned_num(multiplier);}}
    if(multiplier.is()==1) return unsigned_num(multiplicand);
    //Add the MAX*MAX case
    const unsigned_num& Multiplier=multiplicand.len<multiplier.len?multiplicand:multiplier;
    const unsigned_num& Multiplicand=multiplicand.len<multiplier.len?multiplier:multiplicand;
    unsigned_num product(Multiplicand.len+Multiplier.len,0);
    for(digit i=0;i<Multiplier.len;i++) product.shift_and_add(multiply_shift_add(Multiplicand,Multiplier[i]),i);
    return product;
}

unsigned_num signed_multiply_shift_add(const unsigned_num& Multiplicand,const unsigned_num& Multiplier)
{
    bool product_sign=(Multiplicand.MSW()<0)^(Multiplier.MSW()<0);
    const unsigned_num& multiplicand=(Multiplicand.MSW()<0)?-Multiplicand:Multiplicand;
    const unsigned_num& multiplier=(Multiplicand.MSW()<0)?-Multiplier:Multiplier;

    return product_sign?-multiply_shift_add(multiplicand,multiplier):multiply_shift_add(multiplicand,multiplier);
}
//not implemented for negative addends (ADDEND MUST BE UNSIGNED) or right shifts, should implement such a version later. Inputs of multiplication operations (multiply_shift_add and operator*) can output -1 as a single word, take care to not break the signed shift addition because of that

//creating a new unsigned_num for sub_digits is a major performance kill (so this does not), it increases the space requirements log2(n) times, the copying also takes up alot of time. However as nlog2(n) grows slower than n^2, KaratsubaMultiplication using duplication is still asymptotically faster
unsigned_num unsigned_num::sub_digits(const digit start,const digit SIZE) const
{
    if(len<start+SIZE) throw invalid_argument("sub_digits(digit start, digit SIZE) : parameters (start+SIZE) exceed unsigned_number.len");
    if(!SIZE) throw invalid_argument("sub_digits(digit start, digit SIZE) : SIZE cannot be zero");
    return unsigned_num(ptr+start,SIZE);
}

unsigned_num operator*(const unsigned_num& multiplicand,const unsigned_num& multiplier)
{
    //choose dynamically between multiply_shift_add and multKaratsuba
    const unsigned_num& Multiplicand=multiplier.len<multiplicand.len?multiplicand:multiplier;
    const unsigned_num& Multiplier=multiplier.len<multiplicand.len?multiplier:multiplicand;
    return multKaratsuba(Multiplicand,Multiplier);
}
unsigned_num multKaratsuba(const unsigned_num& multiplicand, const unsigned_num& multiplier)
{
    if(!(multiplier.len-1))
    {
        if(!(multiplicand.len-1))
        {

            unsigned_num result((digit*)new two_digit,2);//this works because 32-bit unsigned_numbers are stored fully packed
            *((two_digit*)(result.ptr))=static_cast<two_digit>(*multiplicand.ptr)*static_cast<two_digit>(*multiplier.ptr);
            return result;
        }
        else return multiply_shift_add(multiplicand,multiplier[0]);
    }
    unsigned_num M_h,M_l,m_l,m_h,z0,z1,z2,Z(multiplicand.len+multiplier.len,0);
    digit Q=multiplier.len/2;
    //beware: unsigned_num sub_digits(const size_t start,const size_t SIZE) not unsigned_num sub_digits(const size_t start, const size_t end)
    M_l=multiplicand.sub_digits(0,Q);
    M_h=multiplicand.sub_digits(Q,multiplicand.len-Q);//M={0 1 2 3 4 5}, Q=3,len=6, multiplicand.sub_digits(0,3) returns {0 1 2} and multiplicand.sub_digits(3,3) returns {3 4 5}
    m_l=multiplier.sub_digits(0,Q);
    m_h=multiplier.sub_digits(Q,multiplier.len-Q);

    z0=M_l*m_l;
    z2=M_h*m_h;
    unsigned_num m1=m_l;
    unsigned_num m2=M_h;
    m1.shift_and_add(m_h,0);
    m2.shift_and_add(M_l,0);
    z1=m1.len<m2.len?m2*m1:m1*m2;
    z1=(z1-z0)-z2;
    Z.shift_and_add(z0,0);
    Z.shift_and_add(z1,Q);
    Z.shift_and_add(z2,2*Q);

    return Z;
}
//member functions of num class

num& num::operator=(num assignment)
{
    if(ptr==assignment.ptr) throw invalid_argument("num::operator= : Is this a personal attack or something? Because I'm too robust for such failure");
    swap(ptr,assignment.ptr);//swap might be useful if I was to implement this with STL vector
    swap(len,assignment.len);
    return *this;
}
//friend functions of num class

bool operator<(const num& op1, const num& op2)
{
    //if one is negative and other is positive, if both are positive, if both are negative
    if(op1.MSW()>=0 && op2.MSW()>=0)
    {
        unsigned_num OP(op2.ptr,op2.len);
        unsigned_num op(op1.ptr,op1.len);
        return op<OP;
    }
    else
    {
        if(op1.MSW()<0 && op2.MSW()>=0) return true;
        else if(op1.MSW()>=0 && op2.MSW()<0) return false;
        else
        {
            unsigned_num OP(op2.ptr,op2.len);
            unsigned_num op(op1.ptr,op1.len);
            return !(op<OP);
        }
    }

}
num operator-(const num& subtrahend)
{
    unsigned_num intermediate(subtrahend.ptr,subtrahend.len);
    num result(-intermediate);
    return result;
}
//unlike operator- of unsigned_num which returns the positive difference, this is a signed difference
num operator-(const num& minuend, const num& subtrahend)
{
    num result=-subtrahend;
    result=result+minuend;
    return result;
}

num operator+(const num& Augend, const num& Addend)
{
    const num& addend=Augend.len<Addend.len?Augend:Addend;
    const num& augend=Augend.len<Addend.len?Addend:Augend;
    if(!augend.is()) return num(addend);
    if(!addend.is()) return num(augend);
    bool operandlengths_equal=(augend.len==addend.len);
    digit sign_extension;//in case length is equal, sign_extension is assigned MSW of addend
    if(!operandlengths_equal)
    {
        sign_extension=static_cast<signed_digit>(addend[addend.len-1])<0 ? -1:0;
    }
    else sign_extension=addend[addend.len-1];

    num sum(augend.len+1);
    sum[0]=0;
    for(digit i=0;i<(operandlengths_equal?(addend.len-1):addend.len);i++)
        sum[i+1]=(bool)(__builtin_add_overflow(augend[i],sum[i],(sum.ptr+i))+__builtin_add_overflow(addend[i],sum[i],(sum.ptr+i)));//operator + is used instead of || as || is short circuiting while + is not. + has the same effect as || for boolean types
    for(digit i=addend.len;i<augend.len-1;i++)
        sum[i+1]=(bool)(__builtin_add_overflow(augend[i],sum[i],(sum.ptr+i))+__builtin_add_overflow(sign_extension,sum[i],(sum.ptr+i)));

    if(__builtin_add_overflow(static_cast<signed_digit>(augend[augend.len-1]),sum[augend.len-1],(signed_digit*)(sum.ptr+augend.len-1))+__builtin_add_overflow(static_cast<signed_digit>(sign_extension),static_cast<signed_digit>(sum[augend.len-1]),(signed_digit*)(sum.ptr+augend.len-1)))
        sum[augend.len]=static_cast<signed_digit>(augend[augend.len-1])<0?-1:0;
    else sum.reallocate(augend.len, LOW);
    return sum;
}
