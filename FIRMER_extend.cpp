#include <bits/stdc++.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "z3++.h"
#include "set"
#include "vector"
#include "fstream"
#include "util.h"
#include "map"
using namespace std;
using namespace z3;

struct Node {
    set<int> children = set<int>();
    int parents[2] = {-1, -1};
    int degree = 0;
    int node_id;
    int stage = -1;
    int worklist = 0;
    string type;
    string gate_parser;
};

map<int, Node> graph;
set<int> REG;
set<int> LOG;
map<string, int> con2gate;

set<string> readBlack(string blacklist_path) {
    set<string> blacked;
    ifstream blackfile(blacklist_path);
    string templine = "";
    while (getline(blackfile, templine)) {
        vector<string> blackTokens;
        blackTokens = split(templine, ' ');
        if (blackTokens[0] != "#") {
            for (auto ele : blackTokens) {
                blacked.insert(ele);
            }
        }
    }
    blackfile.close();
    return blacked;
}

set<string> readOut(string benchpath) {
    set<string> outs;
    ifstream outfile(benchpath);
    string templine = "";
    while (getline(outfile, templine)) {
        vector<string> blackTokens;
        blackTokens = split(templine, ' ');
        if (blackTokens[0] == "out") {
            outs.insert(blackTokens[1]);
        }
    }
    outfile.close();
    return outs;
}

bool is_blacked(string name, const set<string> &blacked) {
    for (auto ele : blacked) {
        if (name.find(ele) != name.npos) {
            return true;
        }
    }
    return false;
}


set<int> static_analysis(set<string> blacked) {
    queue<Node> q;
    set<int> fault_list;
    int black_num = 0;
    for (auto &v : graph) {
        if (v.second.degree == 0) {
            q.push(v.second);
            // fault_list.insert(v.second.node_id);
        }
    }
    cout << q.size() << endl;
    int worklistnum = 1;
    while(!q.empty()) {
        Node tmp = q.front();
        q.pop();
        set<int> worknum;
        int flag = 0;
        if (tmp.type == "in") {

        }
        else {
            for (int child : tmp.children) {
                if (graph[child].type == "out" || graph[child].type == "reg") {
                    tmp.worklist = worklistnum++;
                    // if (is_blacked(tmp.gate_parser, blacked)) {
                    //     flag = 1;
                    //     break;
                    // }
                    // else {
                        fault_list.insert(tmp.node_id);
                        flag = 1;
                        break;
                    // }
                }
                else {
                    // if child is in blacklist meanwhile child need to inject fault, this node must inject fault
                    if (is_blacked(graph[child].gate_parser, blacked) && fault_list.count(graph[child].node_id) != 0) {
                        fault_list.insert(tmp.node_id);
                    }
                    if (worknum.size() == 0) {
                        worknum.insert(graph[child].worklist);//graph[child].worklist
                    }
                    else if (worknum.count(graph[child].worklist) == 0) {
                        tmp.worklist = worklistnum++;
                        fault_list.insert(tmp.node_id);
                        flag = 1;
                        break;
                    }
                }
            }
            if (flag == 0)
                tmp.worklist = *worknum.begin();
        }
        if (tmp.parents[0] != -1) {
            graph[tmp.parents[0]].degree -= 1;
            if (graph[tmp.parents[0]].degree == 0) {
                q.push(graph[tmp.parents[0]]);
            }
        }
        if (tmp.parents[1] != -1) {
            graph[tmp.parents[1]].degree -= 1;
            if (graph[tmp.parents[1]].degree == 0) {
                q.push(graph[tmp.parents[1]]);
            }
        }
    }
    for (int num : fault_list) {
        if (is_blacked(graph[num].gate_parser, blacked)) {
            fault_list.erase(num);
        }
    }
    cout << "worklist number" << endl;
    cout << worklistnum << endl;
    cout << "number of fault: " << fault_list.size() << endl;
    return fault_list;
}

void constuct_the_graph(string benchpath) {
    set<string> binaryOp = {"xnor", "xor", "and", "nand", "nor", "or"};
    set<string> unaryOp = {"reg", "not"};

    ifstream bench(benchpath);
    string templine;
    int index = 0;
    while (getline(bench, templine)) {
        // cout << "index: " << index << endl;
        vector<string> linelist = split(templine, ' ');
        graph[index].type = linelist[0];
        graph[index].node_id = index;
        if (graph[index].type == "reg") REG.insert(index);
        else LOG.insert(index);
        int is_binary = binaryOp.count(linelist[0]);
        if (is_binary != 0)
            graph[index].gate_parser = linelist[4];
        else
            graph[index].gate_parser = linelist[3];
        if (graph[index].type == "in") {
        } else if (graph[index].type == "out") {
            graph[stoi(linelist[1])].children.insert(index);
            graph[index].parents[0] = stoi(linelist[1]);
        } else {
            if (binaryOp.count(graph[index].type) != 0) {
                graph[stoi(linelist[1])].children.insert(index);
                graph[stoi(linelist[1])].degree += 1;
                graph[stoi(linelist[2])].children.insert(index);
                graph[stoi(linelist[2])].degree += 1;
                graph[index].parents[0] = stoi(linelist[1]);
                graph[index].parents[1] = stoi(linelist[2]);
            }
            else {
                graph[stoi(linelist[1])].children.insert(index);
                graph[stoi(linelist[1])].degree += 1;
                graph[index].parents[0] = stoi(linelist[1]);
            }
        }
        index += 1;
    }
}

int max_stage = 0;
void get_register_stage(int node, int reg) {
    if (graph[node].type == "reg") {
        if (graph[node].stage >= reg) return ;
        graph[node].stage = max(reg, graph[node].stage);
        reg += 1;
    }
    // if (graph[node].type == "reg") {
    //     if (graph[node].stage != -1) return ;
    //     graph[node].stage = max(reg, graph[node].stage);
    //     reg += 1;
    // }
    if (graph[node].type == "out") {
        max_stage = max(max_stage, reg);
        return;
    }
    for (auto child : graph[node].children) {
        get_register_stage(child, reg);
    }
}

void get_gate_stage(int node, int reg) {
    if (graph[node].type != "reg") {
        if (graph[node].stage != -1 && graph[node].stage <= reg) {
            return ;
        }
        else {
            graph[node].stage = reg;
            if (graph[node].type == "in") {
                return ;
            }
        }
    }
    else {
        reg = graph[node].stage;
    }
    int parent1 = graph[node].parents[0];
    int parent2 = graph[node].parents[1];
    if (graph[parent1].stage <= reg && graph[parent1].stage != -1 && graph[parent1].type != "reg") {
        
    }
    else {
        get_gate_stage(parent1, reg);
    }
    if (graph[parent2].stage <= reg && graph[parent2].stage != -1 && graph[parent2].type != "reg") {
        
    }
    else {
        get_gate_stage(parent2, reg);
    }
}

void readIn(string benchpath, expr_vector &inlist, context &c, map<string, int> &inlistDict) {
    ifstream bench(benchpath);
    string templine;
    int ind = 0;
    while(getline(bench, templine)) {
        vector<string> linelist = split(templine, ' ');
        if(linelist[0] == "in") {
            expr a = c.bool_const(("in" + linelist[1]).c_str());
            inlist.push_back(a);
            inlistDict[linelist[1]] = inlist.size() - 1;
        }
    }
}

void encoding_circuit(string benchpath, context &c, expr_vector &outlist, expr_vector &inlist, map<string, int> &inlistDict, string composition, expr_vector &reglist, string v_name = "i") {
    ifstream bench(benchpath);
    string templine;
    map<string, int> variableDict;
    map<string, int> gate_type;
    expr_vector variableDictV(c);
    set<string> binaryOp = {"xnor", "xor", "and", "nand", "nor", "or"};
    set<string> unaryOp = {"reg", "not"};
    int index = 0;
    while(getline(bench, templine)) {
        vector<string> linelist = split(templine, ' ');
        gate_type[linelist[0]] += 1;
        if(linelist[0] == "in") {
            if (inlist.size() == 0) {
                // cout << "no inlist" << endl;
                expr a = c.bool_const((v_name + linelist[1]).c_str());
                variableDictV.push_back(a);
                variableDict[linelist[1]] = variableDictV.size() - 1;
            }
            else {
                variableDictV.push_back(inlist[inlistDict[linelist[1]]]);
                variableDict[linelist[1]] = variableDictV.size() - 1;
            }
        } else if(linelist[0] == "out") {
            outlist.push_back(variableDictV[variableDict[linelist[1]]]);
        } else {
            expr a(c);
            int is_binary = binaryOp.count(linelist[0]);
            if(is_binary != 0) {

                auto left = variableDictV[variableDict[linelist[1]]];
                auto right = variableDictV[variableDict[linelist[2]]];

                if(linelist[0] == "xnor") {
                    a = xnor(left,right);
                } else if(linelist[0] == "xor") {
                    a = left ^ right;
                } else if(linelist[0] == "nor") {
                    a = nor(left, right);
                } else if(linelist[0] == "and") {
                    a = left && right;
                } else if(linelist[0] == "nand") {
                    a = nand(left, right);
                } else if(linelist[0] == "or") {
                    a = left | right;
                }
            } else {
                auto left = variableDictV[variableDict[linelist[1]]];

                if(linelist[0] == "not") {
                    a = !left;
                } else if(linelist[0] == "reg") {
                    a = left;
                    if (composition == "composition") {
                        reglist.push_back(variableDictV[variableDict[linelist[1]]]);
                    }
                }
            }
            variableDictV.push_back(a);
            variableDict[to_string(index)] = variableDictV.size() - 1;
        }
        index++;
    }
    cout << "number of gate for every type" << endl;
    for (auto &i : gate_type) {
        cout << i.first << " " << i.second << endl;
    }
    cout << "line 324" << endl;
    cout << endl;
    // cout << variableDictV.size() << endl;
}


void encoding_fault(string benchpath, set<string> blacked, set<string> outs, bool isOpt, context &c, solver &s, int nof, string fault_type, string countermeasures, int variate, 
        string outpath_cnf, expr_vector &inlist, map<string, int> &inlistDict, expr_vector &outlistP, vector <expr_vector> &conditions, int opt_meth, string composition, expr_vector &reglistP) {
    set<string> binaryOp = {"xnor", "xor", "and", "nand", "nor", "or"};
    set<string> unaryOp = {"reg", "not"};
    map<string, int> gate_type;
    ifstream bench(benchpath);
    string templine;
    set<int> fault_list;
    int black_num = 0;
    if (opt_meth == 2) {
        cout << "opt_meth is 2" << endl;
        fault_list = static_analysis(blacked);
    }
    map<string, int> variablePrimeDict;
    expr_vector variablePrimeDictV(c);
    expr_vector count_conditions(c);
    int index = 0;
    int combinedGate = 0;
    int seqGate = 0;
    int reg_num = 0;
    vector<string> faultLocations;
    cout << "REG size: " << REG.size() << endl;

    for (int i = 0; i <= max_stage; i++) {
        expr_vector tmp(c);
        conditions.push_back(tmp);
    }
    while(getline(bench, templine)) {
        vector<string> linelist = split(templine, ' ');
        gate_type[linelist[0]] += 1;
        if(linelist[0] == "in") {
            if (inlist.size() == 0) {
                expr a = c.bool_const(("i" + linelist[1]).c_str());

                variablePrimeDictV.push_back(a);
                variablePrimeDict[linelist[1]] = variablePrimeDictV.size() - 1;
            }
            else {
                variablePrimeDictV.push_back(inlist[inlistDict[linelist[1]]]);
                variablePrimeDict[linelist[1]] = variablePrimeDictV.size() - 1;
            }

        } else if(linelist[0] == "out") {
            outlistP.push_back(variablePrimeDictV[variablePrimeDict[linelist[1]]]);
        } else {
            expr aP(c);
            int is_binary = binaryOp.count(linelist[0]);
            if(is_binary != 0) {

                auto leftP = variablePrimeDictV[variablePrimeDict[linelist[1]]];
                auto rightP = variablePrimeDictV[variablePrimeDict[linelist[2]]];

                if(linelist[0] == "xnor") {
                    aP = xnor(leftP, rightP);
                } else if(linelist[0] == "xor") {
                    aP = leftP ^ rightP;
                } else if(linelist[0] == "nor") {
                    aP = nor(leftP, rightP);
                } else if(linelist[0] == "and") {
                    aP = leftP && rightP;
                } else if(linelist[0] == "nand") {
                    aP = nand(leftP, rightP);
                } else if(linelist[0] == "or") {
                    aP = leftP | rightP;
                }
            } else {
                auto leftP = variablePrimeDictV[variablePrimeDict[linelist[1]]];

                if(linelist[0] == "not") {
                    aP = !leftP;
                } else if(linelist[0] == "reg") {
                    aP = leftP;
                    if (composition == "composition") {
                        reglistP.push_back(variablePrimeDictV[variablePrimeDict[linelist[1]]]);
                    }
                }
            }
            if (is_blacked(graph[index].gate_parser, blacked)) {
                black_num ++;
            }
            // else if(REG.count(index) == 1) {
            //     reg_num ++;
            // }
            else {
                if(REG.count(index) == 1) {
                    reg_num ++;
                }
                assert(graph[index].children.size() != 0);
                int flag_inj = 0;
                if (opt_meth == 1) {
                    if (graph[index].children.size() != 1) flag_inj = 1;
                    for (int child : graph[index].children) {
                        if (graph[child].type == "out" || REG.count(child) == 1 || is_blacked(graph[child].gate_parser, blacked)) {
                            flag_inj = 1;
                            break;
                        }
                    }
                }
                else {
                    if (fault_list.count(index) != 0) {
                        flag_inj = 1;
                    }
                }
                if(isOpt && flag_inj == 0) {

                } else {
                    expr con = c.bool_const(("c" + to_string(index)).c_str());
                    // con2gate["c" + to_string(index)).c_str()] = index;
                    count_conditions.push_back(con);
                    // cout << graph[index].stage << endl;
                    conditions[graph[index].stage].push_back(con);
                    expr s_r = c.bool_const(("sr_" + to_string(index)).c_str());
                    if (fault_type == "bit-flip") {
                        aP = ite(con, !aP, aP);
                    }
                    else if (fault_type == "set") {
                        aP = ite(con, xnor(s_r, s_r), aP);
                    }
                    else if (fault_type == "reset") {
                        aP = ite(con, s_r ^ s_r, aP);
                    }
                    else if (fault_type == "set-reset") {
                        // cout << "sr" << endl;
                        expr selectbit1 = c.bool_const(("sb_" + to_string(index)).c_str());
                        expr tmp1 = ite(selectbit1, xnor(s_r,  s_r), s_r ^ s_r);
                        aP = ite(con, tmp1, aP);
                    }
                    else if (fault_type == "all") {
                        expr selectbit1 = c.bool_const(("sb1_" + to_string(index)).c_str());
                        expr selectbit2 = c.bool_const(("sb2_" + to_string(index)).c_str());
                        expr tmp2 = ite(selectbit2, xnor(s_r,  s_r), s_r ^ s_r);//(aP || true) && selectbit1 && !selectbit2;
                        expr tmp1 = ite(selectbit1, tmp2, !aP);
                        aP = ite(con, tmp1, aP);
                    }
                    faultLocations.push_back(to_string(index));
                }
            }
            variablePrimeDictV.push_back(aP);
            variablePrimeDict[to_string(index)] = variablePrimeDictV.size() - 1;
            if(linelist[0] == "reg")
                seqGate++;
            else
                combinedGate++;
        }
        index += 1;
    }
    // cout << "conditions" << endl;
    // cout << count_conditions.size() << endl;
    // for (int i = 0; i <= max_stage; i++) {
    //     cout << conditions[i].size() << endl;
    // }
    cout << "number of gate for every type" << endl;
    for (auto &i : gate_type) {
        cout << i.first << " " << i.second << endl;
    }
    cout << endl;
    cout << "blacklist gates number: " << black_num << endl;
    cout << "registers number: " << reg_num << endl;
    cout << "faultLocation: " << faultLocations.size() << endl << endl;
}


void encoding_fault_withoutstage(string benchpath, set<string> blacked, set<string> outs, bool isOpt, context &c, solver &s, int nof, string fault_type, string countermeasures, int variate, 
         string outpath_cnf, expr_vector &inlist, map<string, int> &inlistDict, expr_vector &outlistP, expr_vector &conditions, int opt_meth, int stage_ind, string composition, expr_vector &reglistP) {
    set<string> binaryOp = {"xnor", "xor", "and", "nand", "nor", "or"};
    set<string> unaryOp = {"reg", "not"};
    map<string, int> gate_type;
    ifstream bench(benchpath);
    string templine;
    set<int> fault_list;
    int black_num = 0;
    if (opt_meth == 2) {
        cout << "opt_meth is 2" << endl;
        fault_list = static_analysis(blacked);
    }
    cout << "in encoding fault without stage "<< stage_ind << endl << endl;
    map<string, int> variablePrimeDict;
    expr_vector variablePrimeDictV(c);
    expr_vector count_conditions(c);
    int index = 0;
    int combinedGate = 0;
    int seqGate = 0;
    vector<string> faultLocations;
    cout << "REG size: " << REG.size() << endl;

    while(getline(bench, templine)) {
        vector<string> linelist = split(templine, ' ');
        gate_type[linelist[0]] += 1;
        if(linelist[0] == "in") {
            if (inlist.size() == 0) {
                expr a = c.bool_const(("i" + linelist[1]).c_str());

                variablePrimeDictV.push_back(a);
                variablePrimeDict[linelist[1]] = variablePrimeDictV.size() - 1;
            }
            else {
                variablePrimeDictV.push_back(inlist[inlistDict[linelist[1]]]);
                variablePrimeDict[linelist[1]] = variablePrimeDictV.size() - 1;
            }

        } else if(linelist[0] == "out") {
            outlistP.push_back(variablePrimeDictV[variablePrimeDict[linelist[1]]]);
        } else {
            expr aP(c);
            int is_binary = binaryOp.count(linelist[0]);
            if(is_binary != 0) {

                auto leftP = variablePrimeDictV[variablePrimeDict[linelist[1]]];
                auto rightP = variablePrimeDictV[variablePrimeDict[linelist[2]]];

                if(linelist[0] == "xnor") {
                    aP = xnor(leftP, rightP);
                } else if(linelist[0] == "xor") {
                    aP = leftP ^ rightP;
                } else if(linelist[0] == "nor") {
                    aP = nor(leftP, rightP);
                } else if(linelist[0] == "and") {
                    aP = leftP && rightP;
                } else if(linelist[0] == "nand") {
                    aP = nand(leftP, rightP);
                } else if(linelist[0] == "or") {
                    aP = leftP | rightP;
                }
            } else {
                auto leftP = variablePrimeDictV[variablePrimeDict[linelist[1]]];

                if(linelist[0] == "not") {
                    aP = !leftP;
                } else if(linelist[0] == "reg") {
                    aP = leftP;
                    if (composition == "composition") {
                        reglistP.push_back(variablePrimeDictV[variablePrimeDict[linelist[1]]]);
                    }
                }
            }
            if (stage_ind == graph[index].stage) {
            if (is_blacked(graph[index].gate_parser, blacked)) {
                black_num ++;
            }
            else {
                
                assert(graph[index].children.size() != 0);
                int flag_inj = 0;
                if (opt_meth == 1) {
                    if (graph[index].children.size() != 1) flag_inj = 1;
                    for (int child : graph[index].children) {
                        if (graph[child].type == "out" || REG.count(child) == 1 || is_blacked(graph[child].gate_parser, blacked)) {
                            flag_inj = 1;
                            break;
                        }
                    }
                }
                else {
                    if (fault_list.count(index) != 0) {
                        flag_inj = 1;
                    }
                }
                if(isOpt && flag_inj == 0) {

                } else {
                    expr con = c.bool_const(("c" + to_string(index)).c_str());
                    count_conditions.push_back(con);
                    conditions.push_back(con);
                    expr s_r = c.bool_const(("sr_" + to_string(index)).c_str());
                    if (fault_type == "bit-flip") {
                        aP = ite(con, !aP, aP);
                    }
                    else if (fault_type == "set") {
                        aP = ite(con, xnor(s_r, s_r), aP);
                    }
                    else if (fault_type == "reset") {
                        aP = ite(con, s_r ^ s_r, aP);
                    }
                    else if (fault_type == "set-reset") {
                        // cout << "sr" << endl;
                        expr selectbit1 = c.bool_const(("sb_" + to_string(index)).c_str());
                        expr tmp1 = ite(selectbit1, xnor(s_r,  s_r), s_r ^ s_r);
                        aP = ite(con, tmp1, aP);
                    }
                    else if (fault_type == "all") {
                        expr selectbit1 = c.bool_const(("sb1_" + to_string(index)).c_str());
                        expr selectbit2 = c.bool_const(("sb2_" + to_string(index)).c_str());
                        expr tmp2 = ite(selectbit2, xnor(s_r,  s_r), s_r ^ s_r);//(aP || true) && selectbit1 && !selectbit2;
                        expr tmp1 = ite(selectbit1, tmp2, !aP);
                        aP = ite(con, tmp1, aP);
                    }
                    faultLocations.push_back(to_string(index));
                }
            }
            }
            variablePrimeDictV.push_back(aP);
            variablePrimeDict[to_string(index)] = variablePrimeDictV.size() - 1;
            if(linelist[0] == "reg")
                seqGate++;
            else
                combinedGate++;
        }
        index += 1;
    }
    // cout << "conditions" << endl;
    // cout << count_conditions.size() << endl;
    // cout << conditions.size() << endl;
    cout << "number of gate for every type" << endl;
    for (auto &i : gate_type) {
        cout << i.first << " " << i.second << endl;
    }
    cout << endl;
    cout << "blacklist gates number: " << black_num << endl;
    cout << "faultLocation: " << faultLocations.size() << endl << endl;
}

void verify_correctness(string pure_circuit, string circuit_with_countermeasure, context &c, solver &s, string outpath_cnf, string countermeasures, string solver_select, string composition) {
    // cout << "test 1" << endl;
    expr_vector outlist1(c);
    expr_vector outlist2(c);
    expr_vector reglist1(c);
    expr_vector reglist2(c);
    expr_vector inlist(c);
    map<string, int> inlistDict;
    // cout << "test 2" << endl;
    readIn(pure_circuit, inlist, c, inlistDict);
    // cout << "test 4" << endl;
    // cout << "inlist: " << inlist.size() << endl;

    encoding_circuit(pure_circuit, c, outlist1, inlist, inlistDict, composition, reglist1);
    encoding_circuit(circuit_with_countermeasure, c, outlist2, inlist, inlistDict, composition, reglist2);
    // cout << outlist1.size() << endl;
    // cout << outlist2.size() << endl;
    
    expr test = outlist1[0] ^ outlist2[0];
    for (int i = 1; i < max(outlist1.size(), outlist2.size()) - 1; i++) {
        test = test | (outlist1[i] ^ outlist2[i]);
    }
    if (composition == "composition") {
        cout << "composition: reg1: " << reglist1.size() << " reg2: " << reglist2.size() << endl;
        for (int i = 0; i < max(reglist1.size(), reglist2.size()); i++) {
            test = test | (reglist1[i] ^ reglist2[i]);
        }
    }
    if (countermeasures == "detection") {
    
    }
    else {
        cout << "correction" << endl;
        test = test |  (outlist1[outlist1.size() - 1]^ outlist2[outlist2.size() - 1]);
    }
    cout << "test output number: " << max(reglist1.size(), reglist2.size()) + max(outlist1.size(), outlist2.size()) << endl;
    // cout << test << endl;
    if (solver_select == "SMT") {
        ofstream outfile;
        cout << "finished read file and static analysis with smt2" << endl;
        s.add(test);
        outpath_cnf = outpath_cnf + ".smt2";
        cout << "Output path: " << outpath_cnf << endl;
        outfile.open(outpath_cnf);
        outfile << s.to_smt2() << endl;
    }
    else if (solver_select == "SAT") {
        cout << "finished read file and static analysis with cnf" << endl;
        ofstream outfile;
        tactic t = tactic(c, "card2bv") & tactic(c, "tseitin-cnf");
        goal g(c);
        g.add(test);
        apply_result r = t(g);
        outpath_cnf = outpath_cnf + ".cnf";
        cout << "Output path: " << outpath_cnf << endl;
        outfile.open(outpath_cnf);
        outfile << r[0].dimacs() << endl;
        outfile.close();
    }
}


void verify_consistency(string circuit_with_countermeasure, context &c, solver &s, string outpath, string solver_select, string composition) {
    expr_vector outlist1(c);
    expr_vector outlist2(c);
    expr_vector reglist1(c);
    expr_vector reglist2(c);
    expr_vector inlist(c);
    map<string, int> inlistDict;
    // cout << inlist.size() << endl;
    encoding_circuit(circuit_with_countermeasure, c, outlist1, inlist, inlistDict, composition, reglist1, "c1");
    encoding_circuit(circuit_with_countermeasure, c, outlist2, inlist, inlistDict, composition, reglist2, "c2");
    // cout << outlist1.size() << endl;
    // cout << outlist2.size() << endl;

    // smt2
    if (solver_select == "SMT") {
        ofstream outfile;
        cout << "finished read file and static analysis with smt2" << endl;
        s.add(outlist1[outlist1.size() - 1] ^ outlist2[outlist2.size() - 1]);
        outpath = outpath + ".smt2";
        cout << "Output path: " << outpath << endl;
        outfile.open(outpath);
        outfile << s.to_smt2() << endl;
    }
    else if(solver_select == "SAT") {
    // cnf
        cout << "finished read file and static analysis with cnf" << endl;
        tactic t = tactic(c, "card2bv") & tactic(c, "tseitin-cnf");
        goal g(c);
        g.add(outlist1[outlist1.size() - 1] ^ outlist2[outlist2.size() - 1]);
        apply_result r = t(g);
        ofstream outfile;
        outpath = outpath + ".cnf";
        cout << "Output path: " << outpath << endl;
        outfile.open(outpath);
        outfile << r[0].dimacs() << endl;
        outfile.close();
    }
}

void verify_inject(string benchpath1, string benchpath2, set<string> blacked, set<string> outs, bool isOpt, context &c, solver &s, int nof, int nof_sm,
            string fault_type, string countermeasures, int variate, string outpath_cnf, int opt_meth, bool encode_stage, string solver_select, string composition) {
    set<string> binaryOp = {"xnor", "xor", "and", "nand", "nor", "or"};
    set<string> unaryOp = {"reg", "not"};
    expr_vector outlist1(c);
    expr_vector outlist2(c);
    expr_vector outlist3(c);
    expr_vector reglist1(c);
    expr_vector reglist2(c);
    expr_vector reglist3(c);
    expr_vector inlist(c);
    map<string, int> inlistDict;
    vector <expr_vector> conditions;
    // cout << "read in" << endl;
    readIn(benchpath1, inlist, c, inlistDict);
    // cout << "encoding circuit" << endl;
    cout << "Encode golden circuit..." << endl;
    encoding_circuit(benchpath1, c, outlist1, inlist, inlistDict, composition, reglist1);
    cout << "Encode faulty circuit..." << endl;
    if (encode_stage == false) {
        for (int stage_ind = 0; stage_ind <= max_stage; stage_ind++) {
            expr_vector condition(c);
            // cout << "encoding fault" << endl;
            encoding_fault_withoutstage(benchpath2, blacked, outs, isOpt, c, s, nof, fault_type, countermeasures, variate, outpath_cnf, inlist, inlistDict, outlist3, condition, opt_meth, stage_ind, composition, reglist3);

            set_param("sat.cardinality.encoding", "grouped");
            expr test = outlist1[0] ^ outlist3[0];
            for (int i = 1; i < outlist3.size() - 1; i++) {
                test = test | (outlist1[i] ^ outlist3[i]);
            }
            if (composition == "composition") {
                cout << "composition: reg1: " << reglist1.size() << " reg2: " << reglist3.size() << endl;
                for (int i = 0; i < max(reglist3.size(), reglist1.size()); i++) {
                    test = test | (reglist1[i] ^ reglist3[i]);
                }
            }
            // cout << "stop after test" << endl;dd
            if (countermeasures == "detection") {
                test = test && (!outlist3[outlist3.size() - 1]);
            }
            else {
                // cout << "correction" << endl;
                test = test |  (outlist1[outlist1.size() - 1]^ outlist3[outlist3.size() - 1]);
            }
            cout << "test output number: " << max(reglist1.size(), reglist3.size()) + max(outlist1.size(), outlist3.size()) << endl;
            if (solver_select == "SAT") {
                tactic t = tactic(c, "card2bv") & tactic(c, "tseitin-cnf");
                goal g(c);
                g.add(test);
                
                int coeffs[condition.size()];
                fill(coeffs, coeffs + condition.size(), 1);
                g.add(pble(condition, coeffs, nof));
                apply_result r = t(g);
                cout << "finished read file and static analysis with cnf" << endl;
                ofstream outfile;
                string tmp_outpath_cnf = outpath_cnf + "-" + to_string(stage_ind) + ".cnf";
                cout << "Output path: " << tmp_outpath_cnf << endl;
                outfile.open(tmp_outpath_cnf);
                outfile << r[0].dimacs() << endl;
                outfile.close();
            }
            else if(solver_select == "SMT") {
                expr bv0 = c.bv_val(0, 16);
                expr bv1 = c.bv_val(1, 16);
                expr num = c.bv_val(0, 16);
                for (auto i :  condition) {
                    num = num + ite(i, bv1, bv0);
                }
                s.add(test);
                s.add(num == nof);
                cout << "finished add conditions" << endl;

                cout << "finished read file and static analysis with smt2" << endl;
                ofstream outfile;
                outpath_cnf = outpath_cnf + to_string(stage_ind) + ".smt2";
                cout << "Output path: " << outpath_cnf << endl;
                outfile.open(outpath_cnf);
                outfile << "(set-logic QF_BV)" << endl;
                outfile << s.to_smt2() << endl;
            }
        }
    }
    else {
        encoding_fault(benchpath2, blacked, outs, isOpt, c, s, nof, fault_type, countermeasures, variate, outpath_cnf, inlist, inlistDict, outlist2, conditions, opt_meth, composition, reglist2);
        
        set_param("sat.cardinality.encoding", "grouped");
        expr test = outlist1[0] ^ outlist2[0];
        for (int i = 1; i < outlist2.size() - 1; i++) {
            test = test | (outlist1[i] ^ outlist2[i]);
        }
        if (composition == "composition") {
            cout << "composition: reg1: " << reglist1.size() << " reg2: " << reglist2.size() << endl;
            for (int i = 0; i < max(reglist1.size(), reglist2.size()); i++) {
                test = test | (reglist1[i] ^ reglist2[i]);
            }
        }
        if (countermeasures == "detection")
            test = test && (!outlist2[outlist2.size() - 1]);
        else {
            cout << "correction" << endl;
            test = test |  (outlist1[outlist1.size() - 1]^ outlist2[outlist2.size() - 1]);
        }
        cout << "test output number: " << max(reglist1.size(), reglist2.size()) + max(outlist1.size(), outlist2.size()) << endl;
        if (solver_select == "SAT") {
            tactic t = tactic(c, "card2bv") & tactic(c, "tseitin-cnf");
            goal g(c);
            g.add(test);
            expr_vector stage_con(c);
            // cout << "line 847" << endl;
            int real_stage = 0;
            for (int stage_ind = 0; stage_ind <= max_stage; stage_ind++) {
                if (conditions[stage_ind].size() == 0) continue;
                real_stage++;
                int coeffs[conditions[stage_ind].size()];
                fill(coeffs, coeffs + conditions[stage_ind].size(), 1);
                if (max_stage == 0) {
                    expr s0(c);
                    if (nof_sm == 1) {
                        cout << "nof_sm is 1" << endl;
                        s0 = pble(conditions[stage_ind], coeffs, nof);
                    }
                    else {
                        cout << "nof_sm is n" << endl;
                        s0 = pbeq(conditions[stage_ind], coeffs, nof);
                    }
                    g.add(s0);
                }
                if (max_stage > 0) {
                    expr s0(c);
                    expr tmp = c.bool_const(("s" + to_string(stage_ind)).c_str());
                    stage_con.push_back(tmp);
                    // cout << "line 862" << endl;
                    if (conditions[stage_ind].size() > 0) {
                        if (nof_sm == 1) {
                            cout << "nof_sm is 1" << endl;
                            s0 = ite(tmp, pble(conditions[stage_ind], coeffs, nof), pble(conditions[stage_ind], coeffs, 0));
                        }
                        else {
                            cout << "nof_sm is n" << endl;
                            s0 = ite(tmp, pbeq(conditions[stage_ind], coeffs, nof), pble(conditions[stage_ind], coeffs, 0));
                        }
                    }
                    // cout << "line 864" << endl;
                    g.add(s0);
                }
            }
            // cout << "line 868" << endl;
            if (max_stage > 0) {
                int stage_coeffs[real_stage + 1];
                fill(stage_coeffs, stage_coeffs + real_stage + 1, 1);
                g.add(pbeq(stage_con, stage_coeffs, variate));
            }

            apply_result r = t(g);

            cout << "finished read file and static analysis with cnf" << endl;
            ofstream outfile;
            outpath_cnf = outpath_cnf + ".cnf";
            cout << "Output path: " << outpath_cnf << endl;
            outfile.open(outpath_cnf);
            outfile << r[0].dimacs() << endl;
            outfile.close();
        }
        else {
            s.add(test);
            expr stage_num = c.bv_val(0, 16);
            for (int i = 0; i <= max_stage; i++) {
                // cout << "stage " << i << endl;
                expr bv0 = c.bv_val(0, 16);
                expr bv1 = c.bv_val(1, 16);
                expr num = c.bv_val(0, 16);
                // cout << conditions[i].size() << endl;
                for (auto j : conditions[i]) {
                    num = num + ite(j, bv1, bv0);
                }
                expr con = c.bool_const(("sc_" + to_string(i)).c_str());
                if (nof_sm == 1) {
                    cout << "nof_sm is 1" << endl;
                    s.add(ite(con, num <= nof, num == 0));
                }
                else {
                    cout << "nof_sm is n" << endl;
                    s.add(ite(con, num == nof, num == 0));
                }
                stage_num = stage_num + ite(con, bv1, bv0);
            }
            s.add(stage_num == variate);
            cout << "finished read file and static analysis with smt2" << endl;
            ofstream outfile;

            outpath_cnf = outpath_cnf + ".smt2";
            cout << "Output path: " << outpath_cnf << endl;
            outfile.open(outpath_cnf);
            outfile << "(set-logic QF_BV)" << endl;
            outfile << s.to_smt2() << endl;
        }
    }
}

int main(int argc, char **argv) {
    cout << "Runing FIRMER..." << endl;
    clock_t start = clock();
    context c;
    solver s(c);
    int task = stoi(argv[1]);
    if (task == 1) {
        cout << "Task 1" << endl << endl;
        string circuit = argv[2];
        string circuit_countermeasures = argv[3];
        string outpath = argv[4];
        string countermeasure = argv[5];
        string solver_select = argv[6];
        string composition = argv[7];
        verify_correctness(circuit, circuit_countermeasures, c, s, outpath, countermeasure, solver_select, composition);
        clock_t end_final = clock();
        cout << "Encode time: " << (end_final - start) / (double)CLOCKS_PER_SEC << endl;
    }
    if (task == 2) {
        cout << "Task 2" << endl << endl;
        string benchpath = argv[2];
        string outpath = argv[3];
        string countermeasure = argv[4];
        string solver_select = argv[5];
        string composition = argv[6];
        cout << "Note that the default flag bit is the last output bit." << endl;
        // cout << benchpath << endl;
        if (countermeasure == "detection") {
            verify_consistency(benchpath, c, s, outpath, solver_select, composition);
        }
        else {
            cout << "countermeasure not supported" << endl;
        }
        clock_t end_final = clock();
        cout << "Encode time: " << (end_final - start) / (double)CLOCKS_PER_SEC << endl;
    }
    if (task == 3) {
        cout << "Task 3" << endl << endl;
        string str1(argv[2]);
        string benchpath = str1;
        string str2(argv[3]);
        string blacklist_path = str2;
        string str4(argv[4]);
        string outpath_cnf = str4;
        int nof = stoi(argv[5]); // number of faults
        int nof_sm = stoi(argv[6]);
        // cout << nof << endl;
        int variate = stoi(argv[7]); // number of stages
        string countermeasures(argv[8]);
        string fault_type(argv[9]);
        string str7(argv[10]);
        bool isOpt;
        if (str7 == "opt") 
            isOpt = true;
        else 
            isOpt = false;
        
        int opt_meth = stoi(argv[11]);
        bool encode_stage;
        string str12(argv[12]);
        // cout << argv[11] << endl;
        if (str12 == "true") {
            encode_stage = true;
        }
        else {
            if (variate != 1) {
                cout << "variate must be 1" << endl;
                return 0;
            }
            encode_stage = false;
        }
        string solver_select = argv[13];
        string composition = argv[14];
        cout << composition << endl;
        set<string> blacked = readBlack(blacklist_path);
        cout << "black list" << endl;
        for (auto it = blacked.begin(); it != blacked.end(); it++) {
            cout << *it << " ";
        }
        cout << endl << endl;
        set<string> outs = readOut(benchpath);
        // cout << "line 981" << endl;
        constuct_the_graph(benchpath);

        clock_t end1 = clock();
        cout << (end1 - start) / (double)CLOCKS_PER_SEC << " seconds for counstuct the graph" << endl;
        for (auto node : graph) {
            if (node.second.type == "in") {
                int reg = 0;
                get_register_stage(node.second.node_id, reg);
            }
        }
        clock_t end2 = clock();
        cout << (end2 - start) / (double)CLOCKS_PER_SEC << " seconds for get register stage" << endl;

        for (auto node : graph) {
            if (node.second.type == "out") {
                int reg = max_stage;
                get_gate_stage(node.second.node_id, reg);
            }
        }
        // cout << "split stage" << endl;
        clock_t end3 = clock();
        cout << (end3 - start) / (double)CLOCKS_PER_SEC << " seconds for get gate stage" << endl;
        cout << "max clock cycles: " << max_stage + 1 << endl << endl;

        verify_inject(benchpath, benchpath, blacked, outs, isOpt, c, s, nof, nof_sm, fault_type, countermeasures, variate, outpath_cnf, opt_meth, encode_stage, solver_select, composition);

        clock_t end_final = clock();
        cout << "Encode time: " << (end_final - start) / (double)CLOCKS_PER_SEC << endl;

    }
    if (task == 4) {
        cout << "Task 4" << endl << endl;
        string str1(argv[2]);
        string benchpath_pure = str1;
        // cout << benchpath_pure << endl;
        string str2(argv[3]);
        string benchpath = str2;
        // cout << benchpath << endl;
        string str3(argv[4]);
        string blacklist_path = str3;
        string str4(argv[5]);
        string outpath_cnf = str4;
        int nof = stoi(argv[6]); // number of faults
        int nof_sm = stoi(argv[7]);
        // cout << nof << endl;
        int variate = stoi(argv[8]); // number of stages
        // cout << argv[7] << endl;
        string countermeasures(argv[9]);
        string fault_type(argv[10]);
        string str7(argv[11]);
        bool isOpt;
        if (str7 == "opt") 
            isOpt = true;
        else 
            isOpt = false;
        int opt_meth = stoi(argv[12]);
        bool encode_stage;
        string str12(argv[13]);
        // cout << argv[12] << endl;
        string solver_select = argv[14];
        string composition = argv[15];
        if (str12 == "true") {
            encode_stage = true;
        }
        else {
            if (variate != 1) {
                cout << "variate must be 1" << endl;
                return 0;
            }
            encode_stage = false;
        }
        set<string> blacked = readBlack(blacklist_path);
        cout << "black list" << endl;
        for (auto it = blacked.begin(); it != blacked.end(); it++) {
            cout << *it << " ";
        }
        cout << endl << endl;
        set<string> outs = readOut(benchpath);


        constuct_the_graph(benchpath);

        clock_t end1 = clock();
        // cout << (end1 - start) / (double)CLOCKS_PER_SEC << " seconds for counstuct the graph" << endl;
        for (auto node : graph) {
            if (node.second.type == "in") {
                int reg = 0;
                get_register_stage(node.second.node_id, reg);
            }
        }
        clock_t end2 = clock();
        // cout << (end2 - start) / (double)CLOCKS_PER_SEC << " seconds for get register stage" << endl;

        for (auto node : graph) {
            if (node.second.type == "out") {
                int reg = max_stage;
                get_gate_stage(node.second.node_id, reg);
            }
        }
        // cout << "split stage" << endl;
        clock_t end3 = clock();
        // cout << (end3 - start) / (double)CLOCKS_PER_SEC << " seconds for get gate stage" << endl;
        cout << "max clock cycles: " << max_stage + 1 << endl << endl;

        verify_inject(benchpath_pure, benchpath, blacked, outs, isOpt, c, s, nof, nof_sm, fault_type, countermeasures, variate, outpath_cnf, opt_meth, encode_stage, solver_select, composition);

        clock_t end_final = clock();
        cout << "Encode time: " << (end_final - start) / (double)CLOCKS_PER_SEC << endl;

    }
    return 0;
}

