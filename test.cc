
int main(int argc, char**argv)
    {
    char* foo = new char[100];
//    delete[] foo;
    
    char* fee = new char[99];
    fee = new char[2];
    return foo[2] + fee[0];
    }
