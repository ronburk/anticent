/* lexin - crazy man writes yet another lexical analyzer generator
 */

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <algorithm>
#include <vector>
using std::vector;
#include <memory>
using std::make_unique;


void Fail(const char* format, ...)
    {
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    exit(1);
    }


enum
    {
    EPSILON     = 256,
    SENTINEL    = 257,
    SYMORBAR    = -2,
    SYMCONCAT   = -3,
    };

using Sym = unsigned short;

const char* SymPrint(Sym symbol)
    {
    static char result[128];
    if(symbol == EPSILON)
        sprintf(result, "'<>' ");
    else if(symbol == SENTINEL)
        sprintf(result, "'<#>'");
    else if(symbol >= ' ' && symbol <= '~')
        sprintf(result, "'%c'  ", symbol);
    else
        sprintf(result, "'x%02X'", symbol);
    return result;
    }


using Int    = short;
using IntSet = vector<Int>;
IntSet Append(const IntSet& a, const IntSet& b)
    {
    IntSet result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
    }
void   Sort(IntSet& a)
    {
    std::sort(a.begin(), a.end());
    }
IntSet Union(const IntSet& a, const IntSet& b)
    {
    IntSet result = Append(a, b);
    Sort(result);
    auto iter = std::unique(result.begin(), result.end());
    result.erase(iter, result.end());
    
#if 0
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());

    IntSet result;
    std::set_union(a.begin(), a.end(), b.begin(), b.end(),
        std::inserter(result,result.begin()));
#endif
    return result;
    }
void DumpIntSet(const IntSet& a)
    {
    printf(" {");
    for(const auto& i:a)
        {
        if(&i != &(*a.begin())) // if not first iteration
            printf(",");
        printf("%d", i);
        }
    printf("}");

    }

class SyntaxNode;
class Machine
    {
    struct Transition { int from; Sym on; int to; };
    friend class SyntaxNode;
public:
    static vector<IntSet>       followpos;
    static vector<IntSet>       states;
    static vector<Transition>   transitions;
    
    static void             AddToFollowPos(int pos, IntSet& set);
    static void             CalculateFollowPos(SyntaxNode* tree);
    static void             CreateDfa(SyntaxNode* root);
    static int              GetStateNum(const IntSet& newState, bool Debug);
    static void             Dump();
    static void             DumpDfa();
    static void             Generate(FILE* output);
    };

class SyntaxNode
    {
    friend class Machine;
public:
    SyntaxNode(Sym symbol);
    SyntaxNode(Sym symbol, SyntaxNode* left, SyntaxNode* right=nullptr);
    bool            isSentinel() { return isLeaf && symbol == '\0'; }
    void            Dump();
    int             Relation(Sym left, Sym right);
    void            CalculateFollowPos();
    static int      nodeCount;
    static int      termCount;
    static vector<Sym>  positions;
//    static vector<IntSet>*      followpos;
    bool            isLeaf  = false;
    SyntaxNode*     left    = nullptr;
    SyntaxNode*     right   = nullptr;
    Sym             symbol  = 0;
    int             nodeNum = nodeCount++;
    int             position= 0;
    bool            nullable= false;
    IntSet          firstpos;
    IntSet          lastpos;
    };

vector<Machine::Transition> Machine::transitions;
vector<IntSet> Machine::states;
vector<IntSet> Machine::followpos;
void Machine::AddToFollowPos(int pos, IntSet& set)
    {
    assert(pos >= 0);
    followpos[pos] = Union(followpos[pos], set);
    }
void Machine::CalculateFollowPos(SyntaxNode* tree)
    {
    tree->CalculateFollowPos();
    }

/* GetStateNum() - return number of a (possibly new) state
 */
int Machine::GetStateNum(const IntSet &possible, bool Debug)
    {
    int result = -1;

    // sort, unique
    auto newState = Union(possible, possible);
    if(Debug)
        {
        printf("after union, = ");
        DumpIntSet(newState);
        printf("\n");
        }
    for(size_t iState=0; iState < states.size(); ++iState)
        {
        auto state = states[iState];
        if(newState == state)
            {
            assert(newState.size() == state.size());
            result = iState;
            break;
            }
        }
    if(result == -1)
        {
        states.push_back(newState);
        result  = states.size()-1;
        }
    return result;
    }

void Machine::CreateDfa(SyntaxNode* root)
    {
    states.push_back(root->firstpos);
    for(decltype(states.size()) iState = 0; iState < states.size(); ++iState)
        {
        auto state = states[iState];
        for(Sym iChar=0; iChar<256; ++iChar)
            {
            bool Debug=(iState == 1 && iChar == 'a');
            IntSet newState;
            for(auto spos: state) // for each position in current state
                {
                if(Debug)printf("position in state[%ld]=%d\n", iState, spos);
//                    if(Debug)
//                    printf("fpos = %d, sym=%s\n", fpos, SymPrint(SyntaxNode::positions[fpos]));   
                if(SyntaxNode::positions[spos] == iChar)
                    {
                    newState = Append(newState, followpos[spos]);
                    if(Debug)
                        {
                        printf("newState + followpos[%d] = ", spos);
                        DumpIntSet(newState);
                        printf("\n");
                        }
                    }
                }
            if(!newState.empty())
                {
                if(Debug)
                    DumpIntSet(newState);
                auto newStateNum = GetStateNum(newState, Debug);
                if(Debug)
                    printf(" is equal to state %d\n", newStateNum);
                transitions.push_back(Transition{int(iState), iChar, newStateNum});
                }

            }
        }
    }   
void Machine::DumpDfa()
    {
    printf("====DFA=====\n");
    int iState = 0;
    for(auto state: states)
        {
        printf("%3d ", iState++);
        printf(" {");
        for(const auto& i:state)
            {
            if(&i != &(*state.begin())) // if not first iteration
                printf(",");
            printf("%d", i);
            }
        printf("}\n");
        
        }
    printf("====transitions=====\n");
    for(auto trans: transitions)
        {
        printf("%3d %s %3d\n", trans.from, SymPrint(trans.on), trans.to);
        }
    }

void Machine::Dump()
    {
    printf("====Machine===\n");
    printf("node followpos\n");
    int iNode = 0;
    for(auto fp: followpos)
        {
        printf("%4d ", iNode++);
        printf("{");
        for(const auto& i:fp)
            {
            if(&i != &(*fp.begin())) // if not first iteration
                printf(",");
            printf("%d", i);
            }
        printf("}\n");
        }
    }

static const char* Template = R"x(

bool Lexin(const char* Input)
    {
    bool result = false;
    int  state  = 0;
    int  ch     = *Input++
    for(auto ch = *Input++; ch; ch = *Input++)
        {
        }
    }

)x";

void Machine::Generate(FILE* output)
    {
    printf("switch(state){\n");
    int iTrans = 0;
    for(int iState = 0; iState < states.size(); ++iState)
        {
        printf("    case %d: // state \n", iState);
        assert(iTrans < transitions.size());
        assert(transitions[iTrans].from == iState);
        printf("        switch(
        }
    }

SyntaxNode::SyntaxNode(Sym symbol)
    : isLeaf(true), left(nullptr), right(nullptr), symbol(symbol)
    {
    position = termCount++;
    nullable = false;
    // ??? someday epsilon?
    firstpos.push_back(position);
    lastpos.push_back(position);
    assert(symbol != 0);
    positions.push_back(symbol);
    if(symbol == 0)
        fprintf(stderr, "oh no\n");
    Machine::followpos.push_back(IntSet());
    }

SyntaxNode::SyntaxNode(Sym symbol, SyntaxNode* left, SyntaxNode* right)
        : isLeaf(false), left(left), right(right), symbol(symbol)
        {
        // set nullable
        if(symbol == '.') // if concatenation operator
            nullable = left->nullable && right->nullable;
        else if(symbol == '|')
            nullable = left->nullable || right->nullable;
        else if(symbol == '*')
            nullable = true;
        // set firstpos
        if(symbol == '|')
            firstpos = Union(left->firstpos, right->firstpos);
        else if(symbol == '.')
            {
            if(left->nullable)
                firstpos = Union(left->firstpos, right->firstpos);
            else
                firstpos = left->firstpos;
            }
        else if(symbol == '*')
            firstpos = left->firstpos;
        // set lastpos
        if(symbol == '|')
            lastpos = Union(left->lastpos, right->lastpos);
        else if(symbol == '.')
            {
            if(right->nullable)
                lastpos = Union(left->firstpos, right->firstpos);
            else
                lastpos = right->firstpos;
            }
        else if(symbol == '*')
            lastpos = left->firstpos;
        }

void SyntaxNode::CalculateFollowPos()
    {
    if(!isLeaf)
        {
        if(left)
            left->CalculateFollowPos();
        if(right)
            right->CalculateFollowPos();
        if(symbol == '.')   // if cat-node
            {
            for(auto i: left->lastpos)
                Machine::AddToFollowPos(i, right->firstpos);
            }
        else if(symbol == '*')  // if star-node
            {
            for(auto i: lastpos)
                Machine::AddToFollowPos(i, firstpos);
            }

        }
    }

int SyntaxNode::Relation(Sym left, Sym right)
    {
    int result = -99;

    switch(left){
    case    '(' :
        switch(right) {
        case    ')' : result = 0;
            break;
            }
        break;
        }
    if(result == 99)
        {
        Fail("No relation between '%c' and '%c'\n", left, right);
        }
    return result;
    }


vector<Sym> SyntaxNode::positions;
int SyntaxNode::nodeCount = 0;
int SyntaxNode::termCount = 0;

void SyntaxNode::Dump()
    {
    printf("%c", nullable ? '?' : ' ');
    if(isLeaf)
        printf("%3d", position);
    else
        printf("   ");
    printf("[%3d] %s", nodeNum, SymPrint(symbol));
    if(left)
        printf(" %3d", left->nodeNum);
    else
        printf("   -");
    if(right)
        printf(" %3d", right->nodeNum);
    else
        printf("   -");
    // firstpos
    printf(" {");
    for(const auto& i:firstpos)
        {
        if(&i != &(*firstpos.begin())) // if not first iteration
            printf(",");
        printf("%d", i);
        }
    printf("}");
    // lastpos
    printf(" {");
    for(const auto& i:lastpos)
        {
        if(&i != &(*lastpos.begin())) // if not first iteration
            printf(",");
        printf("%d", i);
        }
    printf("}");
    printf("\n");
    if(left)
        left->Dump();
    if(right)
        right->Dump();
    }



class Parser
    {
public:
    Parser(FILE* input) : input(input), peeked(false) {}
    SyntaxNode* Parse();

private:
    SyntaxNode* ParseE();
    SyntaxNode* ParseC();
    SyntaxNode* ParseU();
    SyntaxNode* ParseT();
    Sym     PeekSym();
    Sym     NextSym();
    bool    Accept(Sym symbol);
    void    Expect(Sym symbol);

    FILE*   input;
    bool    peeked;
    Sym     peekSym;
    Sym     currentSym;
    };


Sym Parser::PeekSym()
    {
    Sym result = peekSym;
    if(!peeked)
        result = peekSym = fgetc(input);
    return result;
    }

Sym Parser::NextSym()
    {
    if(peeked)
        {
        currentSym  = peekSym;
        peeked      = false;
        }
    else // ??? EOF???
        currentSym = fgetc(input);

    return currentSym;
    }
bool Parser::Accept(Sym symbol)
    {
    bool    result = false; // assume failure
    if(symbol == currentSym)
        {
        if(currentSym != SENTINEL)
            NextSym();
        result = true;
        }
    return result;
    }
void Parser::Expect(Sym symbol)
    {
    if(!Accept(symbol))
        Fail("Expecting '%c', not '%c'\n", symbol, currentSym);
    }


/*
E-> '(' E ')'
  | E E
  | E '|' E
  | E '*'
  | x #

make palatable to recursive descent, add op prec, etc:

start->E ('\n'|'\0')

E-> C '|' E
  | C

C-> U C
  | U

U-> T '*'
  | T

T-> '(' E ')'
  | '(' '|' E ')'
  | x

 */

SyntaxNode* Parser::Parse()
    {
    NextSym();
    auto result = ParseE();
    fprintf(stderr, "Parse() RETURNS\n");
    if(currentSym == SENTINEL)
        {
//        result = new SyntaxNode('.', result, new SyntaxNode(256));
        }
    else
        Fail("Missing end of line.\n");
    return result;
    }

SyntaxNode* Parser::ParseE()
    {
    SyntaxNode* result = ParseC();

    if(Accept('|'))
        {
        auto right = ParseE();
        auto left  = result;
        result  = new SyntaxNode('|', left, right);
        }

    return result;
    }

/* C-> U C
 *   | U
 */
SyntaxNode* Parser::ParseC()
    {
    SyntaxNode* result = ParseU();
    switch(currentSym)
        {
        case '*' :
        case '|' :
        case ')' :
        case SENTINEL :
            break;
        default:
            auto left = result;
            auto right = ParseC();
            result = new SyntaxNode('.', left, right);
        }
    return result;
    }
SyntaxNode* Parser::ParseU()
    {
    SyntaxNode* result = ParseT();
    
    if(Accept('*'))
        {
        auto left = result;
        result = new SyntaxNode('*', left);
        }
    return result;
    }
SyntaxNode* Parser::ParseT()
    {
    SyntaxNode* result = nullptr;
    
    if(Accept('('))
        {
        Accept('(');
        result  = ParseE();
        fprintf(stderr, "ParseT() expect right paren\n");
        Expect(')');
        }
    else
        {
        if(currentSym == '\0' || currentSym == '\n')
            currentSym = SENTINEL;
        result = new SyntaxNode(currentSym);
        Accept(currentSym);
        }
    return result;
    }

int main(int argCount, char** args)
    {
    auto parser = make_unique<Parser>(stdin);
    auto tree = parser->Parse();
    printf("====DUMP SYNTAX TREE====\n");
    printf("?=nullable, position, nodenum, sym, left, right, {firstpos} {lastpos}\n");
    tree->Dump();
    Machine::CalculateFollowPos(tree);
    Machine::Dump();
    Machine::CreateDfa(tree);
    Machine::DumpDfa();
    Machine::Generate(stdout);
    }
