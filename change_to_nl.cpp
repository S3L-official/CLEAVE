#include <bits/stdc++.h>
#include "parser.h"
using namespace std;

int main(int argc, char **argv) {
    cout << "ok" << endl;
    string str1(argv[1]); //library path
    string library_file = str1; //example "../cell/Library.txt";
    string str2(argv[2]); // library name
    string library_name = str2; //example "NANG45";
    string str3(argv[3]); // input path
    string input_file = str3; //example "../test/led64-1round/test1round1bit.v";
    string str4(argv[4]); // circuit name
    string circuit_modelu = str4; // example "LED64Enc"; 
    string str5(argv[5]); // output path
    string output_file = str5;
    // string output_file = "test/no_protection/test_unroll.nl"; //example "../test/led64-1round/test1round1bit.nl";
    int res = parse_and_convert_wrapper(library_file, library_name, input_file, circuit_modelu, output_file);
    return 0;
}
