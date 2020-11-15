/* lexin - crazy man writes yet another lexical analyzer generator
 */

#include <cassert>
#include <cstring>
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
    
    return result;
    }
void DumpIntSet(FILE*output, const IntSet& a)
    {
    fprintf(output, " {");
    for(const auto& i:a)
        {
        if(&i != &(*a.begin())) // if not first iteration
            fprintf(output, ",");
        fprintf(output, "%d", i);
        }
    fprintf(output, "}");
    }

class SyntaxNode;
class Machine
    {
    struct Transition { int from; Sym on; int to; };
    friend class SyntaxNode;
public:
    static vector<IntSet>       followpos;
    static vector<IntSet>       states;
    static vector<bool>         final;
    static vector<IntSet>       actions;
    static vector<Transition>   transitions;
    
    static void             AddToFollowPos(int pos, IntSet& set);
    static void             CalculateFollowPos(SyntaxNode* tree);
    static void             CreateDfa(SyntaxNode* root);
    static int              GetStateNum(const IntSet& newState, bool Debug);
    static void             Dump(FILE* output);
    static void             DumpDfa(FILE* output);
    static void             Generate(FILE* output);
    static void             GenerateMachine(FILE* output);
    };

class SyntaxNode
    {
    friend class Machine;
public:
    SyntaxNode(Sym symbol);
    SyntaxNode(Sym symbol, SyntaxNode* left, SyntaxNode* right=nullptr);
    void            Dump(FILE* output);
    void            DumpSyntax(FILE* output);
    int             Relation(Sym left, Sym right);
    void            CalculateFollowPos();
    static int      nodeCount;
    static int      termCount;
    static vector<Sym>  positions;
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
vector<bool> Machine::final;
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
//        for(Sym iChar=0; iChar<256; ++iChar)
        for(Sym iChar=0; iChar<=SENTINEL; ++iChar)
            {
            bool Debug=(iState == 1 && iChar == 'a');
            IntSet newState;
            for(auto spos: state) // for each position in current state
                if(SyntaxNode::positions[spos] == iChar)
                    newState = Append(newState, followpos[spos]);
            if(!newState.empty())
                {
                auto newStateNum = GetStateNum(newState, Debug);
                transitions.push_back(Transition{int(iState), iChar, newStateNum});
                }
            }
        }
#if 0
    // now identify final states
    final = vector<bool>(states.size(), false);
    int iState = 0;
    for(auto const& state: states)
        {
        if(std::find(state.begin(), state.end(), final) != state.end())
            final[iState] = true;
        ++iState;
        }
#endif
    }   
void Machine::DumpDfa(FILE* output)
    {
    fprintf(output, "====DFA=====\n");
    int iState = 0;
    for(auto state: states)
        {
        fprintf(output, "%3d ", iState++);
        fprintf(output, " {");
        for(const auto& i:state)
            {
            if(&i != &(*state.begin())) // if not first iteration
                fprintf(output, ",");
            fprintf(output, "%d", i);
            }
        fprintf(output, "}\n");
        
        }
    fprintf(output, "====transitions=====\n");
    for(auto trans: transitions)
        {
        fprintf(output, "%3d %s %3d\n", trans.from, SymPrint(trans.on), trans.to);
        }
    }

void Machine::Dump(FILE* output)
    {
    fprintf(output, "====Machine===\n");
    fprintf(output, "node followpos\n");
    int iNode = 0;
    for(auto fp: followpos)
        {
        fprintf(output, "%4d ", iNode++);
        fprintf(output, "{");
        for(const auto& i:fp)
            {
            if(&i != &(*fp.begin())) // if not first iteration
                fprintf(output, ",");
            fprintf(output, "%d", i);
            }
        fprintf(output, "}\n");
        }
    }


static const char* Template = R"x(

#include <cassert>
#include <cstdio>

bool Lexin(const char* Input)
    {
    bool result = true;
    int  state  = 0;
    int  ch     = *Input++;
    for(auto ch = *Input++; ch; ch = *Input++)
        {
$(TRANS)
        }
NOMATCH:
    return false;
    }

int main()
    {
    const char* text = "aaabbbbbbbabb";
    printf("match is %s\n", Lexin(text) ? "TRUE" : "FALSE");
    }


)x";


void Machine::Generate(FILE* output)
    {
    const char* rover = Template;
    while(*rover && !(rover[0] == '$' && rover[1] == '('))
        fputc(*rover++, output);
    if(!strncmp(rover, "$(TRANS)", 8))
        {
        GenerateMachine(output);
        rover += 8;
        if(*rover == '\n')
            ++rover;
        }
    fprintf(output, "%s", rover);
    }
void Machine::GenerateMachine(FILE* output)
    {
    int indent = 8;
    printf("%*sswitch(state){\n", indent, "");
    indent += 4;
    int iTrans = 0;
    int nStates = int(states.size());
    for(int iState = 0; iState < nStates; ++iState)
        {
        printf("%*scase %d: // state \n", indent, "", iState);
        assert(iTrans < int(transitions.size()));
        assert(transitions[iTrans].from == iState);
        indent += 4;
        printf("%*sswitch(ch){\n", indent, "");
        indent += 4;
        for(; transitions[iTrans].from == iState; ++iTrans)
            {
            Sym ch = transitions[iTrans].on;
            if(ch >= ' ' && ch <= '~')
                printf("%*scase '%c': state=%d; continue;\n", indent, "",
                    ch, transitions[iTrans].to);
            else
                printf("%*scase 0x%02X: state=%d; continue;\n", indent, "",
                    ch, transitions[iTrans].to);
            }
        printf("%*s}\n", indent, "");
        indent -= 4;
        printf("%*sgoto NOMATCH;\n", indent, "");
        indent -= 4;
        }
    printf("%*sdefault: assert(false);\n", indent, "");
    printf("%*s}\n", indent, "");
    indent -= 4;
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

void SyntaxNode::DumpSyntax(FILE* output)
    {
    if(!isLeaf)
        {
        fprintf(output, "(");
        left->DumpSyntax(output);
        if(symbol == '|' || symbol == '.')
            {
            if(symbol == '|')
                fprintf(output, "|");
            right->DumpSyntax(output);
            }
        else if(symbol == '*')
            fprintf(output, "*");
        fprintf(output, ")");
        }
    else
        fprintf(output, "%s", SymPrint(symbol));
    }
void SyntaxNode::Dump(FILE* output)
    {
    fprintf(output, "%c", nullable ? '?' : ' ');
    if(isLeaf)
        fprintf(output, "%3d", position);
    else
        fprintf(output, "   ");
    fprintf(output, "[%3d] %s", nodeNum, SymPrint(symbol));
    if(left)
        fprintf(output, " %3d", left->nodeNum);
    else
        fprintf(output, "   -");
    if(right)
        fprintf(output, " %3d", right->nodeNum);
    else
        fprintf(output, "   -");
    // firstpos
    fprintf(output, " {");
    for(const auto& i:firstpos)
        {
        if(&i != &(*firstpos.begin())) // if not first iteration
            fprintf(output, ",");
        fprintf(output, "%d", i);
        }
    fprintf(output, "}");
    // lastpos
    fprintf(output, " {");
    for(const auto& i:lastpos)
        {
        if(&i != &(*lastpos.begin())) // if not first iteration
            fprintf(output, ",");
        fprintf(output, "%d", i);
        }
    fprintf(output, "}");
    fprintf(output, "\n");
    if(left)
        left->Dump(output);
    if(right)
        right->Dump(output);
    }



class Parser
    {
public:
    Parser(FILE* input) : input(input) {}
    SyntaxNode* Parse();

private:
    SyntaxNode* ParseE();
    SyntaxNode* ParseC();
    SyntaxNode* ParseU();
    SyntaxNode* ParseT();
    Sym     NextSym();
    bool    Accept(Sym symbol);
    void    Expect(Sym symbol);

    bool    quoteMode = false;
    FILE*   input;
    Sym     currentSym;
    };


Sym Parser::NextSym()
    {
    auto ch = fgetc(input);
    if(ch == EOF)
        ch = SENTINEL;

    return currentSym = ch;
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

start->E #

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
        result = new SyntaxNode('.', result, new SyntaxNode(SENTINEL));
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
    for(bool done=false; !done;)
        switch(currentSym)
            {
            case '*' :
            case '|' :
            case ')' :
            case SENTINEL :
                done    = true;
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
    fprintf(stderr, "====DUMP SYNTAX TREE====\n");
    tree->DumpSyntax(stderr);
    fprintf(stderr, "\n?=nullable, position [nodenum] 'sym' left, right, {firstpos} {lastpos}\n");
    tree->Dump(stderr);
    Machine::CalculateFollowPos(tree);
    Machine::Dump(stderr);
    Machine::CreateDfa(tree);
    Machine::DumpDfa(stderr);
    Machine::Generate(stdout);
    }
