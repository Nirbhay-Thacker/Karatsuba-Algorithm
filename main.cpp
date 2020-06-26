#include <iostream>
#include <limits>
#include <cstdint>
using namespace std;
typedef uint_fast32_t digit;
typedef int_fast32_t sdigit;
typedef uint_fast64_t large_digit;
typedef int_fast64_t slarge_digit;
enum paddingType{LEFT,RIGHT};
//2's complement
struct num{
    digit* ptr; //to MSB -2^n-1
    size_t len;//max value on the respective platforms can be checked by SIZE_MAX, which is defined after C++11 in cstdint
    explicit num(size_t length);
    num(const digit* PTR, const size_t LEN):ptr(PTR),len(LEN){};
    inline digit& operator[](const size_t i);
    void reallocate(num& number, size_t new_len);
    friend num operator+(const num augend, const num addend);
    friend void operator+=(num& augend, const num addend);
    friend num operator-(const num minuend, const num subtrahend);
    friend num operator*(const num multiplicand, const num multiplier);
    num operator<<(const num shifts);
    friend void operator=(num& assignee, const num assignment);
};
num m,M;
bool first;
size_t left_shift=0;
num::explicit num(const size_t length){
    len=length;
    ptr=static_cast<digit*>(malloc(sizeof(digit[len])));

    if(ptr==nullptr) {
        bad_alloc except;
        throw except;
    }
}
void num::reallocate(num& number, size_t new_len){
    //Can replace by new operator for type safety at the cost of reduced efficiency
    digit* new_ptr=(digit*)realloc(number.ptr,sizeof(digit[number.len+1]));
    bad_alloc except;
    if (new_ptr==nullptr) throw except;
    number.ptr=new_ptr;
    number.len++;
    number[number.len-1]=number[len-2]<0?-1:0;
}
digit& num::operator[](const size_t i){
    if(!(i<len)) throw invalid_argument("digit (array) index out of bounds");
    return *(ptr+i);
}
void operator=(num& assignee, const num assignment){
    assignee.len=assignment.len;
    assignee.ptr=assignment.ptr;
}
num operator+(const num augend, const num addend){
    //domain augend,addend {-2^(n-1),2^(n-1)-1}
    //range result{-2^n,2^n-2}
    num temp=augend.len<addend.len?augend:addend;
    augend=augend.len<addend.len?addend:augend;
    addend=temp;
    num result(augend.len+1);
    //there is asm code that adds with carry, but it I haven't learned it yet
    large_digit sum;
    result[0]=0;
    for(size_t i=left_shift;i<addend.len;i++){
        sum=augend[i]+addend[i];
        sum+=result[i];
        result[i+1]=sum>>32;
        result[i]=static_cast<digit>(sum);//This is safe because 0<(digit)sum<digit_MAX-2
    }
    digit add;
    add=addend[addend.len-1]<0?-1:0;
    if (!add)
    for(size_t i=addend.len;i<augend.len;i++){
        sum=augend[i]+add;
        sum+=result[i];
        result[i+1]=sum>>32;
        result[i]=static_cast<digit>(sum);
    }
    result[result.len-1]=result[result.len-1]?-1:0;
    }
    left_shift=0;
    return result;
}
/* Alternatively
        result[0]=0;
        for(int i=0;i<x;i++){
        result[i]+=augend[i]+addend[i];
        result[i+1]+=(result[i]<augend[i] || carry && (result=augend[i]) && (augend!=0);
        //This works for typedef digit uint_fast64_t
*/
/*Alternatively
    void add_with_carry(uint64_t* A, uint64_t* B, uint64_t* result, int x){
        for(int i=0;i<x;i++){
        result[i]=A[i]+B[i];
        if(carry && result[i]==numeric_limits<digit>::max()) { //Alternatively write ~result[i]==0;
            carry=true;
            result[i]=0;
        }
        else {
            result[i]+=carry;
            carry=__builtin_add_overflow(A[i],B[i],(result.ptr+i));
        }
    }
}
*/
void operator+=(num& augend,const num addend){
    augend=reallocate((augend.len<addend.len:addend:augend)+1);
    augend=augend+addend;//calls overloaded + operator
}
//takes 2's complement of subtrahend and then adds
num operator-(const num& minuend,const num& subtrahend){
    //domain minuend,subtrahend e {-2^(n-1),2^(n-1)-1}=n bits, assumes operands are in 2's complement notation
    //range result {-2^n,2^n-1}=n+1 bits
    //assign result as one digit larger than largest
    num result((minuend.len<subtrahend.len?subtrahend.len:minuend.len)+1);
    //appends a word before taking 2's complement if subtrahend is MNN
    bool MNN=true,zero=false;
    for(size_t i=0;i<subtrahend.len;i++)if(subtrahend[i]!=0) zero=false;
    if(!zero && (subtrahend[0]==numeric_limits<digit>::min())){
        for(size_t i=1;i<subtrahend.len;i++) if (subtrahend[i]!=0) MNN=false;
        if(MNN && (SIZE_MAX==sizeof(digit)*subtrahend.len)) throw invalid_argument("2's complement of subtrahend exceeds memory");
    }
    else MNN=false;
    //copy MSB to appended word, if MSW<0, MSB=1 else MSB=0. -1 is all bits 1
    if(MNN) result[result.len-1]=0;
    else{
        result[result.len-1]=subtrahend[subtrahend.len-1]<0?0:-1;//MSW=bit string of complement of MSB
    }
    size_t i=0;
    for(;i<subtrahend.len;i++) result[i]=~subtrahend[i];
    i=0;
    //propagates carry if digit is max
    while(result[i]==numeric_limits<digit>::max())result[i++]=0;
    result[i]++;
    result=result+minuend; //really not in the mood to overload += to support num
    return result;
}
inline num shift_and_add(num multiplicand, digit multiplier){
    num result(multiplicand.len+1);
    large_digit product;
    for(size_t i=0;i<multiplicand.len;i++){
        product=augend[i]*addend[i];
        result[i+1]=product>>32;
        result[i]+=static_cast<digit>(sum);
        result[i+1]+=__builtin_add_overflow(result[i],static_cast<digit>(sum),(result.ptr+i));//This is safe because digit_MAX*digit_MAX=doubledigit_MAX-2*digit_MAX
    }
    return result;
}
num operator*(num multiplicand,num multiplier){
    //The following step negates the need for appending zeros, unfortunately it has a memory cost, but it is acceptable than CPU time costs of appending zeros
    //Alternatives are to separate the operator function and the multiplication function
    if(first){
    m=multiplicand.len<multiplier.len?multiplicand:multiplier;//m is multiplier and is smaller than multiplicand
    M=multiplicand.len<multiplier.len?multiplier:multiplicand;//M is multiplicand and is larger than multiplier
    num::first=false;
    multiplicand
    }
    //base cases
    if(multiplier.len==1){
        return shift_and_add(multiplicand, multiplier[0]);
    }
    size_t q=multiplier.len/2;
    num m_l(m.ptr,q),m_h(m.ptr+q,m.len-q),M_l(M.ptr,q),M_h(M.ptr+q,M.len-q);
    num z0(2*q),z1(),z2(m.len+M.len-2*q),Z(m.len+M.len);
    z0=m_l*M_l;
    z2=m_h*M_h;
    z1=(m_l-m_h)*(M_h-M_l)+z0+z1;//z1=m_l*M_h+m_h*M_l=(m_l-m_h)*(M_h-M_l)+z0+z2 THE KARATSUBA STEP
    for(size_t i=0;i<Z.len;i++) Z.ptr[i]=0;
    Z+=z0;
    left_shift=q;
    Z+=z1;
    left_shift=2*q;
    Z+=z2;//left_shift is set to zero by + operator function, left shift operator can be overloaded to append zero, but this is faster and takes less space
}
int main()
{
    cout << "Hello world!" << endl;
    return 0;
}

