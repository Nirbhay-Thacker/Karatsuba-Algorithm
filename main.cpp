//Acknowledgements to Dhaval Bothra for introducing me to the Karatsuba Algorithm and to Jinay Shah for helping me debug, identifying sub_digits as the cause of errors in operator* and suggesting a fix.
#include"num.hpp"
#include <iostream>

using namespace std;

int main()
{
    unsigned_num a,b,c;
    cin>>a;
    cin>>b;
    c=multiply_shift_add(a,b);
    cout<<endl<<c;
    c=a*b;
    cout<<endl<<c;
    return 0;
}
