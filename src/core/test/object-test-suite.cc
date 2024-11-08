/*
 * Copyright (c) 2007 INRIA, Gustavo Carneiro
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Gustavo Carneiro <gjcarneiro@gmail.com>,
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ns3/assert.h"
#include "ns3/object-factory.h"
#include "ns3/object.h"
#include "ns3/test.h"

/**
 * @file
 * @ingroup core-tests
 * @ingroup object
 * @ingroup object-tests
 * Object test suite.
 */

/**
 * @ingroup core-tests
 * @defgroup object-tests Object test suite
 */

namespace
{

/**
 * @ingroup object-tests
 * Base class A.
 */
class BaseA : public ns3::Object
{
  public:
    /**
     * Register this type.
     * @return The TypeId.
     */
    static ns3::TypeId GetTypeId()
    {
        static ns3::TypeId tid = ns3::TypeId("ObjectTest:BaseA")
                                     .SetParent<Object>()
                                     .SetGroupName("Core")
                                     .HideFromDocumentation()
                                     .AddConstructor<BaseA>();
        return tid;
    }

    /** Constructor. */
    BaseA()
    {
    }
};

/**
 * @ingroup object-tests
 * Derived class A.
 */
class DerivedA : public BaseA
{
  public:
    /**
     * Register this type.
     * @return The TypeId.
     */
    static ns3::TypeId GetTypeId()
    {
        static ns3::TypeId tid = ns3::TypeId("ObjectTest:DerivedA")
                                     .SetParent<BaseA>()
                                     .SetGroupName("Core")
                                     .HideFromDocumentation()
                                     .AddConstructor<DerivedA>();
        return tid;
    }

    /** Constructor. */
    DerivedA()
    {
    }

  protected:
    void DoDispose() override
    {
        BaseA::DoDispose();
    }
};

/**
 * @ingroup object-tests
 * Base class B.
 */
class BaseB : public ns3::Object
{
  public:
    /**
     * Register this type.
     * @return The TypeId.
     */
    static ns3::TypeId GetTypeId()
    {
        static ns3::TypeId tid = ns3::TypeId("ObjectTest:BaseB")
                                     .SetParent<Object>()
                                     .SetGroupName("Core")
                                     .HideFromDocumentation()
                                     .AddConstructor<BaseB>();
        return tid;
    }

    /** Constructor. */
    BaseB()
    {
    }
};

/**
 * @ingroup object-tests
 * Derived class B.
 */
class DerivedB : public BaseB
{
  public:
    /**
     * Register this type.
     * @return The TypeId.
     */
    static ns3::TypeId GetTypeId()
    {
        static ns3::TypeId tid = ns3::TypeId("ObjectTest:DerivedB")
                                     .SetParent<BaseB>()
                                     .SetGroupName("Core")
                                     .HideFromDocumentation()
                                     .AddConstructor<DerivedB>();
        return tid;
    }

    /** Constructor. */
    DerivedB()
    {
    }

  protected:
    void DoDispose() override
    {
        BaseB::DoDispose();
    }
};

NS_OBJECT_ENSURE_REGISTERED(BaseA);
NS_OBJECT_ENSURE_REGISTERED(DerivedA);
NS_OBJECT_ENSURE_REGISTERED(BaseB);
NS_OBJECT_ENSURE_REGISTERED(DerivedB);

} // unnamed namespace

namespace ns3
{

namespace tests
{

/**
 * @ingroup object-tests
 * Test we can make Objects using CreateObject.
 */
class CreateObjectTestCase : public TestCase
{
  public:
    /** Constructor. */
    CreateObjectTestCase();
    /** Destructor. */
    ~CreateObjectTestCase() override;

  private:
    void DoRun() override;
};

CreateObjectTestCase::CreateObjectTestCase()
    : TestCase("Check CreateObject<Type> template function")
{
}

CreateObjectTestCase::~CreateObjectTestCase()
{
}

void
CreateObjectTestCase::DoRun()
{
    Ptr<BaseA> baseA = CreateObject<BaseA>();
    NS_TEST_ASSERT_MSG_NE(baseA, nullptr, "Unable to CreateObject<BaseA>");

    //
    // Since baseA is a BaseA, we must be able to successfully ask for a BaseA.
    //
    NS_TEST_ASSERT_MSG_EQ(baseA->GetObject<BaseA>(),
                          baseA,
                          "GetObject() of same type returns different Ptr");

    //
    // Since BaseA is a BaseA and not a DerivedA, we must not find a DerivedA if we look.
    //
    NS_TEST_ASSERT_MSG_NE(!baseA->GetObject<DerivedA>(),
                          0,
                          "GetObject() of unrelated type returns nonzero pointer");

    //
    // Since baseA is not a BaseA, we must not be able to ask for a DerivedA even if we
    // try an implied cast back to a BaseA.
    //
    NS_TEST_ASSERT_MSG_NE(!baseA->GetObject<BaseA>(DerivedA::GetTypeId()),
                          0,
                          "GetObject() of unrelated returns nonzero Ptr");

    baseA = CreateObject<DerivedA>();
    NS_TEST_ASSERT_MSG_NE(baseA,
                          nullptr,
                          "Unable to CreateObject<DerivedA> with implicit cast to BaseA");

    //
    // If we create a DerivedA and cast it to a BaseA, then if we do a GetObject for
    // that BaseA we should get the same address (same Object).
    //
    NS_TEST_ASSERT_MSG_EQ(baseA->GetObject<BaseA>(), baseA, "Unable to GetObject<BaseA> on BaseA");

    //
    // Since we created a DerivedA and cast it to a BaseA, we should be able to
    // get back a DerivedA and it should be the original Ptr.
    //
    NS_TEST_ASSERT_MSG_EQ(baseA->GetObject<DerivedA>(),
                          baseA,
                          "GetObject() of the original type returns different Ptr");

    // If we created a DerivedA and cast it to a BaseA, then we GetObject for the
    // same DerivedA and cast it back to the same BaseA, we should get the same
    // object.
    //
    NS_TEST_ASSERT_MSG_EQ(baseA->GetObject<BaseA>(DerivedA::GetTypeId()),
                          baseA,
                          "GetObject returns different Ptr");
}

/**
 * @ingroup object-tests
 * Test we can aggregate Objects.
 */
class AggregateObjectTestCase : public TestCase
{
  public:
    /** Constructor. */
    AggregateObjectTestCase();
    /** Destructor. */
    ~AggregateObjectTestCase() override;

  private:
    void DoRun() override;
};

AggregateObjectTestCase::AggregateObjectTestCase()
    : TestCase("Check Object aggregation functionality")
{
}

AggregateObjectTestCase::~AggregateObjectTestCase()
{
}

void
AggregateObjectTestCase::DoRun()
{
    Ptr<BaseA> baseA = CreateObject<BaseA>();
    NS_TEST_ASSERT_MSG_NE(baseA, nullptr, "Unable to CreateObject<BaseA>");

    Ptr<BaseB> baseB = CreateObject<BaseB>();
    NS_TEST_ASSERT_MSG_NE(baseB, nullptr, "Unable to CreateObject<BaseB>");

    Ptr<BaseB> baseBCopy = baseB;
    NS_TEST_ASSERT_MSG_NE(baseBCopy, nullptr, "Unable to copy BaseB");

    //
    // Make an aggregation of a BaseA object and a BaseB object.
    //
    baseA->AggregateObject(baseB);

    //
    // We should be able to ask the aggregation (through baseA) for the BaseA part
    // of the aggregation.
    //
    NS_TEST_ASSERT_MSG_NE(baseA->GetObject<BaseA>(),
                          nullptr,
                          "Cannot GetObject (through baseA) for BaseA Object");

    //
    // There is no DerivedA in this picture, so we should not be able to GetObject
    // for that type.
    //
    NS_TEST_ASSERT_MSG_NE(!baseA->GetObject<DerivedA>(),
                          0,
                          "Unexpectedly found a DerivedA through baseA");

    //
    // We should be able to ask the aggregation (through baseA) for the BaseB part
    //
    NS_TEST_ASSERT_MSG_NE(baseA->GetObject<BaseB>(),
                          nullptr,
                          "Cannot GetObject (through baseA) for BaseB Object");

    //
    // There is no DerivedB in this picture, so we should not be able to GetObject
    // for that type.
    //
    NS_TEST_ASSERT_MSG_NE(!baseA->GetObject<DerivedB>(),
                          0,
                          "Unexpectedly found a DerivedB through baseA");

    //
    // We should be able to ask the aggregation (through baseA) for the BaseB part
    //
    NS_TEST_ASSERT_MSG_NE(baseB->GetObject<BaseB>(),
                          nullptr,
                          "Cannot GetObject (through baseB) for BaseB Object");

    //
    // There is no DerivedB in this picture, so we should not be able to GetObject
    // for that type.
    //
    NS_TEST_ASSERT_MSG_NE(!baseB->GetObject<DerivedB>(),
                          0,
                          "Unexpectedly found a DerivedB through baseB");

    //
    // We should be able to ask the aggregation (through baseB) for the BaseA part
    // of the aggregation.
    //
    NS_TEST_ASSERT_MSG_NE(baseB->GetObject<BaseA>(),
                          nullptr,
                          "Cannot GetObject (through baseB) for BaseA Object");

    //
    // There is no DerivedA in this picture, so we should not be able to GetObject
    // for that type.
    //
    NS_TEST_ASSERT_MSG_NE(!baseB->GetObject<DerivedA>(),
                          0,
                          "Unexpectedly found a DerivedA through baseB");

    //
    // baseBCopy is a copy of the original Ptr to the Object BaseB.  Even though
    // we didn't use baseBCopy directly in the aggregations, the object to which
    // it points was used, therefore, we should be able to use baseBCopy as if
    // it were baseB and get a BaseA out of the aggregation.
    //
    NS_TEST_ASSERT_MSG_NE(baseBCopy->GetObject<BaseA>(),
                          nullptr,
                          "Cannot GetObject (through baseBCopy) for a BaseA Object");

    //
    // Now, change the underlying type of the objects to be the derived types.
    //
    baseA = CreateObject<DerivedA>();
    NS_TEST_ASSERT_MSG_NE(baseA,
                          nullptr,
                          "Unable to CreateObject<DerivedA> with implicit cast to BaseA");

    baseB = CreateObject<DerivedB>();
    NS_TEST_ASSERT_MSG_NE(baseB,
                          nullptr,
                          "Unable to CreateObject<DerivedB> with implicit cast to BaseB");

    //
    // Create an aggregation of two objects, both of the derived types; and leave
    // an unaggregated copy of one lying around.
    //
    baseBCopy = baseB;
    baseA->AggregateObject(baseB);

    //
    // We should be able to ask the aggregation (through baseA) for the DerivedB part
    //
    NS_TEST_ASSERT_MSG_NE(baseA->GetObject<DerivedB>(),
                          nullptr,
                          "Cannot GetObject (through baseA) for DerivedB Object");

    //
    // Since the DerivedB is also a BaseB, we should be able to ask the aggregation
    // (through baseA) for the BaseB part
    //
    NS_TEST_ASSERT_MSG_NE(baseA->GetObject<BaseB>(),
                          nullptr,
                          "Cannot GetObject (through baseA) for BaseB Object");

    //
    // We should be able to ask the aggregation (through baseB) for the DerivedA part
    //
    NS_TEST_ASSERT_MSG_NE(baseB->GetObject<DerivedA>(),
                          nullptr,
                          "Cannot GetObject (through baseB) for DerivedA Object");

    //
    // Since the DerivedA is also a BaseA, we should be able to ask the aggregation
    // (through baseB) for the BaseA part
    //
    NS_TEST_ASSERT_MSG_NE(baseB->GetObject<BaseA>(),
                          nullptr,
                          "Cannot GetObject (through baseB) for BaseA Object");

    //
    // baseBCopy is a copy of the original Ptr to the Object BaseB.  Even though
    // we didn't use baseBCopy directly in the aggregations, the object to which
    // it points was used, therefore, we should be able to use baseBCopy as if
    // it were baseB (same underlying Object) and get a BaseA and a DerivedA out
    // of the aggregation through baseBCopy.
    //
    NS_TEST_ASSERT_MSG_NE(baseBCopy->GetObject<BaseA>(),
                          nullptr,
                          "Cannot GetObject (through baseBCopy) for a BaseA Object");
    NS_TEST_ASSERT_MSG_NE(baseBCopy->GetObject<DerivedA>(),
                          nullptr,
                          "Cannot GetObject (through baseBCopy) for a BaseA Object");

    //
    // Since the Ptr<BaseB> is actually a DerivedB, we should be able to ask the
    // aggregation (through baseB) for the DerivedB part
    //
    NS_TEST_ASSERT_MSG_NE(baseB->GetObject<DerivedB>(),
                          nullptr,
                          "Cannot GetObject (through baseB) for DerivedB Object");

    //
    // Since the DerivedB was cast to a BaseB, we should be able to ask the
    // aggregation (through baseB) for the BaseB part
    //
    NS_TEST_ASSERT_MSG_NE(baseB->GetObject<BaseB>(),
                          nullptr,
                          "Cannot GetObject (through baseB) for BaseB Object");

    //
    // Make sure reference counting works in the aggregate.  Create two Objects
    // and aggregate them, then release one of them.  The aggregation should
    // keep a reference to both and the Object we released should still be there.
    //
    baseA = CreateObject<BaseA>();
    NS_TEST_ASSERT_MSG_NE(baseA, nullptr, "Unable to CreateObject<BaseA>");

    baseB = CreateObject<BaseB>();
    NS_TEST_ASSERT_MSG_NE(baseB, nullptr, "Unable to CreateObject<BaseA>");

    baseA->AggregateObject(baseB);
    baseA = nullptr;

    baseA = baseB->GetObject<BaseA>();
    NS_TEST_ASSERT_MSG_NE(baseA, nullptr, "Unable to GetObject on released object");
}

/**
 * @ingroup object-tests
 * Test we can aggregate Objects.
 */
class UnidirectionalAggregateObjectTestCase : public TestCase
{
  public:
    /** Constructor. */
    UnidirectionalAggregateObjectTestCase();
    /** Destructor. */
    ~UnidirectionalAggregateObjectTestCase() override;

  private:
    void DoRun() override;
};

UnidirectionalAggregateObjectTestCase::UnidirectionalAggregateObjectTestCase()
    : TestCase("Check Object unidirectional aggregation functionality")
{
}

UnidirectionalAggregateObjectTestCase::~UnidirectionalAggregateObjectTestCase()
{
}

void
UnidirectionalAggregateObjectTestCase::DoRun()
{
    Ptr<BaseA> baseAOne = CreateObject<BaseA>();
    NS_TEST_ASSERT_MSG_NE(baseAOne, nullptr, "Unable to CreateObject<BaseA>");
    Ptr<BaseA> baseATwo = CreateObject<BaseA>();
    NS_TEST_ASSERT_MSG_NE(baseATwo, nullptr, "Unable to CreateObject<BaseA>");

    Ptr<BaseB> baseB = CreateObject<BaseB>();
    NS_TEST_ASSERT_MSG_NE(baseB, nullptr, "Unable to CreateObject<BaseB>");

    //
    // Make an unidirectional aggregation of a BaseA object and a BaseB object.
    //
    baseAOne->UnidirectionalAggregateObject(baseB);
    baseATwo->UnidirectionalAggregateObject(baseB);

    //
    // We should be able to ask the aggregation (through baseA) for the BaseB part
    // on either BaseA objects
    //
    NS_TEST_ASSERT_MSG_NE(baseAOne->GetObject<BaseB>(),
                          nullptr,
                          "Cannot GetObject (through baseAOne) for BaseB Object");

    NS_TEST_ASSERT_MSG_NE(baseATwo->GetObject<BaseB>(),
                          nullptr,
                          "Cannot GetObject (through baseATwo) for BaseB Object");

    NS_TEST_ASSERT_MSG_EQ(
        baseAOne->GetObject<BaseB>(),
        baseATwo->GetObject<BaseB>(),
        "GetObject (through baseAOne and baseATwo) for BaseB Object are not equal");

    //
    // We should not be able to ask the aggregation (through baseB) for the BaseA part
    // of the aggregation.
    //
    NS_TEST_ASSERT_MSG_NE(!baseB->GetObject<BaseA>(),
                          0,
                          "Can GetObject (through baseB) for BaseA Object");
}

/**
 * @ingroup object-tests
 * Test an Object factory can create Objects
 */
class ObjectFactoryTestCase : public TestCase
{
  public:
    /** Constructor. */
    ObjectFactoryTestCase();
    /** Destructor. */
    ~ObjectFactoryTestCase() override;

  private:
    void DoRun() override;
};

ObjectFactoryTestCase::ObjectFactoryTestCase()
    : TestCase("Check ObjectFactory functionality")
{
}

ObjectFactoryTestCase::~ObjectFactoryTestCase()
{
}

void
ObjectFactoryTestCase::DoRun()
{
    ObjectFactory factory;

    //
    // Create an Object of type BaseA through an object factory.
    //
    factory.SetTypeId(BaseA::GetTypeId());
    Ptr<Object> a = factory.Create();
    NS_TEST_ASSERT_MSG_EQ(!a, 0, "Unable to factory.Create() a BaseA");

    //
    // What we made should be a BaseA, not have anything to do with a DerivedA
    //
    NS_TEST_ASSERT_MSG_NE(!a->GetObject<BaseA>(DerivedA::GetTypeId()),
                          0,
                          "BaseA is unexpectedly a DerivedA also");

    //
    // The BaseA we got should not respond to a GetObject for DerivedA
    //
    NS_TEST_ASSERT_MSG_NE(!a->GetObject<DerivedA>(),
                          0,
                          "BaseA unexpectedly responds to GetObject for DerivedA");

    //
    // Now tell the factory to make DerivedA Objects and create one with an
    // implied cast back to a BaseA
    //
    factory.SetTypeId(DerivedA::GetTypeId());
    a = factory.Create();

    //
    // Since the DerivedA has a BaseA part, we should be able to use GetObject to
    // dynamically cast back to a BaseA.
    //
    NS_TEST_ASSERT_MSG_EQ(a->GetObject<BaseA>(),
                          a,
                          "Unable to use GetObject as dynamic_cast<BaseA>()");

    //
    // Since a is already a BaseA and is really a DerivedA, we should be able to
    // GetObject for the DerivedA and cast it back to a BaseA getting the same
    // value that is there.
    //
    NS_TEST_ASSERT_MSG_EQ(a->GetObject<BaseA>(DerivedA::GetTypeId()),
                          a,
                          "GetObject with implied cast returns different Ptr");

    //
    // Since a declared a BaseA, even if it is really a DerivedA, we should not
    // be able to GetObject for a DerivedA since this would break the type
    // declaration.
    //
    NS_TEST_ASSERT_MSG_EQ(!a->GetObject<DerivedA>(),
                          0,
                          "Unexpectedly able to work around C++ type system");
}

/**
 * @ingroup object-tests
 * The Test Suite that glues the Test Cases together.
 */
class ObjectTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    ObjectTestSuite();
};

ObjectTestSuite::ObjectTestSuite()
    : TestSuite("object")
{
    AddTestCase(new CreateObjectTestCase);
    AddTestCase(new AggregateObjectTestCase);
    AddTestCase(new UnidirectionalAggregateObjectTestCase);
    AddTestCase(new ObjectFactoryTestCase);
}

/**
 * @ingroup object-tests
 * ObjectTestSuite instance variable.
 */
static ObjectTestSuite g_objectTestSuite;

} // namespace tests

} // namespace ns3
