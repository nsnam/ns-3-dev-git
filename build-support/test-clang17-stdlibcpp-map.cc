#include <iostream>
#include <map>
#include <tuple>

int
main()
{
    std::map<std::tuple<int, int>, int> testMap;
    testMap[{1, 1}] = 2;
    std::cout << testMap.at({1, 1}) << std::endl;
    return 0;
}
