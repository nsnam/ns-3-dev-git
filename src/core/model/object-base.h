/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef OBJECT_BASE_H
#define OBJECT_BASE_H

#include "callback.h"
#include "type-id.h"
#include "warnings.h"

#include <list>
#include <string>

/**
 * @file
 * @ingroup object
 * ns3::ObjectBase declaration and
 * NS_OBJECT_ENSURE_REGISTERED() macro definition.
 */

/**
 * @ingroup object
 * @brief Register an Object subclass with the TypeId system.
 *
 * This macro should be invoked once for every class which
 * defines a new GetTypeId method.
 *
 * If the class is in a namespace, then the macro call should also be
 * in the namespace.
 */
#define NS_OBJECT_ENSURE_REGISTERED(type)                                                          \
    static struct Object##type##RegistrationClass                                                  \
    {                                                                                              \
        Object##type##RegistrationClass()                                                          \
        {                                                                                          \
            NS_WARNING_PUSH_DEPRECATED;                                                            \
            ns3::TypeId tid = type::GetTypeId();                                                   \
            tid.SetSize(sizeof(type));                                                             \
            tid.GetParent();                                                                       \
            NS_WARNING_POP;                                                                        \
        }                                                                                          \
    } Object##type##RegistrationVariable

/**
 * @ingroup object
 * @brief Explicitly instantiate a template class with one template parameter
 *        and register the resulting instance with the TypeId system.
 *
 * This macro should be invoked once for every required instance of a template
 * class with one template parameter which derives from the Object class and
 * defines a new GetTypeId method.
 *
 * If the template class is in a namespace, then the macro call should also be
 * in the namespace.
 *
 * @note The type names used as arguments for this macro, being used to form a
 * class name and a variable name, CANNOT contain the scope resolution
 * operator (::)
 *
 * @tparam type the template class
 * @tparam param the first template parameter
 */
#define NS_OBJECT_TEMPLATE_CLASS_DEFINE(type, param)                                               \
    template class type<param>;                                                                    \
    template <>                                                                                    \
    std::string DoGetTemplateClassName<type<param>>()                                              \
    {                                                                                              \
        return std::string("ns3::") + std::string(#type) + std::string("<") +                      \
               std::string(#param) + std::string(">");                                             \
    }                                                                                              \
    static struct Object##type##param##RegistrationClass                                           \
    {                                                                                              \
        Object##type##param##RegistrationClass()                                                   \
        {                                                                                          \
            ns3::TypeId tid = type<param>::GetTypeId();                                            \
            tid.SetSize(sizeof(type<param>));                                                      \
            tid.GetParent();                                                                       \
        }                                                                                          \
    } Object##type##param##RegistrationVariable

/**
 * @ingroup object
 * @brief Explicitly instantiate a template class with two template parameters
 *        and register the resulting instance with the TypeId system.
 *
 * This macro should be invoked once for every required instance of a template
 * class with two template parameters which derives from the Object class and
 * defines a new GetTypeId method.
 *
 * If the template class is in a namespace, then the macro call should also be
 * in the namespace.
 *
 * @note The type names used as arguments for this macro, being used to form a
 * class name and a variable name, CANNOT contain the scope resolution
 * operator (::)
 *
 * @tparam type the template class
 * @tparam param1 the first template parameter
 * @tparam param2 the second template parameter
 */
#define NS_OBJECT_TEMPLATE_CLASS_TWO_DEFINE(type, param1, param2)                                  \
    template class type<param1, param2>;                                                           \
    template <>                                                                                    \
    std::string DoGetTemplateClassName<type<param1, param2>>()                                     \
    {                                                                                              \
        return std::string("ns3::") + std::string(#type) + std::string("<") +                      \
               std::string(#param1) + std::string(",") + std::string(#param2) + std::string(">");  \
    }                                                                                              \
    static struct Object##type##param1##param2##RegistrationClass                                  \
    {                                                                                              \
        Object##type##param1##param2##RegistrationClass()                                          \
        {                                                                                          \
            ns3::TypeId tid = type<param1, param2>::GetTypeId();                                   \
            tid.SetSize(sizeof(type<param1, param2>));                                             \
            tid.GetParent();                                                                       \
        }                                                                                          \
    } Object##type##param1##param2##RegistrationVariable

namespace ns3
{

/**
 * @brief Helper function to get the name (as a string) of the type
 *        of a template class
 * @return the name of the type of a template class as a string
 *
 * A specialization of this function is defined by the
 * NS_OBJECT_TEMPLATE_CLASS_DEFINE macro.
 */
template <typename T>
std::string DoGetTemplateClassName();

/**
 * @brief Helper function to get the name (as a string) of the type
 *        of a template class
 * @return the name of the type of a template class as a string
 */
template <typename T>
std::string
GetTemplateClassName()
{
    return DoGetTemplateClassName<T>();
}

class AttributeConstructionList;

/**
 * @ingroup object
 *
 * @brief Anchor the ns-3 type and attribute system.
 *
 * Every class which wants to integrate in the ns-3 type and attribute
 * system should derive from this base class. This base class provides:
 * - A way to associate an ns3::TypeId to each object instance.
 * - A way to set and get the attributes registered in the ns3::TypeId.
 */
class ObjectBase
{
  public:
    /**
     * Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Virtual destructor.
     */
    virtual ~ObjectBase();

    /**
     * Get the most derived TypeId for this Object.
     *
     * This method is typically implemented by ns3::Object::GetInstanceTypeId
     * but some classes which derive from ns3::ObjectBase directly
     * have to implement it themselves.
     *
     * @return The TypeId associated to the most-derived type
     *          of this instance.
     */
    virtual TypeId GetInstanceTypeId() const = 0;

    /**
     *
     * Set a single attribute, raising fatal errors if unsuccessful.
     *
     * This will either succeed at setting the attribute
     * or it will raise NS_FATAL_ERROR() on these conditions:
     *
     *   - The attribute doesn't exist in this Object.
     *   - The attribute can't be set (no Setter).
     *   - The attribute couldn't be deserialized from the AttributeValue.
     *
     * @param [in] name The name of the attribute to set.
     * @param [in] value The name of the attribute to set.
     */
    void SetAttribute(std::string name, const AttributeValue& value);
    /**
     * Set a single attribute without raising errors.
     *
     * If the attribute could not be set this will return \c false,
     * but not raise any errors.
     *
     * @param [in] name The name of the attribute to set.
     * @param [in] value The value to set it to.
     * @return \c true if the requested attribute exists and could be set,
     *         \c false otherwise.
     */
    bool SetAttributeFailSafe(std::string name, const AttributeValue& value);
    /**
     * Get the value of an attribute, raising fatal errors if unsuccessful.
     *
     * This will either succeed at setting the attribute
     * or it will raise NS_FATAL_ERROR() on these conditions:
     *
     *   - The attribute doesn't exist in this Object.
     *   - The attribute can't be read (no Getter).
     *   - The attribute doesn't support string formatting.
     *   - The attribute couldn't be serialized into the AttributeValue.
     *
     * @param [in]  name The name of the attribute to read.
     * @param [out] value Where the result should be stored.
     * @param [in] permissive If false (by default), will generate warnings and errors for
     * deprecated and obsolete attributes, respectively. If set to true, warnings for deprecated
     * attributes will be suppressed.
     */
    void GetAttribute(std::string name, AttributeValue& value, bool permissive = false) const;
    /**
     * Get the value of an attribute without raising errors.
     *
     * If the attribute could not be read this will return \c false,
     * but not raise any errors.
     *
     * @param [in]  name The name of the attribute to read.
     * @param [out] value Where the result value should be stored.
     * @return \c true if the requested attribute was found, \c false otherwise.
     */
    bool GetAttributeFailSafe(std::string name, AttributeValue& value) const;

    /**
     * Connect a TraceSource to a Callback with a context.
     *
     * The target trace source should be registered with TypeId::AddTraceSource.
     *
     * @param [in] name The name of the target trace source.
     * @param [in] context The trace context associated to the callback.
     * @param [in] cb The callback to connect to the trace source.
     * @returns \c true on success, \c false if TraceSource was not found.
     */
    bool TraceConnect(std::string name, std::string context, const CallbackBase& cb);
    /**
     * Connect a TraceSource to a Callback without a context.
     *
     * The target trace source should be registered with TypeId::AddTraceSource.
     *
     * @param [in] name The name of the target trace source.
     * @param [in] cb The callback to connect to the trace source.
     * @returns \c true on success, \c false if TraceSource was not found.
     */
    bool TraceConnectWithoutContext(std::string name, const CallbackBase& cb);
    /**
     * Disconnect from a TraceSource a Callback previously connected
     * with a context.
     *
     * The target trace source should be registered with TypeId::AddTraceSource.
     *
     * @param [in] name The name of the target trace source.
     * @param [in] context The trace context associated to the callback.
     * @param [in] cb The callback to disconnect from the trace source.
     * @returns \c true on success, \c false if TraceSource was not found.
     */
    bool TraceDisconnect(std::string name, std::string context, const CallbackBase& cb);
    /**
     * Disconnect from a TraceSource a Callback previously connected
     * without a context.
     *
     * The target trace source should be registered with TypeId::AddTraceSource.
     *
     * @param [in] name The name of the target trace source.
     * @param [in] cb The callback to disconnect from the trace source.
     * @returns \c true on success, \c false if TraceSource was not found.
     */
    bool TraceDisconnectWithoutContext(std::string name, const CallbackBase& cb);

  protected:
    /**
     * Notifier called once the ObjectBase is fully constructed.
     *
     * This method is invoked once all member attributes have been
     * initialized. Subclasses can override this method to be notified
     * of this event but if they do this, they must chain up to their
     * parent's NotifyConstructionCompleted method.
     */
    virtual void NotifyConstructionCompleted();
    /**
     * Complete construction of ObjectBase; invoked by derived classes.
     *
     * Invoked from subclasses to initialize all of their
     * attribute members. This method will typically be invoked
     * automatically from ns3::CreateObject if your class derives
     * from ns3::Object. If you derive from ns3::ObjectBase directly,
     * you should make sure that you invoke this method from
     * your most-derived constructor.
     *
     * @param [in] attributes The attribute values used to initialize
     *        the member variables of this object's instance.
     */
    void ConstructSelf(const AttributeConstructionList& attributes);

  private:
    /**
     * Attempt to set the value referenced by the accessor \pname{spec}
     * to a valid value according to the \c checker, based on \pname{value}.
     *
     * @param [in] spec The accessor for the storage location.
     * @param [in] checker The checker to use in validating the value.
     * @param [in] value The value to attempt to store.
     * @returns \c true if the \c value could be validated by the \pname{checker}
     *          and written to the storage location.
     */
    bool DoSet(Ptr<const AttributeAccessor> spec,
               Ptr<const AttributeChecker> checker,
               const AttributeValue& value);
};

// The following explicit template instantiation declarations prevent all the translation
// units including this header file to implicitly instantiate the callbacks class and
// function templates having ObjectBase as template type parameter that are required to be
// instantiated more often (accorging to the ClangBuildAnalyzer tool).
// These classes and functions are explicitly instantiated in object-base.cc
extern template Callback<ObjectBase*> MakeCallback<ObjectBase*>(ObjectBase* (*)());
extern template Callback<ObjectBase*>::Callback();
extern template class CallbackImpl<ObjectBase*>;

} // namespace ns3

#endif /* OBJECT_BASE_H */
