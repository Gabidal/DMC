#include "DMC.h"

#include <iostream>
#include <vector>

using namespace std;

int main(){
    srand(time(NULL));

    Language Lang("Languages/English.txt");

    Lang.Concat_Raw_Buffer();
    Lang.Markov_Buffer();

    Teller teller(&Lang);
    teller.Init_Weight({
        {Weight(1), "Games"},
        {Weight(1), "Game"},
        {Weight(1), "games"},
        {Weight(1), "game"},
    });

    cout << teller.Generate_Thought() << endl;

    teller.Print_Weights("Weights.txt");

    string await;
    cin >> await;
}