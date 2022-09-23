#include "DMC.h"

#include <iostream>
#include <vector>

using namespace std;

int main(){
    srand(time(NULL));

    Language Lang("Languages/lcet10.txt");

    Lang.Concat_Raw_Buffer();
    Lang.Markov_Buffer();
    Teller teller(&Lang);
    teller.Init_Weight({
        {Weight(1), "Computer"},
        {Weight(1), "computer"},
        {Weight(1), "would"},
    });

    cout << teller.Generate_Thought() << endl;

    //teller.Print_Weights("Weights.txt");

    string await;
    cin >> await;
}