#ifndef _DMC_H_
#define _DMC_H_

//Deterministic Markov Chain
//DMC

#include <vector>
#include <string>
#include <map>
#include <bits/stdc++.h>

using namespace std;

// A Language is a compilation of sentences specific to that language.
class Language{
public:
    string Language_Name = "";

    //This buffer contains the raw data that just got scooped from the given text file. 
    string Raw_Buffer = "";

    // The raw buffer of words.
    vector<class Word> Cut_Buffer;

    // The Markov chain buffer, but made in map for improved performance.
    map<string, class Word*> Fast_Markov;

    // Width and height dimensions. X^2
    int Width = 0;

    //Loads the file contenct to the cut buffer.
    // And applies the markov chain to it.
    Language(string File_Name);

    // This function cuts the buffer into words divided with whitespace.
    void Concat_Raw_Buffer();

    void Apply_Markov_To_Buffer();

    void Output(string File_Name);



    // Utils
    //-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
    // Given cut buffers word returns markov chain index.
    class Word* Find(string w, int Start);
    // Given cut buffers coordinates returns markov chain index.
    class Word* Find(int x, int y);

    class Word* Get_Left(class Word* w);
    class Word* Get_Right(class Word* w);
    class Word* Get_Up(class Word* w);
    class Word* Get_Down(class Word* w);

};

// A word contains the word id and the language id it references to.
// This enables main language speak with some words replased with some other language.
// This phenomenon sometimes occurs when a entity knows more than one language.
class Word{
public:
    string Data = "";

    int X = 0;
    int Y = 0;

    vector<Word*> Next_Chain;
    vector<Word*> Previus_Chain;

    // Count of all instances of this Word.
    int Instances = 0;

    vector<Word*> Get_Chain_In_Order(){
        sort(Next_Chain.begin(), Next_Chain.end(), [=](Word* a, Word* b){
            return a->Instances > b->Instances;
        });
    }

    Word(string Data) : Data(Data) {};

    Word(char Data) {
        this->Data = string(1, Data);
    }
};

// This could also be replaced by Vector2
class Transformation{
public:
    int Origin_X = 0;
    int Origin_Y = 0;

    int Target_X = 0;
    int Target_Y = 0;
};

class Weight{
public:
    float Intensity = 0; //-1 to 1

    Weight(){}

    Weight(float Intensity) : Intensity(Intensity) {};
};

class Vector2{
public:
    int X = 0;
    int Y = 0;

    Vector2(){}
    Vector2(int X, int Y) : X(X), Y(Y) {}

    std::size_t operator()() const {
        return X * INT8_MAX + Y;
    }
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
    
    // The gradient map containing the indicies for diffusion and *djikstra.
    vector<Vector2> Gradient_Map;

    Teller(Language* lang);


    // Gradient Utils
    //-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
    void Apply_Gradient_To_Markov();
    vector<Vector2> Get_Surrounding(Vector2 origin, int Distance_From_Center);
    void Print_Gradient();


    // Utils
    //-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
    void Init_Weight(vector<pair<Weight,string>> weights = {});
    vector<pair<int, int>> Get_Surrounding(int x, int y);
    // Makes a slow burning gradient around the points given.
    void Diffuse_Around_Point_Of_Interest(int x, int y, int parent_x, int parent_y);
    void Print_Weights(string file_name);    
    bool Djikstra(vector<Word*>& Result, Word* Current, Word* End);
};



#endif