#include <iostream>

#include "../Core/Core.h"
#include "../Network/Network.h"

int main() {
    std::cout << Core::GetCoreName() << '\n';
    std::cout << Network::GetNetworkName() << '\n';
    return 0;
}
