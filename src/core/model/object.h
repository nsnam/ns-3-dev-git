/*
 * Copyright (c) 2007 INRIA, Gustavo Carneiro
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Gustavo Carneiro <gjcarneiro@gmail.com>,
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef OBJECT_H
#define OBJECT_H

#include "attribute-construction-list.h"
#include "attribute.h"
#include "object-base.h"
#include "ptr.h"
#include "simple-ref-count.h"

#include <stdint.h>
#include <string>
#include <vector>

/**
 * @file
 * @ingroup object
 * ns3::Object class declaration, which is the root of the Object hierarchy
 * and Aggregation.
 */

namespace ns3
{

class Object;
class AttributeAccessor;
class AttributeValue;
class TraceSourceAccessor;

/**
 * @ingroup core
 * @defgroup object Object
 * @brief Base classes which provide memory management and object aggregation.
 */

/**
 * @ingroup object
 * @ingroup ptr
 * Standard Object deleter, used by SimpleRefCount
 * to delete an Object when the reference count drops to zero.
 */
struct ObjectDeleter
{
    /**
     * Smart pointer deleter implementation for Object.
     *
     * Delete implementation, forwards to the Object::DoDelete()
     * method.
     *
     * @param [in] object The Object to delete.
     */
    inline static void Delete(Object* object);
};

/**
 * @ingroup object
 * @brief A base class which provides memory management and object aggregation
 *
 * The memory management scheme is based on reference-counting with
 * dispose-like functionality to break the reference cycles.
 * The reference count is incremented and decremented with
 * the methods Ref() and Unref(). If a reference cycle is
 * present, the user is responsible for breaking it
 * by calling Dispose() in a single location. This will
 * eventually trigger the invocation of DoDispose() on itself and
 * all its aggregates. The DoDispose() method is always automatically
 * invoked from the Unref() method before destroying the Object,
 * even if the user did not call Dispose() directly.
 */
class Object : public SimpleRefCount<Object, ObjectBase, ObjectDeleter>
{
  public:
    /**
     * @brief Register this type.
     * @return The Object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Iterate over the Objects aggregated to an ns3::Object.
     *
     * This iterator does not allow you to iterate over the parent
     * Object used to call Object::GetAggregateIterator.
     *
     * @note This is a java-style iterator.
     */
    class AggregateIterator
    {
      public:
        /** Default constructor, which has no Object. */
        AggregateIterator();

        /**
         * Check if there are more Aggregates to iterate over.
         *
         * @returns \c true if Next() can be called and return a non-null
         *          pointer, \c false otherwise.
         */
        bool HasNext() const;

        /**
         * Get the next Aggregated Object.
         *
         * @returns The next aggregated Object.
         */
        Ptr<const Object> Next();

      private:
        /** Object needs access. */
        friend class Object;
        /**
         * Construct from an Object.
         *
         * This is private, with Object as friend, so only Objects can create
         * useful AggregateIterators.
         *
         * @param [in] object The Object whose Aggregates should be iterated over.
         */
        AggregateIterator(Ptr<const Object> object);
        Ptr<const Object> m_object; //!< Parent Object.
        uint32_t m_current;         //!< Current position in parent's aggregates.
        /// Iterator to the unidirectional aggregates.
        std::vector<Ptr<Object>>::const_iterator m_uniAggrIter;
    };

    /** Constructor. */
    Object();
    /** Destructor. */
    ~Object() override;

    TypeId GetInstanceTypeId() const final;

    /**
     * Get a pointer to the requested aggregated Object.  If the type of object
     * requested is ns3::Object, a Ptr to the calling object is returned.
     *
     * @tparam T \explicit The type of the aggregated Object to retrieve.
     * @returns A pointer to the requested Object, or zero
     *          if it could not be found.
     */
    template <typename T>
    inline Ptr<T> GetObject() const;
    /**
     * Get a pointer to the requested aggregated Object by TypeId.  If the
     * TypeId argument is ns3::Object, a Ptr to the calling object is returned.
     *
     * @tparam T \explicit The type of the aggregated Object to retrieve.
     * @param [in] tid The TypeId of the requested Object.
     * @returns A pointer to the requested Object with the specified TypeId,
     *          or zero if it could not be found.
     */
    template <typename T>
    Ptr<T> GetObject(TypeId tid) const;
    /**
     * Dispose of this Object.
     *
     * Run the DoDispose() methods of this Object and all the
     * Objects aggregated to it.
     * After calling this method, this Object is expected to be
     * totally unusable except for the Ref() and Unref() methods.
     *
     * @note You can call Dispose() many times on the same Object or
     * different Objects aggregated together, and DoDispose() will be
     * called only once for each aggregated Object.
     *
     * This method is typically used to break reference cycles.
     */
    void Dispose();
    /**
     * Aggregate two Objects together.
     *
     * @param [in] other The other Object pointer
     *
     * This method aggregates the two Objects together: after this
     * method returns, it becomes possible to call GetObject()
     * on one to get the other, and vice-versa.
     *
     * This method calls the virtual method NotifyNewAggregates() to
     * notify all aggregated Objects that they have been aggregated
     * together.
     *
     * \sa NotifyNewAggregate()
     */
    void AggregateObject(Ptr<Object> other);

    /**
     * Aggregate an Object to another Object.
     *
     * @param [in] other The other Object pointer
     *
     * This method aggregates the an object to another Object:
     * after this method returns, it becomes possible to call GetObject()
     * on the aggregating Object to get the other, but not vice-versa.
     *
     * This method calls the virtual method NotifyNewAggregates() to
     * notify all aggregated Objects that they have been aggregated
     * together.
     *
     * This method is useful only if there is the need to aggregate an
     * object to more than one object at the same time, and should be avoided
     * if not strictly necessary.
     * In particular, objects aggregated with this method should be destroyed
     * only after making sure that the objects they are aggregated to are
     * destroyed as well. However, the destruction of the aggregating objects
     * will take care of the unidirectional aggregated objects gracefully.
     *
     * Beware that an object aggregated to another with this function
     * behaves differently than other aggregates in the following ways.
     * Suppose that Object B is aggregated unidirectionally:
     *   - It can be aggregated unidirectionally to more than one objects
     *     (e.g., A1 and A2).
     *   - It is not possible to call GetObject on B to find an aggregate of
     *     object A1 or A2.
     *   - When A1 or A2 are initialized, B is initialized, whichever happens first.
     *   - When A1 or A2 are destroyed, B is destroyed, whichever happens last.
     *   - If B is initialized, A1 and A2 are unaffected.
     *   - If B is forcefully destroyed, A1 and A2 are unaffected.
     *
     *
     * \sa AggregateObject()
     */
    void UnidirectionalAggregateObject(Ptr<Object> other);

    /**
     * Get an iterator to the Objects aggregated to this one.
     *
     * @returns An iterator to the first Object aggregated to this
     *          Object.
     *
     * If no Objects are aggregated to this Object, then, the returned
     * iterator will be empty and AggregateIterator::HasNext() will
     * always return \c false.
     */
    AggregateIterator GetAggregateIterator() const;

    /**
     * Invoke DoInitialize on all Objects aggregated to this one.
     *
     * This method calls the virtual DoInitialize() method on all the Objects
     * aggregated to this Object. DoInitialize() will be called only once over
     * the lifetime of an Object, just like DoDispose() is called only
     * once.
     *
     * \sa DoInitialize()
     */
    void Initialize();

    /**
     * Check if the object has been initialized.
     *
     * @brief Check if the object has been initialized.
     * @returns \c true if the object has been initialized.
     */
    bool IsInitialized() const;

  protected:
    /**
     * Notify all Objects aggregated to this one of a new Object being
     * aggregated.
     *
     * This method is invoked whenever two sets of Objects are aggregated
     * together.  It is invoked exactly once for each Object in both sets.
     * This method can be overridden by subclasses who wish to be notified
     * of aggregation events. These subclasses must chain up to their
     * base class NotifyNewAggregate() method.
     *
     * It is safe to call GetObject() and AggregateObject() from within
     * this method.
     */
    virtual void NotifyNewAggregate();
    /**
     * Initialize() implementation.
     *
     * This method is called only once by Initialize(). If the user
     * calls Initialize() multiple times, DoInitialize() is called only the
     * first time.
     *
     * Subclasses are expected to override this method and chain up
     * to their parent's implementation once they are done. It is
     * safe to call GetObject() and AggregateObject() from within this method.
     */
    virtual void DoInitialize();
    /**
     * Destructor implementation.
     *
     * This method is called by Dispose() or by the Object's
     * destructor, whichever comes first.
     *
     * Subclasses are expected to implement their real destruction
     * code in an overridden version of this method and chain
     * up to their parent's implementation once they are done.
     * _i.e_, for simplicity, the destructor of every subclass should
     * be empty and its content should be moved to the associated
     * DoDispose() method.
     *
     * It is safe to call GetObject() from within this method.
     */
    virtual void DoDispose();
    /**
     * Copy an Object.
     *
     * @param [in] o the Object to copy.
     *
     * Allow subclasses to implement a copy constructor.
     *
     * While it is technically possible to implement a copy
     * constructor in a subclass, we strongly discourage you
     * from doing so. If you really want to do it anyway, you have
     * to understand that this copy constructor will _not_
     * copy aggregated Objects, _i.e_, if your Object instance
     * is already aggregated to another Object and if you invoke
     * this copy constructor, the new Object instance will be
     * a pristine standalone Object instance not aggregated to
     * any other Object. It is thus _your_ responsibility
     * as a caller of this method to do what needs to be done
     * (if it is needed) to ensure that the Object stays in a
     * valid state.
     */
    Object(const Object& o);

  private:
    /**
     * Copy an Object.
     *
     * @tparam T \deduced The type of the Object being copied.
     * @param [in] object A pointer to the object to copy.
     * @returns A copy of the input object.
     *
     * This method invoke the copy constructor of the input object
     * and returns the new instance.
     */
    template <typename T>
    friend Ptr<T> CopyObject(Ptr<T> object);

    /** @copydoc CopyObject(Ptr<T>) */
    template <typename T>
    friend Ptr<T> CopyObject(Ptr<const T> object);

    /**
     * Set the TypeId and construct all Attributes of an Object.
     *
     * @tparam T \deduced The type of the Object to complete.
     * @param [in] object The uninitialized object pointer.
     * @return The derived object.
     */
    template <typename T>
    friend Ptr<T> CompleteConstruct(T* object);

    /** Friends. @{*/
    friend class ObjectFactory;
    friend class AggregateIterator;
    friend struct ObjectDeleter;

    /**@}*/

    /**
     * The list of Objects aggregated to this one.
     *
     * This data structure uses a classic C-style trick to
     * hold an array of variable size without performing
     * two memory allocations: the declaration of the structure
     * declares a one-element array but when we allocate
     * memory for this struct, we effectively allocate a larger
     * chunk of memory than the struct to allow space for a larger
     * variable sized buffer whose size is indicated by the element
     * \c n
     */
    struct Aggregates
    {
        /** The number of entries in \c buffer. */
        uint32_t n;
        /** The array of Objects. */
        Object* buffer[1];
    };

    /**
     * Find an Object of TypeId tid in the aggregates of this Object.
     *
     * @param [in] tid The TypeId we're looking for
     * @return The matching Object, if it is found
     */
    Ptr<Object> DoGetObject(TypeId tid) const;
    /**
     * Verify that this Object is still live, by checking it's reference count.
     * @return \c true if the reference count is non zero.
     */
    bool Check() const;
    /**
     * Check if any aggregated Objects have non-zero reference counts.
     *
     * @return \c true if any of our aggregates have non zero reference count.
     *
     * In some cases, when an event is scheduled against a subclass of
     * Object, and if no one owns a reference directly to this Object, the
     * Object is alive, has a refcount of zero and the method run when the
     * event expires runs against the raw pointer, which means that we are
     * manipulating an Object with a refcount of zero.  So, instead we
     * check the aggregate reference count.
     */
    bool CheckLoose() const;
    /**
     * Set the TypeId of this Object.

     * @param [in] tid The TypeId value to set.
     *
     * Invoked from ns3::CreateObject only.
     * Initialize the \c m_tid member variable to
     * keep track of the type of this Object instance.
     */
    void SetTypeId(TypeId tid);
    /**
     * Initialize all member variables registered as Attributes of this TypeId.
     *
     * @param [in] attributes The attribute values used to initialize
     *        the member variables of this Object's instance.
     *
     * Invoked from ns3::ObjectFactory::Create and ns3::CreateObject only.
     * Initialize all the member variables which were
     * registered with the associated TypeId.
     */
    void Construct(const AttributeConstructionList& attributes);

    /**
     * Keep the list of aggregates in most-recently-used order
     *
     * @param [in,out] aggregates The list of aggregated Objects.
     * @param [in] i The most recently used entry in the list.
     */
    void UpdateSortedArray(Aggregates* aggregates, uint32_t i) const;
    /**
     * Attempt to delete this Object.
     *
     * This method iterates over all aggregated Objects to check if they all
     * have a zero refcount. If yes, the Object and all
     * its aggregates are deleted. If not, nothing is done.
     */
    void DoDelete();

    /**
     * Identifies the type of this Object instance.
     */
    TypeId m_tid;
    /**
     * Set to \c true when the DoDispose() method of the Object has run,
     * \c false otherwise.
     */
    bool m_disposed;
    /**
     * Set to \c true once the DoInitialize() method has run,
     * \c false otherwise
     */
    bool m_initialized;
    /**
     * A pointer to an array of 'aggregates'.
     *
     * A pointer to each Object aggregated to this Object is stored in this
     * array.  The array is shared by all aggregated Objects
     * so the size of the array is indirectly a reference count.
     */
    Aggregates* m_aggregates;

    /**
     * An array of unidirectional aggregates, i.e., objects that are
     * aggregated to the current object, but not vice-versa.
     *
     * This is useful (and suggested) only for Objects that should
     * be aggregated to multiple other Objects, where the normal
     * Aggregation would create an issue.
     */
    std::vector<Ptr<Object>> m_unidirectionalAggregates;

    /**
     * The number of times the Object was accessed with a
     * call to GetObject().
     *
     * This integer is used to implement a heuristic to sort
     * the array of aggregates in most-frequently accessed order.
     */
    uint32_t m_getObjectCount;
};

template <typename T>
Ptr<T> CopyObject(Ptr<const T> object);
template <typename T>
Ptr<T> CopyObject(Ptr<T> object);

} // namespace ns3

namespace ns3
{

/*************************************************************************
 *   The Object implementation which depends on templates
 *************************************************************************/

void
ObjectDeleter::Delete(Object* object)
{
    object->DoDelete();
}

template <typename T>
Ptr<T>
Object::GetObject() const
{
    // This is an optimization: if the cast works (which is likely),
    // things will be pretty fast.
    T* result = dynamic_cast<T*>(m_aggregates->buffer[0]);
    if (result != nullptr)
    {
        return Ptr<T>(result);
    }
    // if the cast does not work, we try to do a full type check.
    Ptr<Object> found = DoGetObject(T::GetTypeId());
    if (found)
    {
        return Ptr<T>(static_cast<T*>(PeekPointer(found)));
    }
    return nullptr;
}

/**
 * Specialization of \link Object::GetObject () \endlink for
 * objects of type ns3::Object.
 *
 * @returns A Ptr to the calling object.
 */
template <>
inline Ptr<Object>
Object::GetObject() const
{
    return Ptr<Object>(const_cast<Object*>(this));
}

template <typename T>
Ptr<T>
Object::GetObject(TypeId tid) const
{
    Ptr<Object> found = DoGetObject(tid);
    if (found)
    {
        return Ptr<T>(static_cast<T*>(PeekPointer(found)));
    }
    return nullptr;
}

/**
 * Specialization of \link Object::GetObject (TypeId tid) \endlink for
 * objects of type ns3::Object.
 *
 * @param [in] tid The TypeId of the requested Object.
 * @returns A Ptr to the calling object.
 */
template <>
inline Ptr<Object>
Object::GetObject(TypeId tid) const
{
    if (tid == Object::GetTypeId())
    {
        return Ptr<Object>(const_cast<Object*>(this));
    }
    else
    {
        return DoGetObject(tid);
    }
}

/*************************************************************************
 *   The helper functions which need templates.
 *************************************************************************/

template <typename T>
Ptr<T>
CopyObject(Ptr<T> object)
{
    Ptr<T> p = Ptr<T>(new T(*PeekPointer(object)), false);
    NS_ASSERT(p->GetInstanceTypeId() == object->GetInstanceTypeId());
    return p;
}

template <typename T>
Ptr<T>
CopyObject(Ptr<const T> object)
{
    Ptr<T> p = Ptr<T>(new T(*PeekPointer(object)), false);
    NS_ASSERT(p->GetInstanceTypeId() == object->GetInstanceTypeId());
    return p;
}

template <typename T>
Ptr<T>
CompleteConstruct(T* object)
{
    object->SetTypeId(T::GetTypeId());
    object->Object::Construct(AttributeConstructionList());
    return Ptr<T>(object, false);
}

/**
 * @ingroup object
 * @{
 */
/**
 * Create an object by type, with varying number of constructor parameters.
 *
 * @tparam T \explicit The type of the derived object to construct.
 * @param [in] args Arguments to pass to the constructor.
 * @return The derived object.
 */
template <typename T, typename... Args>
Ptr<T>
CreateObject(Args&&... args)
{
    return CompleteConstruct(new T(std::forward<Args>(args)...));
}

/**@}*/

} // namespace ns3

#endif /* OBJECT_H */
