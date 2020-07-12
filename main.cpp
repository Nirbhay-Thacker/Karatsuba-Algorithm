#include <iostream>
#include <string>
#include "num.hpp"
//This is an implementation of the Karatsuba multiplication algorithm.
//B=2^32, little endian system
using namespace std;
int main()
{

    unsigned_num number1(9,0xAAAAAAAA);
    unsigned_num number2(2,0xAB17AA97);
    cout<<0xAAAAAAAAAAAAAAAA<<" * "<<0xAB17AA97AB17AA97<<" = "<<0x720FC70F55555554E3458E46<<endl;
    unsigned_num number=multiply_shift_add(number1,number2);
    unsigned_num Karatsuba=number1*number2;
    for(digit i=0;i<number.length();i++) cout<<number[i]<<endl;
    cout<<endl;
    for(digit i=0;i<Karatsuba.length();i++) cout<<Karatsuba[i]<<endl;
    //for(digit i=0;i<C.length();i++) cout<<C[i]<<endl;

    /*
    num number1(2,0xAAAAAAAA);
    num number2(2);
    number2[0]=0x12345678;
    number2[1]=0x9ABCDEF;
    num number=number1+number2;
    for(digit i=0;i<number.length();i++) cout<<number[i]<<endl;
    */
    return 0;
}
