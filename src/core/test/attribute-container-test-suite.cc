/*
 * Copyright (c) 2018 Caliola Engineering, LLC.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jared Dulmage <jared.dulmage@caliola.com>
 */

#include "ns3/attribute-container.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/pair.h"
#include "ns3/ptr.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/tuple.h"
#include "ns3/type-id.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <utility>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AttributeContainerTestSuite");

/**
 * @file
 * @ingroup attribute-tests
 * Attribute container test suite
 */

/**
 * @ingroup attribute-tests
 * Attribute container object.
 */
class AttributeContainerObject : public Object
{
  public:
    AttributeContainerObject();
    ~AttributeContainerObject() override;

    /**
     * Reverses the list of doubles.
     */
    void ReverseDoubleList();

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Set the list of doubles to the given list
     *
     * @param doubleList the given list
     */
    void SetDoubleList(const std::list<double>& doubleList);
    /**
     * Get the list of doubles
     *
     * @return the list of doubles
     */
    std::list<double> GetDoubleList() const;

    /**
     * Set the vector of ints to the given vector
     *
     * @param vec the given vector
     */
    void SetIntVec(std::vector<int> vec);
    /**
     * Get the vector of ints
     *
     * @return the vector of ints
     */
    std::vector<int> GetIntVec() const;

  private:
    std::list<double> m_doublelist; //!< List of doubles.
    std::vector<int> m_intvec;      //!< Vector of ints.
    // TODO(jared): need PairValue attributevalue to handle std::pair elements
    std::map<std::string, int> m_map;                         //!< Map of <std::string, int>.
    std::map<int64_t, std::list<int64_t>> m_intVecIntMapping; //!< Mapping integers to vectors
};

AttributeContainerObject::AttributeContainerObject()
{
}

AttributeContainerObject::~AttributeContainerObject()
{
}

TypeId
AttributeContainerObject::GetTypeId()
{
    using IntVecMapValue = PairValue<IntegerValue, AttributeContainerValue<IntegerValue>>;

    static TypeId tid =
        TypeId("ns3::AttributeContainerObject")
            .SetParent<Object>()
            .SetGroupName("Test")
            .AddConstructor<AttributeContainerObject>()
            .AddAttribute("DoubleList",
                          "List of doubles",
                          AttributeContainerValue<DoubleValue>(),
                          MakeAttributeContainerAccessor<DoubleValue>(
                              &AttributeContainerObject::m_doublelist),
                          MakeAttributeContainerChecker<DoubleValue>(MakeDoubleChecker<double>()))
            .AddAttribute(
                "IntegerVector",
                "Vector of integers",
                // the container value container differs from the underlying object
                AttributeContainerValue<IntegerValue>(),
                // the type of the underlying container cannot be deduced
                MakeAttributeContainerAccessor<IntegerValue, ';', std::list>(
                    &AttributeContainerObject::SetIntVec,
                    &AttributeContainerObject::GetIntVec),
                MakeAttributeContainerChecker<IntegerValue, ';'>(MakeIntegerChecker<int>()))
            .AddAttribute(
                "MapStringInt",
                "Map of strings to ints",
                // the container value container differs from the underlying object
                AttributeContainerValue<PairValue<StringValue, IntegerValue>>(),
                MakeAttributeContainerAccessor<PairValue<StringValue, IntegerValue>>(
                    &AttributeContainerObject::m_map),
                MakeAttributeContainerChecker<PairValue<StringValue, IntegerValue>>(
                    MakePairChecker<StringValue, IntegerValue>(MakeStringChecker(),
                                                               MakeIntegerChecker<int>())))
            .AddAttribute(
                "IntVecPairVec",
                "An example of complex attribute that is defined by a vector of pairs consisting "
                "of an integer value and a vector of integers. In case a string is used to set "
                "this attribute, the string shall contain the pairs separated by a semicolon (;); "
                "in every pair, the integer value and the vector of integers are separated by a "
                "blank space, and the elements of the vectors are separated by a comma (,) "
                "without spaces. E.g. \"0 1,2,3; 1 0; 2 0,1\" consists of three pairs containing "
                "vectors of 3, 1 and 2 elements, respectively.",
                StringValue(""),
                MakeAttributeContainerAccessor<IntVecMapValue, ';'>(
                    &AttributeContainerObject::m_intVecIntMapping),
                MakeAttributeContainerChecker<IntVecMapValue, ';'>(
                    MakePairChecker<IntegerValue, AttributeContainerValue<IntegerValue>>(
                        MakeIntegerChecker<int>(),
                        MakeAttributeContainerChecker<IntegerValue>(MakeIntegerChecker<int>()))));
    return tid;
}

void
AttributeContainerObject::ReverseDoubleList()
{
    m_doublelist.reverse();
}

void
AttributeContainerObject::SetDoubleList(const std::list<double>& doubleList)
{
    m_doublelist = doubleList;
}

std::list<double>
AttributeContainerObject::GetDoubleList() const
{
    return m_doublelist;
}

void
AttributeContainerObject::SetIntVec(std::vector<int> vec)
{
    m_intvec = vec;
}

std::vector<int>
AttributeContainerObject::GetIntVec() const
{
    return m_intvec;
}

/**
 * @ingroup attribute-tests
 *
 * Test AttributeContainer instantiation, initialization, access
 */
class AttributeContainerTestCase : public TestCase
{
  public:
    AttributeContainerTestCase();

    ~AttributeContainerTestCase() override
    {
    }

  private:
    void DoRun() override;
};

AttributeContainerTestCase::AttributeContainerTestCase()
    : TestCase("test instantiation, initialization, access")
{
}

void
AttributeContainerTestCase::DoRun()
{
    {
        std::list<double> ref = {1.0, 2.1, 3.145269};

        AttributeContainerValue<DoubleValue> ac(ref);

        NS_TEST_ASSERT_MSG_EQ(ref.size(), ac.GetN(), "Container size mismatch");
        auto aciter = ac.Begin();
        for (auto rend = ref.end(), riter = ref.begin(); riter != rend; ++riter)
        {
            NS_TEST_ASSERT_MSG_NE(true, (aciter == ac.End()), "AC iterator reached end");
            NS_TEST_ASSERT_MSG_EQ(*riter, (*aciter)->Get(), "Incorrect value");
            ++aciter;
        }
        NS_TEST_ASSERT_MSG_EQ(true, (aciter == ac.End()), "AC iterator did not reach end");
    }

    {
        std::vector<int> ref = {-2, 3, 10, -1042};

        AttributeContainerValue<IntegerValue> ac(ref.begin(), ref.end());

        NS_TEST_ASSERT_MSG_EQ(ref.size(), ac.GetN(), "Container size mismatch");
        auto aciter = ac.Begin();
        for (auto rend = ref.end(), riter = ref.begin(); riter != rend; ++riter)
        {
            NS_TEST_ASSERT_MSG_NE(true, (aciter == ac.End()), "AC iterator reached end");
            NS_TEST_ASSERT_MSG_EQ(*riter, (*aciter)->Get(), "Incorrect value");
            ++aciter;
        }
        NS_TEST_ASSERT_MSG_EQ(true, (aciter == ac.End()), "AC iterator did not reach end");
    }

    {
        auto ref = {"one", "two", "three"};
        AttributeContainerValue<StringValue> ac(ref.begin(), ref.end());

        NS_TEST_ASSERT_MSG_EQ(ref.size(), ac.GetN(), "Container size mismatch");
        auto aciter = ac.Begin();
        for (auto v : ref)
        {
            NS_TEST_ASSERT_MSG_NE(true, (aciter == ac.End()), "AC iterator reached end");
            NS_TEST_ASSERT_MSG_EQ(v, (*aciter)->Get(), "Incorrect value");
            ++aciter;
        }
        NS_TEST_ASSERT_MSG_EQ(true, (aciter == ac.End()), "AC iterator did not reach end");
    }

    {
        auto ref = {"one", "two", "three"};
        AttributeContainerValue<StringValue, ',', std::vector> ac(ref);

        NS_TEST_ASSERT_MSG_EQ(ref.size(), ac.GetN(), "Container size mismatch");
        auto aciter = ac.Begin();
        for (auto v : ref)
        {
            NS_TEST_ASSERT_MSG_NE(true, (aciter == ac.End()), "AC iterator reached end");
            NS_TEST_ASSERT_MSG_EQ(v, (*aciter)->Get(), "Incorrect value");
            ++aciter;
        }
        NS_TEST_ASSERT_MSG_EQ(true, (aciter == ac.End()), "AC iterator did not reach end");
    }

    {
        // use int64_t which is default for IntegerValue
        std::map<std::string, int64_t> ref = {{"one", 1}, {"two", 2}, {"three", 3}};
        AttributeContainerValue<PairValue<StringValue, IntegerValue>> ac(ref);

        NS_TEST_ASSERT_MSG_EQ(ref.size(), ac.GetN(), "Container size mismatch");
        auto aciter = ac.Begin();
        for (const auto& v : ref)
        {
            NS_TEST_ASSERT_MSG_NE(true, (aciter == ac.End()), "AC iterator reached end");
            bool valCheck =
                v.first == (*aciter)->Get().first && v.second == (*aciter)->Get().second;
            NS_TEST_ASSERT_MSG_EQ(valCheck, true, "Incorrect value");
            ++aciter;
        }
        NS_TEST_ASSERT_MSG_EQ(true, (aciter == ac.End()), "AC iterator did not reach end");
    }
}

/**
 * @ingroup attribute-tests
 *
 * Attribute serialization and deserialization TestCase.
 */
class AttributeContainerSerializationTestCase : public TestCase
{
  public:
    AttributeContainerSerializationTestCase();

    ~AttributeContainerSerializationTestCase() override
    {
    }

  private:
    void DoRun() override;
};

AttributeContainerSerializationTestCase::AttributeContainerSerializationTestCase()
    : TestCase("test serialization and deserialization")
{
}

void
AttributeContainerSerializationTestCase::DoRun()
{
    {
        // notice embedded spaces
        std::string doubles = "1.0001, 20.53, -102.3";

        AttributeContainerValue<DoubleValue> attr;
        auto checker = MakeAttributeContainerChecker(attr);
        auto acchecker = DynamicCast<AttributeContainerChecker>(checker);
        acchecker->SetItemChecker(MakeDoubleChecker<double>());
        NS_TEST_ASSERT_MSG_EQ(attr.DeserializeFromString(doubles, checker),
                              true,
                              "Deserialize failed");
        NS_TEST_ASSERT_MSG_EQ(attr.GetN(), 3, "Incorrect container size");

        std::string reserialized = attr.SerializeToString(checker);
        std::string canonical = doubles;
        canonical.erase(std::remove(canonical.begin(), canonical.end(), ' '), canonical.end());
        NS_TEST_ASSERT_MSG_EQ(reserialized, canonical, "Reserialization failed");
    }

    {
        // notice embedded spaces
        std::string ints = "1, 2, -3, -4";

        AttributeContainerValue<IntegerValue> attr;
        auto checker = MakeAttributeContainerChecker(attr);
        auto acchecker = DynamicCast<AttributeContainerChecker>(checker);
        acchecker->SetItemChecker(MakeIntegerChecker<int>());
        NS_TEST_ASSERT_MSG_EQ(attr.DeserializeFromString(ints, checker),
                              true,
                              "Deserialize failed");
        NS_TEST_ASSERT_MSG_EQ(attr.GetN(), 4, "Incorrect container size");

        std::string reserialized = attr.SerializeToString(checker);
        std::string canonical = ints;
        canonical.erase(std::remove(canonical.begin(), canonical.end(), ' '), canonical.end());
        NS_TEST_ASSERT_MSG_EQ(reserialized, canonical, "Reserialization failed");
    }

    {
        std::string strings = "this is a sentence with words";

        AttributeContainerValue<StringValue, ' '> attr;
        auto checker = MakeAttributeContainerChecker(attr);
        auto acchecker = DynamicCast<AttributeContainerChecker>(checker);
        acchecker->SetItemChecker(MakeStringChecker());
        NS_TEST_ASSERT_MSG_EQ(attr.DeserializeFromString(strings, checker),
                              true,
                              "Deserialize failed");
        NS_TEST_ASSERT_MSG_EQ(attr.GetN(), 6, "Incorrect container size");

        std::string reserialized = attr.SerializeToString(checker);
        NS_TEST_ASSERT_MSG_EQ(reserialized, strings, "Reserialization failed");
    }

    {
        std::string pairs = "one 1,two 2,three 3";
        AttributeContainerValue<PairValue<StringValue, IntegerValue>> attr;
        auto checker = MakeAttributeContainerChecker(attr);
        auto acchecker = DynamicCast<AttributeContainerChecker>(checker);
        acchecker->SetItemChecker(
            MakePairChecker<StringValue, IntegerValue>(MakeStringChecker(),
                                                       MakeIntegerChecker<int>()));
        NS_TEST_ASSERT_MSG_EQ(attr.DeserializeFromString(pairs, checker),
                              true,
                              "Deserialization failed");
        NS_TEST_ASSERT_MSG_EQ(attr.GetN(), 3, "Incorrect container size");

        std::string reserialized = attr.SerializeToString(checker);
        NS_TEST_ASSERT_MSG_EQ(reserialized, pairs, "Reserialization failed");
    }

    {
        std::string tupleVec = "{-1, 2, 3.4};{-2, 1, 4.3}";
        AttributeContainerValue<TupleValue<IntegerValue, UintegerValue, DoubleValue>, ';'> attr;
        auto checker = MakeAttributeContainerChecker(attr);
        auto acchecker = DynamicCast<AttributeContainerChecker>(checker);
        acchecker->SetItemChecker(MakeTupleChecker<IntegerValue, UintegerValue, DoubleValue>(
            MakeIntegerChecker<int8_t>(),
            MakeUintegerChecker<uint16_t>(),
            MakeDoubleChecker<double>()));
        NS_TEST_ASSERT_MSG_EQ(attr.DeserializeFromString(tupleVec, checker),
                              true,
                              "Deserialization failed");
        NS_TEST_ASSERT_MSG_EQ(attr.GetN(), 2, "Incorrect container size");

        std::string reserialized = attr.SerializeToString(checker);
        NS_TEST_ASSERT_MSG_EQ(reserialized, tupleVec, "Reserialization failed");
    }
}

/**
 * @ingroup attribute-tests
 *
 * Attribute set and get TestCase.
 */
class AttributeContainerSetGetTestCase : public TestCase
{
  public:
    AttributeContainerSetGetTestCase();

    ~AttributeContainerSetGetTestCase() override
    {
    }

  private:
    void DoRun() override;
};

AttributeContainerSetGetTestCase::AttributeContainerSetGetTestCase()
    : TestCase("test attribute set and get")
{
}

void
AttributeContainerSetGetTestCase::DoRun()
{
    Ptr<AttributeContainerObject> obj = CreateObject<AttributeContainerObject>();
    {
        auto doubleList = obj->GetDoubleList();
        NS_TEST_ASSERT_MSG_EQ(doubleList.empty(), true, "DoubleList initialized incorrectly");
    }

    const std::list<double> doubles = {1.1, 2.22, 3.333};
    obj->SetAttribute("DoubleList", AttributeContainerValue<DoubleValue>(doubles));
    {
        auto doubleList = obj->GetDoubleList();
        NS_TEST_ASSERT_MSG_EQ(std::equal(doubles.begin(), doubles.end(), doubleList.begin()),
                              true,
                              "DoubleList incorrectly set");
    }

    obj->ReverseDoubleList();
    {
        auto doubleList = obj->GetDoubleList();
        NS_TEST_ASSERT_MSG_EQ(std::equal(doubles.rbegin(), doubles.rend(), doubleList.begin()),
                              true,
                              "DoubleList incorrectly reversed");

        // NOTE: changing the return container here too!
        AttributeContainerValue<DoubleValue> value;
        obj->GetAttribute("DoubleList", value);
        NS_TEST_ASSERT_MSG_EQ(doubles.size(), value.GetN(), "AttributeContainerValue wrong size");

        AttributeContainerValue<DoubleValue>::result_type doublevec = value.Get();
        NS_TEST_ASSERT_MSG_EQ(doubles.size(), doublevec.size(), "DoublesVec wrong size");
        NS_TEST_ASSERT_MSG_EQ(std::equal(doubles.rbegin(), doubles.rend(), doublevec.begin()),
                              true,
                              "Incorrect value in doublesvec");
    }

    const std::vector<int> ints = {-1, 0, 1, 2, 3};
    // NOTE: here the underlying attribute container type differs from the actual container
    obj->SetAttribute("IntegerVector", AttributeContainerValue<IntegerValue, ';'>(ints));

    {
        // NOTE: changing the container here too!
        AttributeContainerValue<IntegerValue, ';'> value;
        obj->GetAttribute("IntegerVector", value);
        NS_TEST_ASSERT_MSG_EQ(ints.size(), value.GetN(), "AttributeContainerValue wrong size");

        AttributeContainerValue<IntegerValue>::result_type intlist = value.Get();
        NS_TEST_ASSERT_MSG_EQ(ints.size(), intlist.size(), "Intvec wrong size");

        NS_TEST_ASSERT_MSG_EQ(std::equal(ints.begin(), ints.end(), intlist.begin()),
                              true,
                              "Incorrect value in intvec");
    }

    std::string intVecPairString("0 1,2,3; 1 0; 2 0,1");
    // NOTE: here the underlying attribute container type differs from the actual container
    obj->SetAttribute("IntVecPairVec", StringValue(intVecPairString));

    {
        using IntVecMapValue = PairValue<IntegerValue, AttributeContainerValue<IntegerValue>>;

        // NOTE: changing the container here too!
        AttributeContainerValue<IntVecMapValue, ';'> value;
        obj->GetAttribute("IntVecPairVec", value);
        NS_TEST_ASSERT_MSG_EQ(3, value.GetN(), "AttributeContainerValue wrong size"); // 3 pairs

        AttributeContainerValue<IntVecMapValue>::result_type reslist = value.Get();
        NS_TEST_ASSERT_MSG_EQ(3, reslist.size(), "IntVecMapValue wrong size");
        auto reslistIt = reslist.begin();
        NS_TEST_ASSERT_MSG_EQ(reslistIt->first, 0, "Incorrect integer value in first pair");
        NS_TEST_ASSERT_MSG_EQ(reslistIt->second.size(),
                              3,
                              "Incorrect number of integer values in first pair");
        ++reslistIt;
        NS_TEST_ASSERT_MSG_EQ(reslistIt->first, 1, "Incorrect integer value in second pair");
        NS_TEST_ASSERT_MSG_EQ(reslistIt->second.size(),
                              1,
                              "Incorrect number of integer values in second pair");
        ++reslistIt;
        NS_TEST_ASSERT_MSG_EQ(reslistIt->first, 2, "Incorrect integer value in third pair");
        NS_TEST_ASSERT_MSG_EQ(reslistIt->second.size(),
                              2,
                              "Incorrect number of integer values in third pair");
    }

    std::map<std::string, int> map = {{"one", 1}, {"two", 2}, {"three", 3}};
    obj->SetAttribute("MapStringInt",
                      AttributeContainerValue<PairValue<StringValue, IntegerValue>>(map));

    {
        AttributeContainerValue<PairValue<StringValue, IntegerValue>> value;
        obj->GetAttribute("MapStringInt", value);
        NS_TEST_ASSERT_MSG_EQ(map.size(), value.GetN(), "AttributeContainerValue wrong size");

        // could possibly make custom assignment operator to make assignment statement work
        std::map<std::string, int> mapstrint;
        auto lst = value.Get();
        for (const auto& l : lst)
        {
            mapstrint[l.first] = l.second;
        }

        NS_TEST_ASSERT_MSG_EQ(map.size(), mapstrint.size(), "mapstrint wrong size");
        auto iter = map.begin();
        for (const auto& v : mapstrint)
        {
            bool valCheck = v.first == (*iter).first && v.second == (*iter).second;
            NS_TEST_ASSERT_MSG_EQ(valCheck, true, "Incorrect value in mapstrint");
            ++iter;
        }
    }
}

/**
 * @ingroup attribute-tests
 *
 * Attribute attribute container TestCase.
 */
class AttributeContainerTestSuite : public TestSuite
{
  public:
    AttributeContainerTestSuite();
};

AttributeContainerTestSuite::AttributeContainerTestSuite()
    : TestSuite("attribute-container-test-suite", Type::UNIT)
{
    AddTestCase(new AttributeContainerTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new AttributeContainerSerializationTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new AttributeContainerSetGetTestCase(), TestCase::Duration::QUICK);
}

static AttributeContainerTestSuite
    g_attributeContainerTestSuite; //!< Static variable for test initialization
