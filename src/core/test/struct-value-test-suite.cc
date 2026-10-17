/*
 * Copyright (c) 2026 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/attribute-container.h"
#include "ns3/double.h"
#include "ns3/object.h"
#include "ns3/string.h"
#include "ns3/struct.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

#include <cstdint>
#include <set>
#include <string>
#include <utility>

using namespace ns3;

/** Structure used by the StructValue tests. */
struct StructTestData
{
    std::string name; //!< Name field
    uint16_t count;   //!< Count field
    double value;     //!< Value field

    /**
     * Compare two structures.
     *
     * @return true if all the fields are equal
     */
    bool operator==(const StructTestData&) const = default;
};

/** StructValue type used by the tests. */
using StructTestValue = StructValue<StructTestData,
                                    StructField<StringValue, &StructTestData::name>,
                                    StructField<UintegerValue, &StructTestData::count>,
                                    StructField<DoubleValue, &StructTestData::value>>;

/** Structure with private fields used by the StructValue tests. */
struct PrivateStructTestData
{
  public:
    /** Default constructor. */
    PrivateStructTestData() = default;

    /**
     * Construct an instance with the given field values.
     *
     * @param name The name field.
     * @param count The count field.
     * @param value The value field.
     */
    PrivateStructTestData(std::string name, uint16_t count, double value)
        : m_name(std::move(name)),
          m_count(count),
          m_value(value)
    {
    }

    /**
     * Construct an instance from its field values.
     *
     * @param name The name field.
     * @param count The count field.
     * @param value The value field.
     * @return the constructed instance
     */
    static PrivateStructTestData Create(std::string name, uint16_t count, double value)
    {
        return {std::move(name), count, value};
    }

    /** @return the count field */
    uint16_t GetCount() const
    {
        return m_count;
    }

    /**
     * Set the count field.
     *
     * @param count The count to set.
     */
    void SetCount(uint16_t count)
    {
        m_count = count;
    }

    /** @return the value field */
    double GetValue() const
    {
        return m_value;
    }

    /**
     * Set the value field.
     *
     * @param value The value to set.
     */
    void SetValue(double value)
    {
        m_value = value;
    }

    /**
     * Compare two structures.
     *
     * @return true if all fields are equal
     */
    bool operator==(const PrivateStructTestData&) const = default;

    std::string m_name; //!< Name field (publicly accessible)

  private:
    uint16_t m_count{}; //!< Count field
    double m_value{};   //!< Value field
};

/** StructValue using getters and setters to access private fields. */
using PrivateStructTestValue = StructValue<
    PrivateStructTestData,
    StructField<StringValue, &PrivateStructTestData::m_name>,
    StructField<UintegerValue, &PrivateStructTestData::GetCount, &PrivateStructTestData::SetCount>,
    StructField<DoubleValue, &PrivateStructTestData::GetValue, &PrivateStructTestData::SetValue>>;

/** StructValue using a constructor function and getters for private fields. */
using ConstructedStructTestValue =
    StructValue<PrivateStructTestData,
                StructConstructor<&PrivateStructTestData::Create>,
                StructField<StringValue, &PrivateStructTestData::m_name>,
                StructField<UintegerValue, &PrivateStructTestData::GetCount>,
                StructField<DoubleValue, &PrivateStructTestData::GetValue>>;

/** Object containing attributes represented by structures. */
class StructObject : public Object
{
  public:
    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Set the structure accessed through methods.
     *
     * @param value the structure to set
     */
    void SetMethodStruct(const StructTestData& value);

    /**
     * Get the structure accessed through methods.
     *
     * @return the stored structure
     */
    StructTestData GetMethodStruct() const;

    StructTestData m_memberStruct;         //!< Structure accessed as a data member
    PrivateStructTestData m_privateStruct; //!< Structure whose fields are accessed through methods

  private:
    StructTestData m_methodStruct; //!< Structure accessed through methods
};

TypeId
StructObject::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::StructObject")
            .SetParent<Object>()
            .SetGroupName("Test")
            .AddConstructor<StructObject>()
            .AddAttribute("MemberStruct",
                          "Structure accessed as a data member",
                          StructTestValue(StructTestData{"member", 2, 3.5}),
                          MakeStructAccessor<StructTestValue>(&StructObject::m_memberStruct),
                          MakeStructChecker<StructTestValue>(MakeStringChecker(),
                                                             MakeUintegerChecker<uint16_t>(1, 10),
                                                             MakeDoubleChecker<double>(0.0, 10.0)))
            .AddAttribute(
                "PrivateStruct",
                "Structure whose fields are accessed through getters and setters",
                PrivateStructTestValue(PrivateStructTestData{"private", 3, 4.5}),
                MakeStructAccessor<PrivateStructTestValue>(&StructObject::m_privateStruct),
                MakeStructChecker<PrivateStructTestValue>(MakeStringChecker(),
                                                          MakeUintegerChecker<uint16_t>(1, 10),
                                                          MakeDoubleChecker<double>(0.0, 10.0)))
            .AddAttribute("MethodStruct",
                          "Structure accessed through a getter and a setter",
                          StructTestValue(StructTestData{"method", 4, 5.5}),
                          MakeStructAccessor<StructTestValue>(&StructObject::SetMethodStruct,
                                                              &StructObject::GetMethodStruct),
                          MakeStructChecker<StructTestValue>(MakeStringChecker(),
                                                             MakeUintegerChecker<uint16_t>(1, 10),
                                                             MakeDoubleChecker<double>(0.0, 10.0)));
    return tid;
}

void
StructObject::SetMethodStruct(const StructTestData& value)
{
    m_methodStruct = value;
}

StructTestData
StructObject::GetMethodStruct() const
{
    return m_methodStruct;
}

/** Test StructValue construction, checking, serialization, and access. */
class StructValueTestCase : public TestCase
{
  public:
    /** Constructor. */
    StructValueTestCase();

  private:
    void DoRun() override;
};

StructValueTestCase::StructValueTestCase()
    : TestCase("test StructValue attribute value")
{
}

void
StructValueTestCase::DoRun()
{
    auto object = CreateObject<StructObject>();

    NS_TEST_ASSERT_MSG_EQ(object->m_memberStruct == StructTestData("member", 2, 3.5),
                          true,
                          "The data member did not receive its default value");
    NS_TEST_ASSERT_MSG_EQ(object->GetMethodStruct() == StructTestData("method", 4, 5.5),
                          true,
                          "The setter did not receive its default value");
    NS_TEST_ASSERT_MSG_EQ(object->m_privateStruct == PrivateStructTestData("private", 3, 4.5),
                          true,
                          "The private structure did not receive its default value");

    auto success = object->SetAttributeFailSafe("MemberStruct",
                                                StructTestValue(StructTestData{"updated", 6, 7.5}));
    NS_TEST_ASSERT_MSG_EQ(success, true, "Setting a valid StructValue failed");
    NS_TEST_ASSERT_MSG_EQ(object->m_memberStruct == StructTestData("updated", 6, 7.5),
                          true,
                          "The data member was not updated");

    StructTestValue value;
    success = object->GetAttributeFailSafe("MemberStruct", value);
    NS_TEST_ASSERT_MSG_EQ(success, true, "Getting a StructValue failed");
    NS_TEST_ASSERT_MSG_EQ(value.Get() == StructTestData("updated", 6, 7.5),
                          true,
                          "The StructValue does not contain the data member fields");

    success = object->SetAttributeFailSafe("PrivateStruct", StringValue("{accessors, 7, 8.5}"));
    NS_TEST_ASSERT_MSG_EQ(success, true, "Deserializing private structure fields failed");
    NS_TEST_ASSERT_MSG_EQ(object->m_privateStruct == PrivateStructTestData("accessors", 7, 8.5),
                          true,
                          "The private fields were not updated through their setters");

    PrivateStructTestValue privateValue;
    success = object->GetAttributeFailSafe("PrivateStruct", privateValue);
    NS_TEST_ASSERT_MSG_EQ(success, true, "Getting private structure fields failed");
    NS_TEST_ASSERT_MSG_EQ(privateValue.Get() == PrivateStructTestData("accessors", 7, 8.5),
                          true,
                          "The private fields were not read through their getters");

    auto constructorChecker =
        MakeStructChecker<ConstructedStructTestValue>(MakeStringChecker(),
                                                      MakeUintegerChecker<uint16_t>(1, 10),
                                                      MakeDoubleChecker<double>(0.0, 10.0));
    ConstructedStructTestValue constructedValue;
    success = constructedValue.DeserializeFromString("{constructed, 9, 1.5}", constructorChecker);
    NS_TEST_ASSERT_MSG_EQ(success, true, "Deserializing constructor-based fields failed");
    NS_TEST_ASSERT_MSG_EQ(constructedValue.Get() == PrivateStructTestData("constructed", 9, 1.5),
                          true,
                          "The private fields were not passed to the constructor function");

    success = object->SetAttributeFailSafe("MethodStruct", StringValue("{parsed, 8, 9.5}"));
    NS_TEST_ASSERT_MSG_EQ(success, true, "Deserializing a valid structure failed");
    NS_TEST_ASSERT_MSG_EQ(object->GetMethodStruct() == StructTestData("parsed", 8, 9.5),
                          true,
                          "The deserialized structure was not passed to the setter");

    success = object->SetAttributeFailSafe("MethodStruct",
                                           StructTestValue(StructTestData{"invalid", 11, 2.5}));
    NS_TEST_ASSERT_MSG_EQ(success, false, "A field outside its valid range was accepted");
    NS_TEST_ASSERT_MSG_EQ(object->GetMethodStruct() == StructTestData("parsed", 8, 9.5),
                          true,
                          "The structure changed after a failed assignment");

    success = object->SetAttributeFailSafe("MethodStruct", StringValue("{incomplete}"));
    NS_TEST_ASSERT_MSG_EQ(success, false, "A structure with missing fields was accepted");
    NS_TEST_ASSERT_MSG_EQ(object->GetMethodStruct() == StructTestData("parsed", 8, 9.5),
                          true,
                          "The structure changed after failed deserialization");

    auto checker = MakeStructChecker<StructTestValue>(MakeStringChecker(),
                                                      MakeUintegerChecker<uint16_t>(1, 10),
                                                      MakeDoubleChecker<double>(0.0, 10.0));
    NS_TEST_ASSERT_MSG_EQ(value.SerializeToString(checker),
                          "{updated, 6, 7.5}",
                          "The structure was not serialized as a tuple");

    auto copy = DynamicCast<StructTestValue>(value.Copy());
    NS_TEST_ASSERT_MSG_NE(copy, nullptr, "Copy returned the wrong AttributeValue type");
    NS_TEST_ASSERT_MSG_EQ(copy->Get() == value.Get(),
                          true,
                          "Copy did not preserve the structure fields");

    struct ContainerData
    {
        std::set<uint8_t> ids;

        bool operator==(const ContainerData&) const = default;
    };

    using ContainerValue =
        StructValue<ContainerData,
                    StructField<AttributeContainerValue<UintegerValue, ';'>, &ContainerData::ids>>;
    auto containerChecker = MakeStructChecker<ContainerValue>(
        MakeAttributeContainerChecker<UintegerValue, ';'>(MakeUintegerChecker<uint8_t>()));
    ContainerValue containerValue(ContainerData{{1, 3, 5}});
    NS_TEST_ASSERT_MSG_EQ((containerValue.Get() == ContainerData{{1, 3, 5}}),
                          true,
                          "Container field was not stored from the data member");
    NS_TEST_ASSERT_MSG_EQ(containerValue.SerializeToString(containerChecker),
                          "{1;3;5}",
                          "Container field was not serialized with the item separator");
    NS_TEST_ASSERT_MSG_EQ(containerValue.DeserializeFromString("{2;4}", containerChecker),
                          true,
                          "Deserializing a container field failed");
    NS_TEST_ASSERT_MSG_EQ((containerValue.Get() == ContainerData{{2, 4}}),
                          true,
                          "Container field was not written back to the data member");
}

/** StructValue test suite. */
class StructValueTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    StructValueTestSuite();
};

StructValueTestSuite::StructValueTestSuite()
    : TestSuite("struct-value-test-suite", Type::UNIT)
{
    AddTestCase(new StructValueTestCase(), TestCase::Duration::QUICK);
}

static StructValueTestSuite g_structValueTestSuite; //!< Static test suite registration
