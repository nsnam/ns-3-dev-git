/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

/**
 * @file
 * Example to demonstrate and test the registration of templated classes.
 */

#include "ns3/core-module.h"

NS_LOG_COMPONENT_DEFINE("TemplatedClassesExample");

namespace ns3
{
namespace alphaTest
{

/**
 * Test class, inside the namespace ns3::alphaTest
 */
template <typename T>
class AlphaProt : public std::enable_if_t<std::is_same_v<Object, T>, T>
{
  public:
    /**
     * @brief Get the TypeId
     *
     * @return The TypeId for this class
     */
    static TypeId GetTypeId()
    {
        std::string name = GetTemplateClassName<AlphaProt<T>>();

        static TypeId tid = TypeId(name).SetParent<Object>();
        return tid;
    }
};

} // namespace alphaTest

/**
 * Test class, inside the namespace ns3
 */
template <typename T>
class BetaProt : public std::enable_if_t<std::is_same_v<Object, T>, T>
{
  public:
    /**
     * @brief Get the TypeId
     *
     * @return The TypeId for this class
     */
    static TypeId GetTypeId()
    {
        std::string name = GetTemplateClassName<BetaProt<T>>();

        static TypeId tid = TypeId(name).SetParent<Object>();
        return tid;
    }
};

NS_OBJECT_TEMPLATE_CLASS_WITH_NS_DEFINE(alphaTest, AlphaProt, ns3, Object);
NS_OBJECT_TEMPLATE_CLASS_DEFINE(BetaProt, Object);

} // namespace ns3

using namespace ns3;

int
main(int argc, char* argv[])
{
    Ptr<alphaTest::AlphaProt<Object>> alpha = CreateObject<alphaTest::AlphaProt<Object>>();

    std::string alphaName("ns3::alphaTest::AlphaProt<Object>");
    std::string alphaClassName(alpha->GetInstanceTypeId().GetName());
    std::string alphaClassNameDirect(GetTemplateClassName<alphaTest::AlphaProt<Object>>());

    if (alphaClassName != alphaName)
    {
        std::cerr << "Unexpected templated class name (\"" << alphaClassName << "\" instead of \""
                  << alphaName << "\")";
        return -1;
    }
    if (alphaClassNameDirect != alphaName)
    {
        std::cerr << "Unexpected templated class name (\"" << alphaClassNameDirect
                  << "\" instead of \"" << alphaName << "\")";
        return -1;
    }

    Ptr<BetaProt<Object>> beta = CreateObject<BetaProt<Object>>();

    std::string betaName("ns3::BetaProt<Object>");
    std::string betaClassName(beta->GetInstanceTypeId().GetName());
    std::string betaClassNameDirect(GetTemplateClassName<BetaProt<Object>>());

    if (betaClassName != betaName)
    {
        std::cerr << "Unexpected templated class name (\"" << betaClassName << "\" instead of \""
                  << betaName << "\")";
        return -1;
    }
    if (betaClassNameDirect != betaName)
    {
        std::cerr << "Unexpected templated class name (\"" << betaClassNameDirect
                  << "\" instead of \"" << betaName << "\")";
        return -1;
    }

    return 0;
}
