#include "DMC.h"

#include <fstream>
#include <iostream>
#include <math.h>

using namespace std;


Language::Language(string File_Name){
    ifstream File(File_Name);

    if (!File.is_open()){
        cout << "Error while opening file" << endl;
    }

    string Line;
    while(getline(File, Line)){
        Raw_Buffer += Line;
    }
    File.close();
}

void Language::Concat_Raw_Buffer(){
    string Current_Word = "";

    for (int i = 0; i < Raw_Buffer.size(); i++){
        if (Raw_Buffer[i] == ' '){
            Buffer.push_back( Word(Current_Word));
            Current_Word = "";
        }
        else{
            Current_Word += Raw_Buffer[i];
        }
    }
    if (Current_Word != ""){
        Buffer.push_back(Word(Current_Word));
    }

    Width = floor(sqrt(Buffer.size()));
}

// This function return 0-1f similiarity of two words. 
float Similiar(string a, string b){
    
    //chage both parameters into downcase
    for (int i = 0; i < a.size(); i++){
        a[i] = tolower(a[i]);
    }

    for (int i = 0; i < b.size(); i++){
        b[i] = tolower(b[i]);
    }

    float Matches = 0;

    for (int i = 0; i < a.size(); i++){
        if (a[i] == b[i]){
            Matches++;
        }
    }

    return Matches / a.size();
}

float Similiar(string a, char b){
    //chage parameter a into downcase
    for (int i = 0; i < a.size(); i++){
        a[i] = tolower(a[i]);
    }

    float Matches = 0;

    for (int i = 0; i < a.size(); i++){
        if (a[i] == b){
            Matches++;
        }
    }

    return Matches / a.size();
}

void Language::Markov_Buffer(){
    for (int i = 0; i < Buffer.size(); i++){
        if (i + 1 >= Buffer.size())
            break;

        Buffer[i].Chain.push_back(&Buffer[i + 1]);
    }

    //now also give the words to know their own location to help out the path finding algorithm later on.
    for (int y = 0; y < Width; y++){
        for (int x = 0; x < Width; x++){
            Buffer[y * Width + x].X = x;
            Buffer[y * Width + x].Y = y;
        }
    }

}

void Language::Output(string File_Name){
    ofstream File(File_Name);

    for (int y = 0; y < Width; y++){
        for (int x = 0; x < Width; x++){
            File << Buffer[y * Width + x].Data << " ";
        }
        File << endl;
    }

    File.close();
}

Teller::Teller(Language* lang){

    Speaks = lang;

}

vector<pair<int, int>> Teller::Get_Surrounding(int x, int y){

    int Width = Speaks->Width;

    vector<pair<int, int>> Surrounding;

    int Start_X = x-1;
    int End_X = x+1;

    int Start_Y = y-1;
    int End_Y = y+1;

    if (Start_X <= 0)
        Start_X = 0;

    if (End_X >= Width)
        End_X = Width;

    if (Start_Y <= 0)
        Start_Y = 0;

    if (End_Y >= Width)
        End_Y = Width;

    for (int y = Start_Y; y < End_Y; y++){
        for (int x = Start_X; x < End_X; x++){
            Surrounding.push_back({x, y});
        }
    }
}

void Teller::Init_Weight(vector<pair<Weight,string>> weights){
    if (Weights.size() == 0){
        Weights.resize(Speaks->Width);
    }

    vector<pair<int, int>> Points_Of_Interest;

    for (int y = 0; y < Speaks->Width; y++){
        for (int x = 0; x < Speaks->Width; x++){
            for (auto& w : weights){
                if (w.second == Speaks->Buffer[y * Speaks->Width + x].Data){
                    Weights[y * Speaks->Width + x].Intensity = w.first.Intensity;
                    Points_Of_Interest.push_back({x, y});
                }
            }

        }
    }

    for (auto& p : Points_Of_Interest){
        Diffuse_Around_Point_Of_Interest(p.first, p.second);
    }
}

float Threshold = 0.01;
//This function returns true if the decimal is close to the integer by the threshold
bool Around(float a, int b){
    return (a - b) < Threshold;
}

void Teller::Diffuse_Around_Point_Of_Interest(int x, int y){
    if (Around(Weights[y * Speaks->Width + x].Intensity, 0))
        return;

    for (auto& s : Get_Surrounding(x, y)){
        Weights[s.second * Speaks->Width + s.first].Intensity += Weights[y * Speaks->Width + x].Intensity / Diffuse;

        //Cascade
        Diffuse_Around_Point_Of_Interest(s.first, s.second);
    }

}

string Teller::Generate_Thought(){
    string Result = "";

    


}



