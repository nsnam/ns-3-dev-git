/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef GLOBAL_VALUE_H
#define GLOBAL_VALUE_H

#include "attribute.h"
#include "ptr.h"

#include <string>
#include <vector>

/**
 * @file
 * @ingroup core
 * ns3::GlobalValue declaration.
 */

namespace ns3
{

/* Forward declaration */
namespace tests
{
class GlobalValueTestCase;
}

/**
 * @ingroup core
 *
 * @brief Hold a so-called 'global value'.
 *
 * A GlobalValue will get its value from (in order):
 *   - The initial value configured where it is defined,
 *   - From the \c NS_GLOBAL_VALUE environment variable,
 *   - From the command line,
 *   - By explicit call to SetValue() or Bind().
 *
 * Instances of this class are expected to be allocated as static
 * global variables and should be used to store configurable global state.
 * For example:
 * @code
 *   // source.cc:
 *   static GlobalValue g_myGlobal =
 *     GlobalValue ("myGlobal", "My global value for ...",
 *                  IntegerValue (12),
 *                  MakeIntegerChecker ());
 * @endcode
 *
 * GlobalValues can be set directly by calling GlobalValue::SetValue()
 * but they can also be set through the \c NS_GLOBAL_VALUE environment variable.
 * For example, \c NS_GLOBAL_VALUE='Name=Value;OtherName=OtherValue;' would set
 * global values \c Name and \c OtherName to \c Value and \c OtherValue,
 * respectively.
 *
 * Users of the CommandLine class also get the ability to set global
 * values through command line arguments to their program:
 * \c --Name=Value will set global value \c Name to \c Value.
 */
class GlobalValue
{
    /** Container type for holding all the GlobalValues. */
    typedef std::vector<GlobalValue*> Vector;

  public:
    /** Iterator type for the list of all global values. */
    typedef Vector::const_iterator Iterator;

    /**
     * Constructor.
     * @param [in] name the name of this global value.
     * @param [in] help some help text which describes the purpose of this
     *        global value.
     * @param [in] initialValue the value to assign to this global value
     *        during construction.
     * @param [in] checker a pointer to an AttributeChecker which can verify
     *        that any user-supplied value to override the initial
     *        value matches the requested type constraints.
     */
    GlobalValue(std::string name,
                std::string help,
                const AttributeValue& initialValue,
                Ptr<const AttributeChecker> checker);

    /**
     * Get the name.
     * @returns The name of this GlobalValue.
     */
    std::string GetName() const;
    /**
     * Get the help string.
     * @returns The help text of this GlobalValue.
     */
    std::string GetHelp() const;
    /**
     * Get the value.
     * @param [out] value The AttributeValue to set to the value
     *                    of this GlobalValue
     */
    void GetValue(AttributeValue& value) const;
    /**
     * Get the AttributeChecker.
     * @returns The checker associated to this GlobalValue.
     */
    Ptr<const AttributeChecker> GetChecker() const;
    /**
     * Set the value of this GlobalValue.
     * @param [in] value the new value to set in this GlobalValue.
     * @returns \c true if the Global Value was set successfully.
     */
    bool SetValue(const AttributeValue& value);

    /** Reset to the initial value. */
    void ResetInitialValue();

    /**
     * Iterate over the set of GlobalValues until a matching name is found
     * and then set its value with GlobalValue::SetValue.
     *
     * @param [in] name the name of the global value
     * @param [in] value the value to set in the requested global value.
     *
     * This method cannot fail. It will crash if the input is not valid.
     */
    static void Bind(std::string name, const AttributeValue& value);

    /**
     * Iterate over the set of GlobalValues until a matching name is found
     * and then set its value with GlobalValue::SetValue.
     *
     * @param [in] name the name of the global value
     * @param [in] value the value to set in the requested global value.
     * @returns \c true if the value could be set successfully,
     *          \c false otherwise.
     */
    static bool BindFailSafe(std::string name, const AttributeValue& value);

    /**
     * The Begin iterator.
     * @returns An iterator which represents a pointer to the first GlobalValue registered.
     */
    static Iterator Begin();
    /**
     * The End iterator.
     * @returns An iterator which represents a pointer to the last GlobalValue registered.
     */
    static Iterator End();

    /**
     * Finds the GlobalValue with the given name and returns its value
     *
     * @param [in] name the name of the GlobalValue to be found
     * @param [out] value where to store the value of the found GlobalValue
     *
     * @return \c true if the GlobalValue was found, \c false otherwise
     */
    static bool GetValueByNameFailSafe(std::string name, AttributeValue& value);

    /**
     * Finds the GlobalValue with the given name and returns its
     * value.
     *
     * This method cannot fail, i.e., it will trigger a
     * NS_FATAL_ERROR if the requested GlobalValue is not found.
     *
     * @param [in] name the name of the GlobalValue to be found
     * @param [out] value where to store the value of the found GlobalValue
     */
    static void GetValueByName(std::string name, AttributeValue& value);

  private:
    /** Test case needs direct access to GetVector() */
    friend class tests::GlobalValueTestCase;

    /**
     * Get the static vector of all GlobalValues.
     *
     * @returns The vector.
     */
    static Vector* GetVector();
    /** Initialize from the \c NS_GLOBAL_VALUE environment variable. */
    void InitializeFromEnv();

    /** The name of this GlobalValue. */
    std::string m_name;
    /** The help string. */
    std::string m_help;
    /** The initial value. */
    Ptr<AttributeValue> m_initialValue;
    /** The current value. */
    Ptr<AttributeValue> m_currentValue;
    /** The AttributeChecker for this GlobalValue. */
    Ptr<const AttributeChecker> m_checker;
};

} // namespace ns3

#endif /* GLOBAL_VALUE_H */
