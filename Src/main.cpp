#include "DMC.h"

#include <iostream>

using namespace std;

int main(){

    Language Lang("Languages/English.txt");

    Lang.Concat_Raw_Buffer();

    Lang.Output("Languages/English_Out.txt");

    string await;
    cin >> await;
}