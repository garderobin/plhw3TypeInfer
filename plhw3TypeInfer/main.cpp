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
 a query has at most 4 top trees. */
class Tree {
public:
    state type; // TYPEVAR, INT, REAL, STR, FUNCTYPE, LISTTYPE
    bool final; // whether all of its elements are primitive types
    string nameStr;
    vector<Tree*> *children;
    Tree *parent;
    virtual void addChild(Tree* ch) {}
    virtual vector<Tree*>::size_type getChildCount() { return 0; }
    virtual string getReplaceVarname(vector<set<string>>& eqNames) { return nameStr; }
    virtual string getOutputStr(map<string, Tree*>& bounds, vector<set<string>>& eqNames, map<string, set<Tree*>>& eqTrees) { return nameStr; }
    virtual void finalize() { //FIXME: 需要把没一轮的leftTree和rightTree储存下来
        final = true;
        if (parent != NULL) {
            parent->final = true;
            for (vector<Tree*>::iterator it = parent->children->begin(), cEnd = parent->children->end(); it != cEnd && parent->final; ++it) {
                parent->final = parent->final && (*it)->final;
            }
        }
    }
    virtual ~Tree() {}
};

class VarTree: public Tree {
public:
    VarTree() {
        type = TYPEVAR;
        nameStr = "`";
        final = false;
    }
    string getOutputStr(map<string, Tree*>& bounds, vector<set<string>>& eqNames, map<string, set<Tree*>>& eqTrees) {
        map<string, Tree*>::iterator itVar = bounds.find(nameStr);
        if (itVar != bounds.end()) { //final bounded
            return itVar->second->getOutputStr(bounds, eqNames, eqTrees);
        }
        
        string s = getReplaceVarname(eqNames); //replacable by an earlier var
        if (s.compare(nameStr) != 0) { return s; }
        
        map<string, set<Tree*>>::iterator eit = eqTrees.find(nameStr); // look for alternative terms even not final to replace this typevar
        if (eit != eqTrees.end()) { // a equals to a non-final term
            for (set<Tree*>::iterator tit = eit->second.begin(), end = eit->second.end(); tit != end; ++tit) {
                if ((*tit)->type != TYPEVAR) { return (*tit)->getOutputStr(bounds, eqNames, eqTrees); }
            }
        }
        
        return nameStr;

    }
    
    string getReplaceVarname(vector<set<string>>& eqNames) {
        for (vector<set<string>>::size_type i = 0, len = eqNames.size(); i < len; i++) { //unbounded but is replacable by an ealier var
            set<string>::iterator sit = eqNames[i].find(nameStr), setEnd = eqNames[i].end();
            if (sit != setEnd) {
                return *(eqNames[i].begin());
            }
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
        ch->parent = this;
    }
    vector<Tree*>::size_type getChildCount() { return children->size(); }
    string getOutputStr(map<string, Tree*>& bounds, vector<set<string>>& eqNames, map<string, set<Tree*>>& eqTrees) {
        string rightPart = ") -> " + children->back()->getOutputStr(bounds, eqNames, eqTrees);
        nameStr = "(";
        vector<Tree*>::size_type i = 0, len = children->size() - 2;
        for (; i < len; i++) {
            nameStr += (*children)[i]->getOutputStr(bounds, eqNames, eqTrees) + ", ";
        }
        nameStr += (*children)[i]->getOutputStr(bounds, eqNames, eqTrees) + rightPart; //FIXME: Possible problems!
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
        ch->parent = this;
    }
    Tree* getChild() {
        return children->front();
    }
    vector<Tree*>::size_type getChildCount() { return children->size(); }
    string getOutputStr(map<string, Tree*>& bounds, vector<set<string>>& eqNames, map<string, set<Tree*>>& eqTrees) {
        nameStr = "[" + children->front()->getOutputStr(bounds, eqNames, eqTrees) + "]";
        return nameStr;
    }
    virtual ~ListTree() {
        delete []children;
    }
};
    
/* MARK: Global Variables */
Tree *leftTree, *rightTree;
set<Tree*> roots;
map<string, Tree*> bounds; //typevar's nameStr as key, (bound to) final type as value.
vector<set<string>> eqNames; //equivalent typevars in a set, all set in a vector.
map<string, set<Tree*>> eqTrees; //equivalent trees, key is always the front of a set in eqNames
    
/* Printers */
void printError();
void printBottom();
    
/* Temp tester */
int test();
    
/* Input handlers */
int processLine(string& line);
int process();
bool isValidInputLine(string& s);
bool isValidWithNonTYPEVARTypeOnStackTop(stack<state>& trans, stack<Tree*>& nodes, string::size_type& i, string& s);
bool isValidCharStartType(string& s, string::size_type pos);
bool isValidCharAfterType(string& s, string::size_type pos);
bool isValidPrimitiveType(int primIndex, stack<state>& trans, stack<Tree*>& nodes, string::size_type& i, string& s);
void deleteAllSpacesAndTabs(string& s, string& rst);
void finishNode(stack<state>& trans, stack<Tree*>& nodes);
static inline string &trim(string &s);
    
/* Unifiers */
void insertEqvlNames(Tree *left, Tree *right);
void addEquivalentPair(Tree* left, Tree* right);
void boundSingleVarToTerm(Tree *var, Tree *term);
void boundEquivVarsOfTermToTheTerm(Tree *term);
void boundFreeVarToFinalTermInMap(Tree *var, Tree *term);
bool isFreeVarAPosteriorOfUnfinalListOrFunc (Tree *var, Tree *term);
bool equalsFinalTerm (Tree* t1, Tree* t2);
void unifyChildren(Tree* left, Tree* right);
void unify(Tree* left, Tree* right);
void unifyTT(Tree* left, Tree* right);
void unifyVT(Tree* term, Tree* var);
void unifyVV(Tree* left, Tree* right);
    
    
int main(int argc, const char * argv[]) {
    string line;
    while(getline(cin, line)) {
        processLine(line);
        cout << leftTree->getOutputStr(bounds,eqNames, eqTrees) << endl;
    }
    return 0;
}

/* MARK: test */
int test() {
    return 0;
}
    
    
int processLine(string& line) {
    string s = trim(line), sn;
    if (s.empty()) {
        printError();
    }
    if (s.compare("QUIT") == 0) { exit(0); } // process "QUIT"
    deleteAllSpacesAndTabs(s, sn);
    if (!isValidInputLine(sn)) {
        printError();
    }
    unify(leftTree, rightTree);
    roots.insert(leftTree);
    roots.insert(rightTree);
    return 0;
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
    // MARK: possible mistakes.
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
            if (trans.empty() || trans.top() != ARGLIST || s[i-1] == ',' || s[i+1] != '-' || nodes.top()->getChildCount() < 1) { return false; }
            trans.pop(); //只退ARGLIST不退FUNCTYPE, 也没有树可以退
            break;
        case ']':
            if (trans.empty() || trans.top() != LISTTYPE || nodes.top()->type != LISTTYPE || nodes.top()->getChildCount() != 1) { return false; }
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
            if (trans.empty() || trans.top() != ARGLIST || !isValidCharStartType(s, i+1) || nodes.top()->getChildCount()<1) { return false; }
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
    s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());   // trim from end
    s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));          // trim from start.
    return s;
}


/* Add new set to eqNames */
void insertEqvlNames(Tree *left, Tree *right) {
    set<string> nameSet;
    nameSet.insert(left->nameStr);
    nameSet.insert(right->nameStr);
    eqNames.push_back(nameSet);
}


/* Add new set to eqTrees */
void insertEqvlTrees(Tree *left, Tree *right) {
    set<Tree*> treeSet;
    treeSet.insert(left);
    treeSet.insert(right);
    eqTrees[left->nameStr] = treeSet;
}
/* Insert new typevar equivalence pair.
     如果是var和var的pair,
     因为插入新pair的时候永远用eqNames的开头来做eqTrees的key，所以eqNames的检索一定可以找到eqTrees.但是反过来，插入eqTress的时候未必有对等的eqNames。
     如果是var和term的对等关系，只操作eqTrees.
     不记录term和term的对等关系，因为没有意义。
     MARK: doing 在解决 `a = [`b], `b = `c 的问题*/
void addEquivalentVVPair(Tree *left, Tree *right) {
    map<string, set<Tree*>>::iterator ml = eqTrees.find(left->nameStr), mr = eqTrees.find(right->nameStr), mEnd = eqTrees.end();
    if (ml != mEnd && mr == mEnd) {
        eqTrees[left->nameStr].insert(right);
    } else if (ml != mEnd && mr != mEnd) {
        ml->second.insert(mr->second.begin(), mr->second.end());
        eqTrees.erase(mr);
    } else if (ml == mEnd && mr != mEnd) {
        mr->second.insert(left);
        eqTrees[left->nameStr] = mr->second;
        eqTrees.erase(mr);
    } else { // ml == mEnd && mr == mEnd
        insertEqvlTrees(left, right);
    }
    insertEqvlNames(left, right);
}


/* 如果这个var没有任何var equivalent，不会进到这个函数里面来 */
void addEquivalentVTPair(Tree *var, Tree *term) {
    map<string, set<Tree*>>::iterator ml = eqTrees.find(var->nameStr), mEnd = eqTrees.end();
    if (ml != mEnd) {
        for (set<Tree*>::iterator it = ml->second.begin(), sEnd = ml->second.end(); it != sEnd; ++it) {
            if ((*it)->type != TYPEVAR && (*it)->getChildCount() != term->getChildCount()) { printBottom(); }
        }
        ml->second.insert(term);
    } else { insertEqvlTrees(var, term); }
    
}

/* Bound a first appear typevar to a term and updates its parents by final*/
void boundSingleVarToTerm(Tree *var, Tree *term) {
    bounds[var->nameStr] = term;
    var->finalize();
    Tree cur = *var;
    while (cur.parent != NULL && cur.parent->final) {
        boundEquivVarsOfTermToTheTerm(cur.parent);
        cur = *(cur.parent);
    }
}
    
/* Look for all equivalent vars to the parent tree and bounds them to parent */
void boundEquivVarsOfTermToTheTerm(Tree *term) {
    for (map<string, set<Tree*>>::iterator mit = eqTrees.begin(), mEnd = eqTrees.end(); mit != mEnd; ++mit) {
        set<Tree*>::iterator sit = mit->second.find(term), sEnd = mit->second.end();
        if (sit != sEnd) { // mit contains term, key(mit) is the term-equiv vars' common outputStr
            set<Tree*>::iterator varItr = eqTrees[mit->first].begin(), vEnd = eqTrees[mit->first].end();
            for (; varItr != vEnd; ++varItr) {
                if ((*varItr)->type == TYPEVAR) {
                    boundSingleVarToTerm(*varItr, term);
                }
            }
            
        }
    }
}
    
/* Bound the var to the term in bounds and update its equivalence. */
void boundFreeVarToFinalTermInMap(Tree *var, Tree *term) {
    boundSingleVarToTerm(var, term);
        
    map<string, set<Tree*>>::iterator treeEqv = eqTrees.find(var->getReplaceVarname(eqNames)), tEnd = eqTrees.end();
    if (treeEqv != tEnd) {
        for (set<Tree*>::iterator it = treeEqv->second.begin(), sEnd = treeEqv->second.end(); it != sEnd; ++it) {
            if ((*it)->type == TYPEVAR) { boundSingleVarToTerm(*it, term); }
            else if ((*it)->getChildCount() != term->getChildCount()) { printBottom(); }
            else { unifyChildren(*it, term); }
        }
    }
    
}
    
/* Use DFS to find whether a free variable appears in the posteriors of a not-final LISTTYPE or FUNCTYPE.*/
bool isFreeVarAPosteriorOfUnfinalListOrFunc(Tree *var, Tree *term) {
    for (vector<Tree*>::iterator it1 = term->children->begin(), itEnd = term->children->end(); it1 != itEnd; ++it1) {
        if ((*it1)->type == TYPEVAR && ((*it1)->getOutputStr(bounds, eqNames, eqTrees)).compare(var->getOutputStr(bounds, eqNames, eqTrees)) == 0)
        { return true; }
    }
    return false;
}
    
/* Unify all children of a LISTTYPE or a FUNCTYPE */
void unifyChildren(Tree* left, Tree* right) { // MARKING: 遍历children
    for (vector<Tree*>::iterator it1 = left->children->begin(), it2 = right->children->begin(); it1 != left->children->end(); ++it1, ++it2) {
        unify(*it1, *it2); // not tested
    }
}
    

/* Main algorithm. */
void unify(Tree* left, Tree* right) {
    if (left->type == TYPEVAR && right->type == TYPEVAR) { unifyVV(left, right); }
    else if (left->type == TYPEVAR && right->type != TYPEVAR) { unifyVT(left, right); }
    else if (left->type != TYPEVAR && right->type == TYPEVAR) { unifyVT(right, left); }
    else { unifyTT(left, right); }
}
    
    
/* Unify two terms. (NOT TESTED)
     case_0: different types:               eg. int & real/()/[],                   # BOTTOM
     case_1: both func, different lengths of arglist                                # ERR
     case_2: both final:                    eg. int & int,                          # do nothing
     case_3: at least one is not final:     eg. final() & (B1, B2, ... Bn)          # for every (Ai, Bi), unify(Ai, Bi)*/
void unifyTT(Tree* left, Tree* right) {
    if (left->type != right->type) { printBottom(); }
    if (left->type == FUNCTYPE && right->type == FUNCTYPE && left->getChildCount() != right->getChildCount()) {
        printError();
    }
    if (left->final && right->final) {
        if (!equalsFinalTerm(left, right)) { printBottom(); }
        else { return; }
    } else { unifyChildren(left, right); }
    
}
    
    
/* Unify a term and a variable.
     case_0: bound(var) =   final(term),    #do nothing
     case_1: bound(var) !=  final(term),    #BOTTOM
     case_2: bound(var),    unfinal(term),  #check bound(var)-type with term and unify or BOTTOM
     // set term and all of its prosteriors to final, bound its prosterious to vb's respective child.
     case_1: unbound(var),  final(term),    #bound the var to the term and update bounds.
     case_2: unbound(var),  unfinal(term),  #try finding var's name in term's posterities. If found, #BOTTOM.
     
     //term has to be FUNCTYPE or LISTTYPE, else it is final
     MARK:doing 检查eqTrees里面是否有与term类型不相同的对等节点，如果有，BOTTOM
     */
void unifyVT(Tree* var, Tree* term) {
    string varStr = var->getOutputStr(bounds, eqNames, eqTrees);
    map<string, Tree*>::iterator varBound = bounds.find(varStr), bEnd = bounds.end();
    if (varBound != bEnd) {
        Tree* vb = varBound->second;
        if (term->final) {              //bound(var), final(term)
            if (!equalsFinalTerm(vb, term)) { printBottom(); }
        } else {                        //bound(var), unFinal(term) 这里不用管eqTrees因为后者在设置bound的时候就已经删掉了多余的东西
            if (vb->type != term->type) { printBottom(); }
            unifyChildren(vb, term); // FIXME: possible improvements here.
        }
    } else { //unbound but may have equivalent trees
        if (term->final) {              //unbound(var), final(term)
            boundFreeVarToFinalTermInMap(var, term);
        } else {                        //unbound(var), unfinal(term)
            if (isFreeVarAPosteriorOfUnfinalListOrFunc(var, term)) { printBottom(); }
            // add the term to the eqTrees for this var
            addEquivalentVTPair(var, term); //FIXME: possible problems of missing cases consideration.
        }
        
    }
//    if (varBound == bEnd && term->final) {              //unbound(var), final(term)
//            boundFreeVarToFinalTermInMap(var, term);
//        } else if (varBound != bEnd && !term->final) {      //bound(var), unFinal(term)
//            //        eqTrees[varStr] // FIXME: bound(var), unFinal(term), handle eqTrees
//            Tree* vb = varBound->second;
//            if (vb->type != term->type) {
//                printBottom();
//            }
//            unifyChildren(vb, term); // FIXME: possible improvements here.
//        } else if (varBound != bEnd && term->final) {       //bound(var), final(term)
//            if (!equalsFinalTerm(varBound->second, term)) { printBottom(); }
//        } else {                                            //unbound(var),  unfinal(term)
//            if (isFreeVarAPosteriorOfUnfinalListOrFunc(var, term)) {
//                printBottom();
//            }
//        }
//        
//    }
}
    
    /* Unify two typevars. (not tested)
     Traverse both varnames in bounds to find whether eigher is bound to a final tree.
     case_0: one is bounded to final, the other not,    #bound the other and all its equivalence to the final term.
     case_1: both are bounded to same final,            #do nothing
     case_2: both are bounded, but to different final,  #BOTTOM
     case_3: neither is bounded, add them to eqNames sets.
     Update everything in bounds that use the two name as keys.
     */
    void unifyVV(Tree* left, Tree* right) {
        string l = left->nameStr, r = right->nameStr;
        map<string, Tree*>::iterator mitLeft = bounds.find(l), mitRight = bounds.find(r); //TODO: 改两遍遍历为一遍遍历
        
        if (mitLeft == bounds.end() && mitRight == bounds.end()) { /* neither is bounded, update the eqNames sets. */
            /* Find their previous equivalences. MARK: 遍历eqNames vector */
            vector<set<string>>::size_type ePosLeft = -1, ePosRight = -1, len = eqNames.size();
            vector<set<string>>::iterator eit = eqNames.begin();
            bool addRightIter = true;
            for (vector<set<string>>::size_type i = 0; i < len && (ePosLeft == -1 || ePosRight == -1); ++i) {
                set<string>::iterator eitLeft = eqNames[i].find(l), eitRight = eqNames[i].find(r), setEnd = eqNames[i].end();
                if (eitLeft != setEnd) { ePosLeft = i; }
                if (eitRight != setEnd) {
                    ePosRight = i;
                    addRightIter = false;
                }
                if (addRightIter) { ++eit; } // stop iterating once found the pos for right.
            }
            
            if (ePosLeft == -1 && ePosRight == -1) {
                addEquivalentVVPair(left, right);
                return;
            } else if (ePosLeft == -1) {
                eqNames[ePosRight].insert(l); // FIXME: possible problems about adding element to a set by vector iterator.
                eqTrees[*(eqNames[ePosRight].begin())].insert(left);
            } else if (ePosRight == -1) {
                eqNames[ePosLeft].insert(r); // FIXME: possible problems about adding element to a set by vector iterator.
                eqTrees[*(eqNames[ePosLeft].begin())].insert(right);
            } else if (ePosRight == ePosLeft){ // both have same equivalences, do nothing
            } else { // left and right have different eqNames sets, combine them.
                eqNames[ePosLeft].insert(eqNames[ePosRight].begin(), eqNames[ePosRight].end());
                eqNames.erase(eit); // erase the set with the right str in eqNames
                string rOutputStr = *(eqNames[ePosRight].begin());
                set<Tree*> rTreeSet = eqTrees[rOutputStr];
                eqTrees[*(eqNames[ePosLeft].begin())].insert(rTreeSet.begin(), rTreeSet.end());
                eqTrees.erase(rOutputStr);
            }
            return;
        } else if (mitLeft == bounds.end()) { /* bound the free variable to the other's bounded final term. */
            bounds[l] = mitRight->second;
        } else if (mitRight == bounds.end()) {
            bounds[r] = mitLeft->second;
        } else if (equalsFinalTerm(bounds[l], bounds[r])) { // already bounded to same final term, do nothing.
        } else {
            printBottom();
        }
        
    }