#include "Src/DMC.h"

#include <iostream>
#include <vector>

using namespace std;

// IDEAS:
/*
    For deterministic behaviour we need to start from the end and look up words that chain to the current word.
    This will return a context for the wanted word.

    The connection between the end and start word is not possible to connect without manipulating the markov chain matrix.
*/

int main(){
    srand(time(NULL));
    
    Language Lang("Languages/Chapter_3_Knight_At_Heart.txt");
    
    Teller t(&Lang);
    
    cout << t.Generate_Thought(123) << endl;

    string await;
    cin >> await;
}