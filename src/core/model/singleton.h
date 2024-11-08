/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef SINGLETON_H
#define SINGLETON_H

/**
 * @file
 * @ingroup singleton
 * ns3::Singleton declaration and template implementation.
 */

namespace ns3
{

/**
 * @ingroup core
 * @defgroup singleton Singleton
 *
 * Template class implementing the Singleton design pattern.
 */

/**
 * @ingroup singleton
 * @brief A template singleton
 *
 * This template class can be used to implement the singleton pattern.
 * The underlying object will be destroyed automatically when the process
 * exits.
 *
 * For a singleton whose lifetime is bounded by the simulation run,
 * not the process, see SimulationSingleton.
 *
 * To force your `class ExampleS` to be a singleton, inherit from Singleton:
 * @code
 *   class ExampleS : public Singleton<ExampleS> { ... };
 * @endcode
 *
 * Then, to reach the singleton instance, just do
 * @code
 *   ExampleS::Get ()->...;
 * @endcode
 *
 * @note
 * If you call Get() again after the object has
 * been destroyed, the object will be re-created which will result in a
 * memory leak as reported by most memory leak checkers. It is up to the
 * user to ensure that Get() is never called from a static variable
 * finalizer.
 */
template <typename T>
class Singleton
{
  public:
    // Delete copy constructor and assignment operator to avoid misuse
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>&) = delete;

    /**
     * Get a pointer to the singleton instance.
     *
     * The instance will be automatically deleted when
     * the process exits.
     *
     * @return A pointer to the singleton instance.
     */
    static T* Get();

  protected:
    /** Constructor. */
    Singleton()
    {
    }

    /** Destructor. */
    virtual ~Singleton()
    {
    }
};

} // namespace ns3

/********************************************************************
 *  Implementation of the templates declared above.
 ********************************************************************/

namespace ns3
{

template <typename T>
T*
Singleton<T>::Get()
{
    static T object;
    return &object;
}

} // namespace ns3

#endif /* SINGLETON_H */
