#include <iostream>

using std::cout;
using std::endl;

int main(int argc, char** argv)
{
    //显式标记为已使用，避免编译器警告
    (void)argc;
    (void)argv;
    
    cout << "func test_interface " << endl;
    return 0;
}