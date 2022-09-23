#ifndef _DMC_H_
#define _DMC_H_

//Deterministic Markov Chain
//DMC

#include <vector>
#include <string>

using namespace std;

// A Language is a compilation of sentences specific to that language.
class Language{
public:
    string Language_Name = "";

    //This buffer contains the raw data that just got scooped from the given text file. 
    string Raw_Buffer = "";

    // A 2D plane of words.
    vector<class Word> Buffer;
    vector<class Word*> Sentence_Starters;
    vector<class Word*> Sentence_Enders;

    int Width = 0;

    //Loads the 
    Language(string File_Name);

    // This function cuts the buffer into words divided with whitespace.
    void Concat_Raw_Buffer();

    void Markov_Buffer();

    void Output(string File_Name);
};

// A word contains the word id and the language id it references to.
// This enables main language speak with some words replased with some other language.
// This phenomenon sometimes occurs when a entity knows more than one language.
class Word{
public:
    string Data = "";
    int X = 0;
    int Y = 0;

    vector<Word*> Chain;

    Word(string Data) : Data(Data) {};


};


class Weight{
public:
    float Intensity = 0; //-1 to 1

    Weight(){}

    Weight(float Intensity) : Intensity(Intensity) {};
};

// Teller is a software that brings Djikstra's algorithm with Markov chain algorithm.
// Combining these two algorithms we can achieve deterministic text generation.
class Teller{
public:
    // Entity tries to avoid certain words, that have negative weight related to it.
    // Entity tries to go towards certain words that have a positive weight attached to it.
    vector<Weight> Weights; 

    // This determines how vast the weight will influence
    // 1 = no change, x < 1 weight will influense less area around it.
    float Diffuse = .5f;

    //What language the entity speaks.
    Language* Speaks = nullptr;

    Teller(Language* lang);

    void Init_Weight(vector<pair<Weight,string>> weights = {});

    string Generate_Thought();

    vector<pair<int, int>> Get_Surrounding(int x, int y);

    void Diffuse_Around_Point_Of_Interest(int x, int y, int parent_x, int parent_y);

    void Print_Weights(string file_name);    

    bool Djikstra(vector<Word*>& Result, Word* Current, Word* End);
};



#endif