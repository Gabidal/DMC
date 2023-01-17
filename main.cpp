#include "Src/DMC.h"

#include <iostream>
#include <vector>

using namespace std;

int main(){
    srand(time(NULL));
    
    Language Lang("C:/Users/gagolzar/source/repos/DMC/Languages/text.txt");
    
    Teller t(&Lang);
    
    //cout << t.Generate_Thought(123) << endl;

    

    string await;
    cin >> await;
}