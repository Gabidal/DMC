#include "DMC.h"

#define _USE_MATH_DEFINES

#include <fstream>
#include <iostream>
#include <math.h>
#include <cmath>
#include <algorithm>
#include <vector>

using namespace std;


Language::Language(string File_Name){
    ifstream File(File_Name);

    // Set the language name as the file name
    // Also cut all the folders and file endings from the file name before assigning it to the name.
    Language_Name = File_Name.substr(File_Name.find_last_of("/\\") + 1);
    Language_Name = Language_Name.substr(0, Language_Name.find_last_of("."));


    if (!File.is_open()){
        cout << "Error while opening file" << endl;
    }

    string Line;
    while(getline(File, Line)){
        Raw_Buffer += Line + " ";
    }
    File.close();

    Concat_Raw_Buffer();

    Apply_Markov_To_Buffer();
}

Word* Language::Find(int x, int y){
    return &Cut_Buffer[x + y * Width];
}

void Language::Concat_Raw_Buffer(){
    string Current_Word = "";

    for (int i = 0; i < Raw_Buffer.size(); i++){
        if (
            Raw_Buffer[i] == ' ' || 
            Raw_Buffer[i] == ',' || 
            Raw_Buffer[i] == ':' || 
            Raw_Buffer[i] == '(' || 
            Raw_Buffer[i] == ')' ||
            Raw_Buffer[i] == '.' || 
            Raw_Buffer[i] == '!' || 
            Raw_Buffer[i] == '?' || 
            Raw_Buffer[i] == '"' || 
            Raw_Buffer[i] == '\'' || 
            Raw_Buffer[i] == '-' || 
            Raw_Buffer[i] == '+' || 
            Raw_Buffer[i] == '*' || 
            Raw_Buffer[i] == ';' || 
            Raw_Buffer[i] == '[' || 
            Raw_Buffer[i] == ']' || 
            Raw_Buffer[i] == '{' || 
            Raw_Buffer[i] == '}' || 
            Raw_Buffer[i] == '\t'
        ){
            if (Current_Word.size() > 0)
                Cut_Buffer.push_back(Word(Current_Word));

            if (Raw_Buffer[i] != ' ')
                Cut_Buffer.push_back(Word(Raw_Buffer[i]));

            Next_Word:;
            Current_Word = "";
        }
        else{
            Current_Word += Raw_Buffer[i];
        }
    }
    if (Current_Word != ""){
        Cut_Buffer.push_back(Word(Current_Word));
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

void Language::Apply_Markov_To_Buffer(){

    if (Cut_Buffer.size() == 0){
        return;
    }

    Width = floor(sqrt(Cut_Buffer.size()));

    // Apply indicies to the cut buffer, since it is te only liquid 2D map.
    for (int y = 0; y < Width; y++){
        for (int x = 0; x < Width; x++){
            Cut_Buffer[x + y * Width].Position = {x, y};
        }
    }

    //Markov_Buffer.push_back(&Cut_Buffer[0]);
    Fast_Markov[Cut_Buffer[0].Data] = &Cut_Buffer[0];

    // First group by one identifier.
    for (int i = 1; i < Cut_Buffer.size(); i++){

        Word* Current = nullptr;

        // If this word has already been defined
        Word* tmp = Fast_Markov[Cut_Buffer[i].Data];
        if (tmp){
            Current = tmp;
        }
        else{
            // If not then make a new one and point to it.
            //Markov_Buffer.push_back(new Word(Cut_Buffer[i]));
            Fast_Markov[Cut_Buffer[i].Data] = new Word(Cut_Buffer[i]);
            //Current = Markov_Buffer.back();
            Current = Fast_Markov[Cut_Buffer[i].Data];
            Current->Instances++;
        }

        Word* Previus = Fast_Markov[Cut_Buffer[i - 1].Data];

        if (Current->Data == Previus->Data){
            continue;
        }

        if (Previus->Get_Next(Current->Data)){
            Previus->Get_Next(Current->Data)->first++;
        }
        else{
            Previus->Next_Chain.push_back({0, Current});
        }

        if (Current->Get_Prev(Previus->Data)){
            Current->Get_Prev(Previus->Data)->first++;
        }
        else{
            Current->Previus_Chain.push_back({0, Previus});
        }

    }

    Finalize_Instance_Countters();
}

// Changes the countting to probabilistics.
void Language::Finalize_Instance_Countters(){
    for (auto& i : Fast_Markov){
        int sum = 0;
        for (auto& [count, next_word] : i.second->Next_Chain){
            sum += count;
        }
        for (auto& [count, next_word] : i.second->Next_Chain){
            count /= sum;
        }
        sum = 0;
        for (auto& [count, prev_word] : i.second->Previus_Chain){
            sum += count;
        }
        for (auto& [count, prev_word] : i.second->Previus_Chain){
            count /= sum;
        }
    }
}

void Teller::Factory(){

    Calculate_Importance_Scaling();

    Centric_Gradient();
    Cubical_Dalmian_Gradient();
    Spherical_Dalmian_Gradient();

}

void Teller::Centric_Gradient(){

    /*
        NOTE:
        All words are given a number of how many times they have occured in the Markov chain.
        The most called words are then put eatch to their own corner there are several approaches to thi method.
        First if to cluster all the most called words into the center and the gradiently put the less called words outwards from the center.
        Second method is to put all the main words into their own corners, there can be more main words than 4, so the buffer may need to be in n'th dimension for this to work.
        Altough using n-dimensions wont be called gradient anymore, it would be more like a volume tranformation.
    */

    // Using Center method.
    // With of the gradient map is same as the Width.
    Gradient_Map.resize(Speaks->Width * Speaks->Width);

    int Center_X = Speaks->Width / 2;
    int Center_Y = Speaks->Width / 2;

    // This is the max distance from the center.
    int Max_Distance = Speaks->Width / 2;

    vector<int> Ordered_Instances;

    // Order the Cut_Buffer, where the most instances having words are first.
    for (int i = 0; i < Speaks->Cut_Buffer.size(); i++){
        Ordered_Instances.push_back(i);
    }

    // Sort the ordered instances.
    sort(Ordered_Instances.begin(), Ordered_Instances.end(), [this](int a, int b){
        return Speaks->Cut_Buffer[a].Instances > Speaks->Cut_Buffer[b].Instances;
    });

    int Order_Index = 0;
    for (int i = 0; i < Max_Distance; i++){
        for (auto index : Get_Surrounding({0, 0}, i)){

            // If the index is out of bounds, then skip it.
            if (index.X > Max_Distance || index.Y > Max_Distance)
                continue;

            Transformation Current_Transform;
            Current_Transform.Origin = Speaks->Cut_Buffer[
                Ordered_Instances[Order_Index++]
            ].Position;

            Current_Transform.Target = index;

            // Save the transformation suggestions.
            Gradient_Map[index.Y * Speaks->Width + index.X].Add_Transform(
                IDS::CENTRIC_GRADIENT, 
                Current_Transform
            );
        }
    }
}

void Teller::Cubical_Dalmian_Gradient(){

    /*
        Generates a n'th dimensional array to hold all the words.
        all keywords have their own corner.
    */
    vector<Word*> Keywords = Get_Keywords();

}

void Teller::Spherical_Dalmian_Gradient(){



}

vector<Vector2> Teller::Get_Circle_Perimeter_Indicies(int Radius){
    vector<Vector2> Result;

    for (int x = -Radius; x < Radius; x++){
        
        // Now calculate the height of that current x position.
        int y = sqrt(Radius * Radius - x * x);

        Result.push_back({x, y});
        Result.push_back({x, -y});
    }

    return Result;
}

float Teller::Get_Radians_From_Circle_Perimeter(Vector2 perimeter_position, int Radius){

    // radian = arctan(y/x)
    // where y is the perimeter_position.Y and x is radius - x 

    return atan2(perimeter_position.Y, Radius - perimeter_position.X);
}

float Teller::Get_Symmetrical_Spacing_On_Circle_Perimeter(int Point_Count){

    // The spacing between the points on the circle perimeter is 2 * pi / Point_Count

    return 2 * M_PI / Point_Count;
}

void Teller::Circular_Dalmian_Gradient(){
    vector<Word*> Keywords = Get_Keywords();

    // We need to get the circle radius needed to house the square area in the circle.
    int Square_Area = Speaks->Width * Speaks->Width;  

    // Get the radius of the circle from the square area
    float Radius = sqrt(Square_Area / M_PI);

    // This is the distance between the points from each other.
    float Radian_Spacing = Get_Symmetrical_Spacing_On_Circle_Perimeter(Keywords.size());

    vector<Vector2> Perimeter_Points = Get_Circle_Perimeter_Indicies(Radius);

    float Previus_Radian = 0;   // This probably needs to be value of 'Radian_Spacing'
    int Current_Keyword_Index = 0;
    for (auto perimeter_point : Perimeter_Points){

        float Current_Radian = Get_Radians_From_Circle_Perimeter(perimeter_point, Radius);

        float Radian_Difference = Current_Radian - Previus_Radian;

        // If the radian difference is bigger than the radiant difference, then we can use this point.
        if (Radian_Difference > Radian_Spacing){

            // Save the transformation suggestions.
            Gradient_Map[perimeter_point.Y * Speaks->Width + perimeter_point.X].Add_Transform(
                IDS::CIRCULAR_DALMIAN_GRADIENT, 
                {
                    Keywords[Current_Keyword_Index]->Position,
                    perimeter_point
                }
            );

            Current_Keyword_Index++;

            // Reset the radian difference.
            Previus_Radian = Current_Radian;
        }
    }
}

void Teller::Calculate_Importance_Scaling(){
    // Calculate importance scaling for each word
    for (auto& i : Speaks->Fast_Markov){
        i.second->Importance = i.second->Complexity + i.second->Next_Chain.size() + i.second->Previus_Chain.size();

        i.second->Importance /= (float)Speaks->Cut_Buffer.size();
    }

    // Now we need to normalize the importance scaler.
    float Max = 0;

    for (auto& i : Speaks->Fast_Markov){
        if (i.second->Importance > Max)
            Max = i.second->Importance;
    }

    // Apply the normalization.
    for (auto& i : Speaks->Fast_Markov){
        i.second->Importance /= Max;
    }
}

vector<Word*> Teller::Get_Keywords(){
    // All words that have the Importance Scaler above 0.5 pass as keywords.
    vector<Word*> Keywords;

    for (auto& i : Speaks->Fast_Markov){
        if (i.second->Importance > 0.5){
            Keywords.push_back(i.second);
        }
    }
}

template<typename T>
bool Find(T obj, vector<T> list){
    for (auto i : list){
        if (obj() == i())
            return true;
    }

    return false;
}

// Retuns a growing rectangle of indicies surrounding the area that is calculated from the distance from the center.
// Distance from center when 0, returns 2x2 center indicies.
// Distance from center when 1, returns the indicies surrounding the 2x2 indicies, that being 12 indicies.
// Distance from center when 2, returns the indicies surrounding the 12 indicies, the being 19 indicies.
// Distance from center when 3, returns the indicies surrounding the 19 indicies, the being 28 indicies.
// Distance from center when 4, returns the indicies surrounding the 28 indicies, the being 39 indicies.
// The returning indicies grow by the factor of numbers non divisible by 2, those being 7, 9, 11, ...
// So the distance from the center by 6, returns indicies surrounding the 39 indicies, the being 52 indicies.
vector<Vector2> Teller::Get_Surrounding(Vector2 origin, int Distance_From_Center){
    vector<Vector2> indices;
    for (int d = 0; d <= Distance_From_Center; d++) {
        for (int i = origin.X - d; i <= origin.X + d; i++) {
            for (int j = origin.Y - d; j <= origin.Y + d; j++) {
                if (abs(i - origin.X) == d || abs(j - origin.Y) == d) {
                    if (Find(Vector2(i, j), indices))
                        continue;
                    
                    indices.push_back(Vector2(i, j));
                }
            }
        }
    }
    return indices;
}

void Language::Output(string File_Name){
    ofstream File(File_Name);

    // Prints the markov chains content with "name": {links, ...}
    for (auto w : Fast_Markov){
        File << w.first << ": {";
        for (auto c : w.second->Next_Chain){
            File << c.second->Data << ", ";
        }
        File << "}" << endl;
    }

    File.close();
}

Teller::Teller(Language* lang){

    Speaks = lang;

    Centric_Gradient();
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
                if (w.second == Speaks->Fast_Markov[Speaks->Cut_Buffer[y * Speaks->Width + x].Data]->Data){
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
    return max(rand(), 1) % Count;
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


