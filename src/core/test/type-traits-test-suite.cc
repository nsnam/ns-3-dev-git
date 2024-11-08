/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/test.h"
#include "ns3/type-traits.h"
#include "ns3/warnings.h"

/**
 * @file
 * @ingroup core-tests
 * @ingroup object
 * @ingroup object-tests
 * TypeTraits test suite.
 */

namespace ns3
{

namespace tests
{

/**
 * @ingroup object-tests
 *  Type traits test
 */
class TypeTraitsTestCase : public TestCase
{
  public:
    /** Constructor. */
    TypeTraitsTestCase();

    /** Destructor. */
    ~TypeTraitsTestCase() override
    {
    }

  private:
    void DoRun() override;
};

TypeTraitsTestCase::TypeTraitsTestCase()
    : TestCase("Check type traits")
{
}

void
TypeTraitsTestCase::DoRun()
{
    // NS_DEPRECATED_3_43
    // The TypeTraits struct has been deprecated in ns-3.43.
    // While the struct isn't removed, the deprecation warnings must be silenced.
    // Once the struct is removed, this test suite should be deleted.
    NS_WARNING_PUSH_DEPRECATED;

    NS_TEST_ASSERT_MSG_EQ(TypeTraits<void (TypeTraitsTestCase::*)()>::IsPointerToMember,
                          1,
                          "Check pointer to member function ()");
    NS_TEST_ASSERT_MSG_EQ(TypeTraits<void (TypeTraitsTestCase::*)() const>::IsPointerToMember,
                          1,
                          "Check pointer to member function () const");
    NS_TEST_ASSERT_MSG_EQ(TypeTraits<void (TypeTraitsTestCase::*)(int)>::IsPointerToMember,
                          1,
                          "Check pointer to member function (int)");
    NS_TEST_ASSERT_MSG_EQ(TypeTraits<void (TypeTraitsTestCase::*)(int) const>::IsPointerToMember,
                          1,
                          "Check pointer to member function (int) const");
    NS_TEST_ASSERT_MSG_EQ(
        TypeTraits<void (TypeTraitsTestCase::*)() const>::PointerToMemberTraits::nArgs,
        0,
        "Check number of arguments for pointer to member function () const");
    NS_TEST_ASSERT_MSG_EQ(
        TypeTraits<void (TypeTraitsTestCase::*)(int) const>::PointerToMemberTraits::nArgs,
        1,
        "Check number of arguments for pointer to member function (int) const");

    NS_WARNING_POP;
}

/**
 * @ingroup object-tests
 *  Type traits test suite
 */
class TypeTraitsTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    TypeTraitsTestSuite();
};

TypeTraitsTestSuite::TypeTraitsTestSuite()
    : TestSuite("type-traits")
{
    AddTestCase(new TypeTraitsTestCase);
}

/**
 * @ingroup object-tests
 * TypeTraitsTestSuite instance variable.
 */
static TypeTraitsTestSuite g_typeTraitsTestSuite;

} // namespace tests

} // namespace ns3
