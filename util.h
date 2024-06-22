#include "string"
#include "vector"
#include "sstream"
using namespace std;
#ifndef FAULT_UTIL_H
#define FAULT_UTIL_H

static vector<string> split(const string line, char delimiter)
{
    // Variable declarations
    vector<string> tokens;
    string token;

    // Turn string into stream
    stringstream stream(line);

    // Extract tokens from line
    while(getline(stream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

static bool gen_comb_norep_lex_init(vector<int>& array, int n, int k)
{
    int j; //index

    if(k > n)
        return 0;
    if(k == 0)
        return 0;
    //initialize: vector[0, ..., k - 1] are 0, ..., k - 1
    for(j = 0; j < k; j++)
        array.push_back(j);

    return 1;
}

static bool gen_comb_norep_lex_next(vector<int>& array, int n, int k)
{
    int j; //index

    //easy case, increase rightmost element
    if(array[k - 1] < n - 1)
    {
        array[k - 1]++;
        return 1;
    }

    //find rightmost element to increase
    for(j = k - 2; j >= 0; j--)
        if(array[j] < n - k + j)
            break;

    //terminate if vector[0] == n - k
    if(j < 0)
        return 0;

    //increase
    array[j]++;

    //set right-hand elements
    while(j < k - 1)
    {
        array[j + 1] = array[j] + 1;
        j++;
    }

    return 1;
}



#endif //FAULT_UTIL_H
