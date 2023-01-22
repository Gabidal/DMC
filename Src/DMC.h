#ifndef _DMC_H_
#define _DMC_H_

//Deterministic Markov Chain
//DMC

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

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
    unordered_map<string, class Word*> Fast_Markov;

    // Width and height dimensions. X^2
    int Width = 0;

    //Loads the file contenct to the cut buffer.
    // And applies the markov chain to it.
    Language(string File_Name);

    // This function cuts the buffer into words divided with whitespace.
    void Concat_Raw_Buffer();

    void Apply_Markov_To_Buffer();

    void Finalize_Instance_Countters();

    void Output(string File_Name);



    // Utils
    //-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
    // Given cut buffers word returns markov chain index.
    class Word* Find(string w, int Start);
    // Given cut buffers coordinates returns markov chain index.
    class Word* Find(int x, int y);

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

// A word contains the word id and the language id it references to.
// This enables main language speak with some words replaced with some other language.
// This phenomenon sometimes occurs when a entity knows more than one language.
class Word{
public:
    string Data = "";

    Vector2 Position;

    vector<pair<int, Word*>> Next_Chain;
    vector<pair<int, Word*>> Previus_Chain;

    int Instances = 0;
    float Importance = 1;   // 0 to 1
    int Complexity = 0;     // How many words usually takes to describe this word.

    Word(string Data) : Data(Data) {};

    pair<int, Word*>* Get_Next(string word){
        for (auto& iter : Next_Chain){
            if (iter.second->Data == word)
                return &iter;
        }

        return nullptr;
    }

    pair<int, Word*>* Get_Prev(string word){
        for (auto& iter : Previus_Chain){
            if (iter.second->Data == word)
                return &iter;
        }

        return nullptr;
    }

    Word(char Data) {
        this->Data = string(1, Data);
    }
};

enum class IDS{
    CENTRIC_GRADIENT,
    CUBICAL_DALMIAN_GRADIENT,
    SPHERICAL_DALMIAN_GRADIENT,
    CIRCULAR_DALMIAN_GRADIENT,
};

// This could also be replaced by Vector2
class Transformation{
public:
    Vector2 Origin; // From where
    Vector2 Target; // to

    Transformation(){}

    Transformation(Vector2 Origin, Vector2 Target) : Origin(Origin), Target(Target) {}
};

class Transforms{
public:
    // <ID, Transform>
    unordered_map<int, Transformation> Transforms;

    void Add_Transform(IDS ID, Transformation transform){
        Transforms[(int)ID] = transform;
    }

    Transformation* Get_Transform(IDS ID){
        return &Transforms[(int)ID];
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
    
    // List of all transforms performed into the singular index.
    vector<Transforms> Gradient_Map;

    Teller(Language* lang);


    // Gradient Utils
    //-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
    
    // Invokes all gradient transformations. 
    void Factory();

    // The most used words at the center and then less used words on the outer rims.
    void Centric_Gradient();
    // Most used words in their own corner, where the rest is filled by less relevant words. 
    // Used n'th Dimensional array.
    // Down sides:
    // Uses more memory.
    // Goods:
    // All points are at the same distance from each other.
    void Cubical_Dalmian_Gradient();
    // same as the above but uses low resolution pointed spherical space.
    // Down sides:
    // Not all points have same distance from each other.
    // Goods:
    // Uses less memory when using just circle or spherical array.
    void Spherical_Dalmian_Gradient();
    void Circular_Dalmian_Gradient();

    void Calculate_Importance_Scaling();
    // All words that have the Importance Scaler above 0.5 pass as keywords.
    vector<Word*> Get_Keywords();
    vector<Vector2> Get_Surrounding(Vector2 origin, int Distance_From_Center);


    // Utils
    //-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
    void Init_Weight(vector<pair<Weight,string>> weights = {});
    vector<pair<int, int>> Get_Surrounding(int x, int y);
    // Makes a slow burning gradient around the points given.
    void Diffuse_Around_Point_Of_Interest(int x, int y, int parent_x, int parent_y);
    void Print_Weights(string file_name);   

    // Circular tools 
    //-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
    vector<Vector2> Get_Circle_Perimeter_Indicies(int Radius);
    float Get_Radians_From_Circle_Perimeter(Vector2 perimeter_position, int Radius);
    float Get_Symmetrical_Spacing_On_Circle_Perimeter(int Point_Count);

    // 
    bool Djikstra(vector<Word*>& Result, Word* Current, Word* End);
};



#endif