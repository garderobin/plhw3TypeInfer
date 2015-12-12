//
//  main.cpp
//  plhw3
//
//  Created by Jasmine Liu on 12/3/15.
//  Copyright © 2015 Jasmine Liu. All rights reserved.
// TODO: output with replacement by BFS

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <stack>
#include <map>
#include <set>
#include <deque>

using namespace std;

/* Constants */
const char TYPE_STARTS[] = {'(', '[', '`', 'i', 'r', 's'}; //len: 6
const char TYPE_AFTERS[] = {']', ')', '-', ',', '&'}; //len: 5
enum state {VARNAME_FIRST, VARNAME_REST, FUNCTYPE, LISTTYPE, ARGLIST, UNIFICATION_QUERY,
    PRIMITIVETYPE, INT, REAL, STR, TYPEVAR};
const state PRIMITIVE_TYPE_STATE[] = {INT, REAL, STR};
const string PRIMITIVE_TYPE_NAME[] = {"int", "real", "str"};

/* MARK: Classes */
/* Every tree represents a type.
 only ARGLIST and LISTTYPE can be a tree with children;
 all of its children are pointers to types;
 a query has at most 4 top trees.
 TODO: getNameStr for Listtype and Functype*/
class Tree {
public:
    state type; // TYPEVAR, INT, REAL, STR, FUNCTYPE, LISTTYPE
    bool final; // whether all of its elements are primitive types
    string nameStr;
    vector<Tree*> *children;
    Tree *parent;
    virtual void addChild(Tree* ch) {}
    virtual string getNameStr() { return nameStr; }
    virtual string getOutputStr(map<string, Tree*>& bounds, vector<set<string>>& eqvl) { return nameStr; }
    virtual ~Tree() {}
};

/* MARK: doing*/
class VarTree: public Tree {
public:
    VarTree() {
        type = TYPEVAR;
        nameStr = "`";
        final = false;
    }
    string getOutputStr(map<string, Tree*>& bounds, vector<set<string>>& eqvl) {
        map<string, Tree*>::iterator itVar = bounds.find(nameStr);
        if (itVar != bounds.end()) { //bounded
            return itVar->second->getOutputStr(bounds, eqvl);
        }
        
        return nameStr;
    }
    
    string getReplaceVarnameStr(string& nameStr, vector<set<string>>& eqvl) {
        for (vector<set<string>>::size_type i = 0, len = eqvl.size(); i < len; ++i) {
            set<string>::iterator itSet = eqvl[i].find(nameStr), setEnd = eqvl[i].end();
            if (itSet != setEnd) { return *(eqvl[i].begin()); }
            
        }
        return nameStr;
    }
    virtual ~VarTree() { }
};

class PrimTree: public Tree {
public:
    PrimTree(int primIndex) {
        type = PRIMITIVE_TYPE_STATE[primIndex];
        nameStr = PRIMITIVE_TYPE_NAME[primIndex];
        final = true;
    }
    virtual ~PrimTree() {}
};

class FuncTree: public Tree {
public:
    FuncTree() {
        type = FUNCTYPE;
        children = new vector<Tree*>();
        final = true;
    }
    void addChild(Tree* ch) { // add node, set final, append nameStr
        children->push_back(ch);
        final = final && ch->final;
    }
    string getNameStr() {
        string rightPart = ") -> " + children->back()->getNameStr();
        nameStr = "(";
        vector<Tree*>::size_type i = 0, len = children->size() - 2;
        for (; i < len; i++) {
            nameStr += (*children)[i]->getNameStr() + ", ";
        }
        nameStr += (*children)[i]->getNameStr() + rightPart; //FIXME: Possible problems!
        return nameStr;
    }
    virtual ~FuncTree() {
        delete []children;
    }
};

class ListTree: public Tree {
public:
    ListTree() {
        type = LISTTYPE;
        children = new vector<Tree*>();
        final = true; // change only if the child is typevar
    }
    void addChild(Tree* ch) {
        children->push_back(ch);
        final = final && ch->final;
    }
    Tree* getChild() {
        return children->front();
    }
    string getNameStr() {
        nameStr = "[" + children->front()->getNameStr() + "]";
        return nameStr;
    }
    virtual ~ListTree() {
        delete []children;
    }
};

/* MARK: Global Variables */
Tree *leftTree, *rightTree, *intTree = new PrimTree(0), *realTree = new PrimTree(1), *strTree = new PrimTree(2);
map<string, Tree*> bounds; //typevar as key, (bound to) final type as value.
vector<set<string>> eqvl; //equivalent typevars in a set, all set in a vector.
//TODO: output map

/* Printers */
void printVector(vector<string> v);
void println(string& s);
void printError();
void printBottom();

/* Temp tester */
int test();

/* Input handlers */
int processLine(string& line);
int process();
bool isValidInputLine(string& s);
bool isValidWithNonTYPEVARTypeOnStackTop(stack<state>& trans, stack<Tree*>& nodes, string::size_type& i, string & s);
bool isValidCharStartType(string& s, string::size_type pos);
bool isValidCharAfterType(string& s, string::size_type pos);
bool isValidPrimitiveType(int primIndex, stack<state>& trans, stack<Tree*>& nodes, string::size_type& i, string& s);
void deleteAllSpacesAndTabs(string& s, string& rst);
void finishNode(stack<state>& trans, stack<Tree*>& nodes);
static inline string &trim(string &s);

/* Unifiers */
bool equalsFinalTerm (Tree* t1, Tree* t2);
void unify(Tree* left, Tree* right);
void unifyTT(Tree* left, Tree* right);
void unifyTV(Tree* term, Tree* var);
void unifyVV(Tree* left, Tree* right);
void boundVarToFinalTermInMap(Tree *var, Tree *term);

int main(int argc, const char * argv[]) {
    //    string line;
    //    while(getline(cin, line)) {
    //        processLine(line);
    //    }
    test();
    return 0;
}

/* MARK: test */
int test() {
    //    set<string> item;
    //    string s1 = "hello", s2 = "`b";
    //    item.insert(s1);
    //    item.insert(s2);
    //    eqvl[0] = *new set<string>("`a", "`b");
    //    eqvl[0] = item;
    //eqvl.push_back(item);
    //    string s =  *(eqvl[0].begin());
    //string s = "hello";
    //println(s);
    return 0;
}


int processLine(string& line) {
    string s = trim(line), sn;
    if (s.empty()) { printError(); }
    if (s.compare("QUIT") == 0) { exit(0); } // process "QUIT"
    deleteAllSpacesAndTabs(s, sn);
    if (!isValidInputLine(sn)) { printError(); }
    else {
        cout << equalsFinalTerm(leftTree, rightTree) << endl;
    }
    return 0;
}

void println (string& s) {
    cout << s << endl;
}

void printVector (vector<string>& v) {
    vector<string>::size_type size = v.size();
    for (int i = 0; i < size; i++) {
        println(v[i]);
    }
}

void printError() {
    cout << "ERR" << endl;
    exit(0);
}

void printBottom() {
    cout << "BOTTOM" << endl;
    exit(0);
}


/* Add all recent finished nodes to their parents.
 Should be called whenever and only when a nodes creation is done. */
void finishNode(stack<state>& trans, stack<Tree*>& nodes) {
    Tree *cur, *parent;
    state curState = UNIFICATION_QUERY;
    while (!trans.empty() && ((curState = trans.top()) == FUNCTYPE)) {
        trans.pop();
        cur = nodes.top(); //FUNCTYPE里面的参数
        nodes.pop(); //FUNCTYPE ENDS
        parent = nodes.top();
        cur->parent = parent;
        parent->addChild(cur);
    }
    // Mark: possible mistakes.
    if (!trans.empty() && (curState == ARGLIST || curState == LISTTYPE)) { // 此时第二晚的node:parent也必须是FuncTree, ListTree
        cur = nodes.top(); // e.g VarTree 此前只退了trans没退nodes
        nodes.pop();
        parent = nodes.top();
        cur->parent = parent;
        parent->addChild(cur); // 把子节点加入父节点时已经把子节点退栈了。
    }
    
}


/* Based on no tab/spaces string; use state stack to check types. */
bool isValidInputLine(string& s) {
    stack<state> trans;
    stack<Tree*> nodes;
    trans.push(UNIFICATION_QUERY);
    for (string::size_type i = 0, len = s.length(); i < len; i++) {
        char c = s[i];
        /* Continue or end a TYPEVAR */
        if (!trans.empty() && trans.top() == VARNAME_REST) {
            if (isalpha(c) || isdigit(c)) { //append varname
                nodes.top()->nameStr += c;
                continue;
            }
            else if (isValidCharAfterType(s, i)) {
                trans.pop(); //typevar ends
                finishNode(trans, nodes);
            } else { return false; }
        } else if (!trans.empty() && trans.top() == VARNAME_FIRST) {
            if (isalpha(c)) {
                trans.pop();
                trans.push(VARNAME_REST);
                nodes.top()->nameStr += c;
                continue;
            } else { return false; }
        }
        
        if (!isValidWithNonTYPEVARTypeOnStackTop(trans, nodes, i, s)) { return false; }
    }
    /* Finish the last type */
    if (trans.empty()) { // the last type is not a typevar
        rightTree = nodes.top();
        return true;
    }
    if (trans.top() == VARNAME_REST) {
        trans.pop(); //typevar ends
        finishNode(trans, nodes);
        rightTree = nodes.top();
    }
    return trans.empty();
}


/* Only special operators that can end a typevar will return true; do not consider alpha or digit*/
bool isValidCharStartType(string& s, string::size_type pos) {
    if (pos == string::npos || pos == s.length()) { return false; }
    return find(begin(TYPE_STARTS), end(TYPE_STARTS), s[pos]) != TYPE_STARTS + 6;
}


/* Check if this char can be a type start mark. */
bool isValidCharAfterType(string& s, string::size_type pos) {
    if (pos == string::npos || pos == s.length()) { return true; }
    return find(begin(TYPE_AFTERS), end(TYPE_AFTERS), s[pos]) != TYPE_AFTERS + 5;
}


/* Handle special mark chars. */
bool isValidWithNonTYPEVARTypeOnStackTop(stack<state>& trans, stack<Tree*>& nodes, string::size_type& i, string & s) {
    switch (s[i]) { //因为上边已经处理过TYPEVAR的结束问题，这里不用担心开启新type的那几个标志符会与前面导致非法输入的问题。结束符要检查需要孩子的时候孩子是否为空。
        case ')':
            if (trans.empty() || trans.top() != ARGLIST || s[i-1] == ',' || s[i+1] != '-' || nodes.top()->children->size() < 1) { return false; }
            trans.pop(); //只退ARGLIST不退FUNCTYPE, 也没有树可以退
            break;
        case ']':
            if (trans.empty() || trans.top() != LISTTYPE || nodes.top()->type != LISTTYPE || nodes.top()->children->size() != 1) { return false; }
            trans.pop();
            finishNode(trans, nodes);
            break;
        case '&': //&, - 和, 来结束一个typevar及其嵌套的函数的情况上面已经处理过，这里不需要考虑, 但他们接下来必须跟新type
            if (trans.empty() || trans.top() != UNIFICATION_QUERY || !isValidCharStartType(s, i+1)) { return false; }
            trans.pop();
            leftTree = nodes.top(); //此时理论上nodes这个栈里面也只剩一个Tree并且它就是leftTree
            nodes.pop();
            break;
        case '-': // FUNCTYPE 在top只能是arglist已经结束的情况
            if (trans.empty() || trans.top() != FUNCTYPE || s[++i] !='>' || !isValidCharStartType(s, i+1)) { return false; }
            else { break; }
        case ',':
            if (trans.empty() || trans.top() != ARGLIST || !isValidCharStartType(s, i+1) || nodes.top()->children->size()<1) { return false; }
            else { break; }
        case '(': //上面已经处理过TYPEVAR还在继续而出现开启一个新type的标志的情况，这里不需要考虑
            trans.push(FUNCTYPE);
            trans.push(ARGLIST);
            nodes.push(new FuncTree());
            break;
        case '[':
            trans.push(LISTTYPE);
            nodes.push(new ListTree());
            break;
        case '`':
            trans.push(VARNAME_FIRST);
            nodes.push(new VarTree()); // this VarTree has only name "`" which needs to be appended.
            break;
        case 'i':
            if (isValidPrimitiveType(0, trans, nodes, i, s)) { break; }
            else { return false; }
        case 'r':
            if (isValidPrimitiveType(1, trans, nodes, i, s)) { break; }
            else { return false; }
        case 's':
            if (isValidPrimitiveType(2, trans, nodes, i, s)) { break; }
            else { return false; }
        default: return false;
    }
    return true;
}


/* Based on: trans.top() can not be VARNAME_FIRST or VARNAME_REST */
bool isValidPrimitiveType(int primIndex, stack<state>& trans, stack<Tree*>& nodes, string::size_type& i, string& s) {
    string::size_type len = PRIMITIVE_TYPE_NAME[primIndex].length();
    if (s.substr(i, len).compare(PRIMITIVE_TYPE_NAME[primIndex]) != 0 || !isValidCharAfterType(s, i+len)) { return false; }
    i += len - 1;
    nodes.push(new PrimTree(primIndex)); // create a PrimTree
    finishNode(trans, nodes);
    return true;
}

/* Compares two same-type final terms and returns true if they are equal */
bool equalsFinalTerm (Tree* t1, Tree* t2) {
    if (t1->type != t2->type) { return false; } // 也许以后还会用都到这一句，但是在当下的语境中能用到这个函数之前两个term的subtype已经被比较过了
    if (t1->type == LISTTYPE || t1->type == FUNCTYPE) {
        for (vector<Tree*>::iterator it1 = t1->children->begin(), it2 = t2->children->begin(); it1 != t1->children->end(); ++it1, ++it2) {
            if (!equalsFinalTerm(*it1, *it2)) { return false; }
        }
    }
    return true;
}


/* Based on trimmed str. (do not check all chars valid, or transthesis valid.)
 Only check invalid case: anything + spaces + alpha/digit except for primitive types. */
void deleteAllSpacesAndTabs(string& s, string& rst) {
    for (string::size_type posStart = 0, posEnd = 0, len = s.length(); posStart < len; posStart++) {
        if (!isspace(s[posStart])) {
            rst += s[posStart];
            continue;
        }
        
        for (posEnd = posStart + 1; posEnd < len && isspace(s[posEnd]); posEnd++) {}
        //这一步能够解决所有的"->"中间有空格的情况,没有空格的“->”作为一个functype开头或者单独的“-”和“>”type检查里面处理
        if (posStart > 0 && s[posStart-1] == '-') {
            if (s[posEnd] == '>') { continue; }
            else { printError(); }
        }
        if (s[posEnd] == '>') { printError(); }
        else if (isdigit(s[posEnd])) { printError(); }
        else if (isalpha(s[posEnd])) {
            switch (s[posEnd]) {
                case 'i':
                case 'r':
                case 's':
                    posStart = posEnd - 1;
                    break;
                default: printError(); break;
            }
        }
        
    }
}


/* Trim from both ends */
static inline string &trim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());   // trim from end.
    s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));          // trim from start.
    return s;
}

/*  ERR: func & func with different lengths of arglists.
 BOTTOM:
 case_1: two different final types. (eg. int & real)
 case_2: recursive in functypes (eg. `a & (`a, int)->int)
 return 0: successfully unified. */
void unify(Tree* left, Tree* right) {
    if (left->type == TYPEVAR && right->type == TYPEVAR) { unifyVV(left, right); }
    else if (left->type == TYPEVAR && right->type != TYPEVAR) { unifyTV(right, left); }
    else if (left->type != TYPEVAR && right->type == TYPEVAR) { unifyTV(left, right); }
    else { unifyTT(left, right); }
}


/* MARK: unify two terms. (NOT TESTED)
 case_0: different types:               eg. int & real/()/[],                   # BOTTOM
 case_1: both func, different lengths of arglist                                # ERR
 case_2: both final:                    eg. int & int,                          # do nothing
 case_3: at least one is not final:     eg. final() & (B1, B2, ... Bn)          # for every (Ai, Bi), unify(Ai, Bi)*/
void unifyTT(Tree* left, Tree* right) {
    if (left->type != right->type) { printBottom(); }
    if (left->type == FUNCTYPE && right->type == FUNCTYPE && left->children->size() != right->children->size()) { printError(); }
    if (left->final && right->final) {
        if (equalsFinalTerm(left, right)) { printBottom(); }
        else { return; }
    } else {
        for (vector<Tree*>::iterator it1 = left->children->begin(), it2 = right->children->begin(); it1 != left->children->end(); ++it1, ++it2) {
            unify(*it1, *it2); // not tested
        }
    }
    
}


/* TODO: unify a term and a variable.
 case_0: bound(var) =   final(term),    #do nothing
 case_1: bound(var) !=  final(term),    #BOTTOM
 case_2: bound(var),    unfinal(term),  #check bound(var)-type with term and unify or BOTTOM
 case_1: unbound(var),  final(term),    #bound the var to the term and update bounds.
 case_2: unbound(var),  unfinal(term),  #try finding var's name in term's posterities. If found, #BOTTOM.
 */
void unifyTV(Tree* var, Tree* term) {
    string varStr = var->getNameStr();
    map<string, Tree*>::iterator varBound = bounds.find(varStr), bEnd = bounds.end();
    if (varBound == bEnd && term->final) {
        if (!equalsFinalTerm(varBound->second, term)) { printBottom(); }
    } else if (varBound == bEnd && !term->final) {
        Tree* vb = varBound->second;
        if (vb->type != term->type) { printBottom(); }
        if (vb->type == FUNCTYPE || vb->type == LISTTYPE) { // set term and all of its prosteriors to final, bound its prosterious to vb's respective child.
            
        } else {
            boundVarToFinalTermInMap(var, term);
        }
    } else if (varBound != bEnd && term->final) {
        
    } else { //unbound(var),  unfinal(term)
        
    }
    
    // TODO: term is not final.Try finding var's name in term's posterities.
    
    // TODO: If found, #BOTTOM; else, bound the var to the term in bounds.
    
}


/* MARK: unify two typevars. (not tested)
 Traverse both varnames in bounds to find whether eigher is bound to a final tree.
 case_0: one is bounded to final, the other not,    #bound the other and all its equivalence to the final term.
 case_1: both are bounded to same final,            #do nothing
 case_2: both are bounded, but to different final,  #BOTTOM
 case_3: neither is bounded, add them to eqvl sets.
 Update everything in bounds that use the two name as keys.
 */
void unifyVV(Tree* left, Tree* right) {
    string l = left->getNameStr(), r = right->getNameStr();
    map<string, Tree*>::iterator mitLeft = bounds.find(l), mitRight = bounds.find(r); //TODO: 改两遍遍历为一遍遍历
    
    if (mitLeft == bounds.end() && mitRight == bounds.end()) { /* neither is bounded, update the eqvl sets. */
        /* Find their previous equivalences. MARK: 遍历eqvl vector */
        vector<set<string>>::size_type ePosLeft = -1, ePosRight = -1, i, len = eqvl.size();
        for (i = 0; i < len && (ePosLeft == -1 || ePosRight == -1); i++) {
            set<string>::iterator eitLeft = eqvl[i].find(l), eitRight = eqvl[i].find(r), setEnd = eqvl[i].end();
            if (eitLeft != setEnd) { ePosLeft = i; }
            if (eitRight != setEnd) { ePosRight = i; }
        }
        
        if (ePosLeft == -1 && ePosRight == -1) {
            //            eqvl.push_back(new set<string>(l, r)); // FIXME: possible problems
            set<string> item;
            item.insert("`a");
            eqvl.push_back(item);
            return;
        } else if (ePosLeft == -1) {
            eqvl[ePosRight].insert(l); // FIXME: possible problems about adding element to a set by vector iterator.
        } else if (ePosRight == -1) {
            eqvl[ePosLeft].insert(r); // FIXME: possible problems about adding element to a set by vector iterator.
        } else if (ePosRight == ePosLeft){ // both have same equivalences, do nothing
        } else { // left and right have different eqvl sets, combine them.
            eqvl[ePosLeft].insert(eqvl[ePosRight].begin(), eqvl[ePosRight].end());
        }
        return;
    } else if (mitLeft == bounds.end()) { /* bound the free variable to the other's bounded final term. */
        bounds[l] = mitRight->second; // FIXME: possible problems by using string key as index
    } else if (mitRight == bounds.end()) {
        bounds[r] = mitLeft->second; // FIXME: possible problems by using string key as index
    } else if (equalsFinalTerm(bounds[l], bounds[r])) { // already bounded to same final term, do nothing.
    } else {
        printBottom();
    }
    
}


/* TODO: bound the var to the term in bounds.
 case_0: var not in the map,                                        #add pair to bounds: bound var to term in bounds
 case_1: var has already bounded to the same final term,            #do nothing
 case_1: var has already bounded to the different address, same type,  final term,            #compare identity, dif>>BOTTOM or same>>repace.
 case_2: var has already bounded to a different address, same type, not final term, #change var to bound to new final term in bounds
 case_3: var has already bounded to a different type of term,       #BOTTOM
 case_4: var has not bounded to any term, but equals to other vars, #update the var and its equals to bound to term in maps
 */
void boundVarToFinalTermInMap(Tree *var, Tree *term) {
    
}


