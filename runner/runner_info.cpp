#include <iostream>
#include <base/info.h>

void foo2() {
    std::cout << lon::backtraceString() << '\n';
}

void foo1() {
    foo2();
}

int main() {
    const auto host_name = lon::getHostName();
    std::cout << host_name << "--:"<< host_name.size() << '\n';

    foo1();

    return 0;
}