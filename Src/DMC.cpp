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

    // Now also get the sentence starters
    if (Buffer.size() > 0)
        Sentence_Starters.push_back(&Buffer[0]);
    
    for (int i = 1; i < Buffer.size(); i++){
        Word* Previus_Word = &Buffer[i - 1];

        if (Previus_Word->Data[Previus_Word->Data.size() - 1] == '.'){
            Sentence_Starters.push_back(&Buffer[i]);

            Sentence_Enders.push_back(Previus_Word);
        }
    }
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

// This function returns the left and right of the x and y point.
// This function will also keep in mind word wrapping.
vector<pair<int, int>> Teller::Get_Surrounding(int x, int y){

    int Width = Speaks->Width;

    vector<pair<int, int>> Surrounding;

    //get the left and right side of the current x, y
    //also remember word wrap
    pair<int, int> Left = { x - 1, y };
    pair<int, int> Right = { x + 1, y };

    if (Left.first < 0){
        Left.first = Width - 1;

        if (y - 1 >= 0)
            Left.second = y - 1;
    }

    if (Right.first >= Width){
        Right.first = 0;

        if (y + 1 < Width)
            Right.second = y + 1;
    }

    Surrounding.push_back(Left);
    Surrounding.push_back(Right);

    return Surrounding;
}

// We can abuse the writing context of any word to be next to another word that has something to do with the same context.
// Note, negative weights are good words and the Teller will cascade towards them.
// Thus positive wieghts are bad words that the Teller tries to avoid.
void Teller::Init_Weight(vector<pair<Weight,string>> weights){
    if (Weights.size() == 0){
        Weights.resize(Speaks->Width * Speaks->Width);
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
        Diffuse_Around_Point_Of_Interest(p.first, p.second, p.first, p.second);
    }
}

float Threshold = 0.01;
//This function returns true if the decimal is close to the integer by the threshold
bool Around(float a, int b){
    return (max(a, (float)b) - min(a, (float)b)) < Threshold;
}

void Teller::Diffuse_Around_Point_Of_Interest(int x, int y, int parent_x, int parent_y){
    if (Around(Weights[y * Speaks->Width + x].Intensity, 0))
        return;

    for (auto& s : Get_Surrounding(x, y)){
        if (s.first == parent_x && s.second == parent_y)
            continue;

        Weights[s.second * Speaks->Width + s.first].Intensity += Weights[y * Speaks->Width + x].Intensity * Diffuse;

        //Cascade
        Diffuse_Around_Point_Of_Interest(s.first, s.second, x, y);
    }

}

// This function returns a random number between 0 and the count
int Choose(int Count){
    return rand() % Count;
}

const int Chunk_Size = 100;     // 100 Words in a single chunk.
const float Word_Cost = 0.1f;   // How much a single word costs 
int Current_Chunk_index = 0;

bool Teller::Djikstra(vector<Word*>& Result, Word* Current, Word* End){
    // If the current chunk has been already toppped, then return. 
    if (Result.size() + 1 >= Chunk_Size){
        cout << Current_Chunk_index << endl;
        return false;
    }

    // Add this current word to the Result.
    Result.push_back(Current);

    // If this is the lottery win then return true.
    if (Current == End)
        return true;

    vector<pair<bool, vector<Word*>>> Chunks;

    // These two are added just to improve performance ;P
    int Reached_To_The_End_Count = 0;
    int End_Reached_Index = 0;

    for (auto& s_c : Current->Chain){
        vector<Word*> Chunk;

        bool state = Djikstra(Chunk, s_c, End);

        Reached_To_The_End_Count += state;
        End_Reached_Index = Chunks.size();

        Chunks.push_back({ state, Chunk });
    }

    // There was only one way to reach the end, return it.
    if (Reached_To_The_End_Count == 1){
        //Combine the chunk to the result
        Result.insert(Result.end(), Chunks[End_Reached_Index].second.begin(), Chunks[End_Reached_Index].second.end());

        return true;
    }

    // Now we sort if some chunks end at closer to the end than others
    else if (Reached_To_The_End_Count > 1){
        // There are multiple ways to reach the end.
        // So remove all those that haven't.
        for (int i = 0; i < Chunks.size(); i++){
            if (!Chunks[i].first){
                Chunks.erase(Chunks.begin() + i);
                i--;
            }
        }
    }    

    // Now we check the cost of every chunk
    vector<float> Costs;

    for (auto& chunk : Chunks){
        float Cost = 0;

        for (auto& word : chunk.second){
            Cost += Word_Cost + Weights[word->Y * Speaks->Width + word->X].Intensity;
        }

        Costs.push_back(Cost);
    }

    // now we sort the chunks by the costs vector.
    for (int i = 0; i < Chunks.size(); i++){
        for (int j = 0; j < Chunks.size(); j++){
            if (Costs[i] < Costs[j]){
                swap(Costs[i], Costs[j]);
                swap(Chunks[i], Chunks[j]);
            }
        }
    }

    // Now if there was multiple end reaching chunks add the best candidate to the result
    if (Reached_To_The_End_Count > 1){
        Result.insert(Result.end(), Chunks[0].second.begin(), Chunks[0].second.end());
        return true;
    }

    // We end up here if we haven't found a way to get to the end.
    // So we start another djikstra on the best candidate and hope for best.
    for (auto& c : Chunks){
        vector<Word*> tmp;

        if (Djikstra(tmp, c.second[0], End)){
            Result.insert(Result.end(), tmp.begin(), tmp.end());
            return true;
        }
    } 

    return false;
}

string Teller::Generate_Thought(){
    string Result = "";

    // First select a sentence starter.
    Word* Start_Word = Speaks->Sentence_Starters[Choose(Speaks->Sentence_Starters.size())];

    // Then select the goal word.
    Word* Goal_Word = Speaks->Sentence_Enders[Choose(Speaks->Sentence_Enders.size())];

    // First we are going to go through from the selected sentence starter,
    // and go through towards the goal word, if we cant reach the goal word whitin the chunk.
    // were going to choose the chunk that ends to the nearest to the goal word.

    vector<Word*> Path;
    Djikstra(Path, Start_Word, Goal_Word);

    // Now we have the path, we can translate the path into string.
    for (auto& w : Path){
        Result += w->Data + " ";
    }

    return Result;
}



void Teller::Print_Weights(string file_name){
    ofstream File(file_name);

    for (int y = 0; y < Speaks->Width; y++){
        for (int x = 0; x < Speaks->Width; x++){
            File << to_string(Weights[y * Speaks->Width + x].Intensity).substr(0, 3) << " ";
        }
        File << endl;
    }

    File.close();
}


