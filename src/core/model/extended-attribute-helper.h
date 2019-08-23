/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 WPL, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jared Dulmage <jared.dulmage@wpli.net>
 */

#ifndef EXTENDED_ATTRIBUTE_HELPER_H
#define EXTENDED_ATTRIBUTE_HELPER_H

#include "ns3/attribute-accessor-helper.h"

namespace ns3 {
/**
 * \ingroup attributeimpl
 *
 * MakeAccessorHelper implementation for a plain ole get functor.
 *
 * \tparam V  \explicit The specific AttributeValue type to use to represent
 *            the Attribute.
 * \tparam T  \deduced The class whose object pointer will be passed to the get functor.
 * \tparam U  \deduced The return type of the get functor.
 * \param [in] getter  The address of the get functor.
 * \returns The AttributeAccessor.
 */
template <typename V, typename T, typename U>
inline
Ptr<const AttributeAccessor>
DoMakeAccessorHelperOne (U (*getter)(const T*))
{
  /* AttributeAccessor implementation with a plain ole functor method. */
  class POFunction : public AccessorHelper<T,V>
  {
    public:
        /*
        * Construct from a class get functor method.
        * \param [in] getter The class get functor method pointer.
        */
        POFunction (U (*getter)(const T*))
        : AccessorHelper<T,V> (),
            m_getter (getter)
        {}
    private:
        virtual bool DoSet (T *object, const V *v) const {
        return false;
        }
        virtual bool DoGet (const T *object, V *v) const {
        v->Set ((*m_getter)(object));
        return true;
        }
        virtual bool HasGetter (void) const {
        return true;
        }
        virtual bool HasSetter (void) const {
        return false;
        }
        U (*m_getter)(const T*);  // The class get functor method pointer.
  };
  return Ptr<const AttributeAccessor> (new POFunction (getter), false);
}

/**
 * \ingroup attributeimpl
 *
 * MakeAccessorHelper implementation for a plain ole function returning void.
 *
 * \tparam V  \explicit The specific AttributeValue type to use to represent
 *            the Attribute.
 * \tparam T  \deduced The class whose object pointer will be passed to the set functor.
 * \tparam U  \deduced The argument type of the set functor.
 * \param [in] setter  The address of the set functor, returning void.
 * \returns The AttributeAccessor.
 */
template <typename V, typename T, typename U>
inline
Ptr<const AttributeAccessor>
DoMakeAccessorHelperOne (void (*setter)(T*, U))
{
  /* AttributeAccessor implementation with a plain ole function returning void. */
  class POFunction : public AccessorHelper<T,V>
  {
public:
    /*
     * Construct from a class set method.
     * \param [in] setter The class set method pointer.
     */
    POFunction (void (*setter)(T*, U))
      : AccessorHelper<T,V> (),
        m_setter (setter)
    {}
private:
    virtual bool DoSet (T *object, const V *v) const {
      typename AccessorTrait<U>::Result tmp;
      bool ok = v->GetAccessor (tmp);
      if (!ok)
        {
          return false;
        }
      (*m_setter)(object, tmp);
      return true;
    }
    virtual bool DoGet (const T *object, V *v) const {
      return false;
    }
    virtual bool HasGetter (void) const {
      return false;
    }
    virtual bool HasSetter (void) const {
      return true;
    }
    void (*m_setter)(T*, U);  // The class set method pointer, returning void.
  };
  return Ptr<const AttributeAccessor> (new POFunction (setter), false);
}

/**
 * \ingroup attributeimpl
 *
 * MakeAccessorHelper implementation for a ns3::Callback argument
 *
 * Using ns3::Callback functors gives the option of binding arguments
 * \tparam V  \explicit The specific AttributeValue type to use to represent
 *            the Attribute.
 * \tparam T  \deduced The class whose object pointer will be passed to the set functor.
 * \tparam U  \deduced The argument type of the set functor.
 * \param [in] setter  The set functor (type ns3::Callback), returning void.
 * \returns The AttributeAccessor.
 */
template <typename V, typename T, typename U>
inline
Ptr<const AttributeAccessor>
DoMakeAccessorHelperOne (Callback <void, T*, U> setter)
{
  /* AttributeAccessor implementation with a plain ole function returning void. */
  class POFunction : public AccessorHelper<T,V>
  {
public:
    /*
     * Construct from a class set method.
     * \param [in] setter The class set method pointer.
     */
    POFunction (Callback<void, T*, U> setter)
      : AccessorHelper<T,V> (),
        m_setter (setter)
    {}
private:
    virtual bool DoSet (T *object, const V *v) const {
      typename AccessorTrait<U>::Result tmp;
      bool ok = v->GetAccessor (tmp);
      if (!ok)
        {
          return false;
        }
      m_setter(object, tmp);
      return true;
    }
    virtual bool DoGet (const T *object, V *v) const {
      return false;
    }
    virtual bool HasGetter (void) const {
      return false;
    }
    virtual bool HasSetter (void) const {
      return true;
    }
    Callback<void, T*, U> m_setter;  // The class set method pointer, returning void.
  };
  return Ptr<const AttributeAccessor> (new POFunction (setter), false);
}

/**
 * \ingroup attributeimpl
 *
 * MakeAccessorHelper implementation with a get functor
 * and a set functor returning void.
 *
 * The two versions of this function differ only in argument order.
 *
 * \tparam W  \explicit The specific AttributeValue type to use to represent
 *            the Attribute.
 * \tparam T  \deduced The class whose object pointer is passed to the functors.
 * \tparam U  \deduced The argument type of the set method.
 * \tparam V  \deduced The return type of the get functor method.
 * \param [in] setter The address of the set functor, returning void.
 * \param [in] getter The address of the get functor.
 * \returns The AttributeAccessor.
 */
template <typename W, typename T, typename U, typename V>
inline
Ptr<const AttributeAccessor>
DoMakeAccessorHelperTwo (void (*setter)(T*, U), V (*getter)(T*))
{
  /*
   * AttributeAccessor implementation with plain ole get functor and set functor,
   * returning void.
   */
  class POFunction : public AccessorHelper<T,W>
  {
public:
    /*
     * Construct from get and set functors.
     * \param [in] setter The set functor pointer, returning void.
     * \param [in] getter The get functor pointer.
     */
    POFunction (void (*setter)(T*, U), V (*getter)(T*))
      : AccessorHelper<T,W> (),
        m_setter (setter),
        m_getter (getter)
    {}
private:
    virtual bool DoSet (T *object, const W *v) const {
      typename AccessorTrait<U>::Result tmp;
      bool ok = v->GetAccessor (tmp);
      if (!ok)
        {
          return false;
        }
      (*m_setter)(object, tmp);
      return true;
    }
    virtual bool DoGet (const T *object, W *v) const {
      v->Set ((*m_getter)(object));
      return true;
    }
    virtual bool HasGetter (void) const {
      return true;
    }
    virtual bool HasSetter (void) const {
      return true;
    }
    void (*m_setter)(T*, U);        // The class set method pointer, returning void.
    V (*m_getter)(T*);              // The class get functor method pointer.
  };
  return Ptr<const AttributeAccessor> (new POFunction (setter, getter), false);
}


/**
 * \ingroup attributeimpl
 * \copydoc DoMakeAccessorHelperTwo(void(*)(T*, U),V(*)(T*))
 */
template <typename W, typename T, typename U, typename V>
inline
Ptr<const AttributeAccessor>
DoMakeAccessorHelperTwo (V (*getter)(T*), void (*setter)(T*, U))
{
  return DoMakeAccessorHelperTwo<W> (setter, getter);
}


/**
 * \ingroup attributeimpl
 *
 * MakeAccessorHelper implementation with a get functor
 * and a set functor returning \p bool.
 *
 * The two versions of this function differ only in argument order.
 *
 * \tparam W  \explicit The specific AttributeValue type to use to represent
 *            the Attribute.
 * \tparam T  \deduced The class whose object pointer is passed to the functors.
 * \tparam U  \deduced The argument type of the set method.
 * \tparam V  \deduced The return type of the get functor method.
 * \param [in] setter The address of the set functor, returning bool.
 * \param [in] getter The address of the get functor.
 * \returns The AttributeAccessor.
 */
template <typename W, typename T, typename U, typename V>
inline
Ptr<const AttributeAccessor>
DoMakeAccessorHelperTwo (bool (*setter)(T*, U), V (*getter)(T*))
{
  /*
   * AttributeAccessor implementation with plain ole get functor and
   * set functor, returning bool.
   */
  class POFunction : public AccessorHelper<T,W>
  {
public:
    /*
     * Construct from class get functor and set method, returning bool.
     * \param [in] setter The class set method pointer, returning bool.
     * \param [in] getter The class get functor method pointer.
     */
    POFunction (bool (*setter)(T*, U), V (*getter)(T**))
      : AccessorHelper<T,W> (),
        m_setter (setter),
        m_getter (getter)
    {}
private:
    virtual bool DoSet (T *object, const W *v) const {
      typename AccessorTrait<U>::Result tmp;
      bool ok = v->GetAccessor (tmp);
      if (!ok)
        {
          return false;
        }
      ok = (*m_setter)(object, tmp);
      return ok;
    }
    virtual bool DoGet (const T *object, W *v) const {
      v->Set ((*m_getter)(object));
      return true;
    }
    virtual bool HasGetter (void) const {
      return true;
    }
    virtual bool HasSetter (void) const {
      return true;
    }
    bool (*m_setter)(T*, U);        // The set method pointer, returning bool.
    V (*m_getter)(T*);              // The get functor method pointer.
  };
  return Ptr<const AttributeAccessor> (new POFunction (setter, getter), false);
}


/**
 * \ingroup attributeimpl
 * \copydoc ns3::DoMakeAccessorHelperTwo(bool(*)(T*, U),V(*)(T*))
 */
template <typename W, typename T, typename U, typename V>
inline
Ptr<const AttributeAccessor>
DoMakeAccessorHelperTwo (V (*getter)(T*), bool (*setter)(T*, U))
{
  return DoMakeAccessorHelperTwo<W> (setter, getter);
}

} // namespace ns3


#endif // EXTENDED_ATTRIBUTE_HELPER_H