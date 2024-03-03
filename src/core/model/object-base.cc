/*
 * Copyright (c) 2008 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "object-base.h"

#include "assert.h"
#include "attribute-construction-list.h"
#include "environment-variable.h"
#include "log.h"
#include "string.h"
#include "trace-source-accessor.h"

#include "ns3/core-config.h"

/**
 * \file
 * \ingroup object
 * ns3::ObjectBase class implementation.
 */

namespace ns3
{
// Explicit instantiation declaration

/**
 * \ingroup callback
 * Explicit instantiation for ObjectBase
 * \return A wrapper Callback
 * \sa ns3::MakeCallback
 */
template Callback<ObjectBase*> MakeCallback<ObjectBase*>(ObjectBase* (*)());
template Callback<ObjectBase*>::Callback();
template class CallbackImpl<ObjectBase*>;

NS_LOG_COMPONENT_DEFINE("ObjectBase");

NS_OBJECT_ENSURE_REGISTERED(ObjectBase);

/**
 * Ensure the TypeId for ObjectBase gets fully configured
 * to anchor the inheritance tree properly.
 *
 * \relates ns3::ObjectBase
 *
 * \return The TypeId for ObjectBase.
 */
static TypeId
GetObjectIid()
{
    NS_LOG_FUNCTION_NOARGS();
    TypeId tid = TypeId("ns3::ObjectBase");
    tid.SetParent(tid);
    tid.SetGroupName("Core");
    return tid;
}

TypeId
ObjectBase::GetTypeId()
{
    NS_LOG_FUNCTION_NOARGS();
    static TypeId tid = GetObjectIid();
    return tid;
}

ObjectBase::~ObjectBase()
{
    NS_LOG_FUNCTION(this);
}

void
ObjectBase::NotifyConstructionCompleted()
{
    NS_LOG_FUNCTION(this);
}

void
ObjectBase::ConstructSelf(const AttributeConstructionList& attributes)
{
    // loop over the inheritance tree back to the Object base class.
    NS_LOG_FUNCTION(this << &attributes);
    TypeId tid = GetInstanceTypeId();
    do // Do this tid and all parents
    {
        // loop over all attributes in object type
        NS_LOG_DEBUG("construct tid=" << tid.GetName() << ", params=" << tid.GetAttributeN());
        for (uint32_t i = 0; i < tid.GetAttributeN(); i++)
        {
            TypeId::AttributeInformation info = tid.GetAttribute(i);
            NS_LOG_DEBUG("try to construct \"" << tid.GetName() << "::" << info.name << "\"");
            // is this attribute stored in this AttributeConstructionList instance ?
            Ptr<const AttributeValue> value = attributes.Find(info.checker);
            std::string where = "argument";

            // See if this attribute should not be set here in the
            // constructor.
            if (!(info.flags & TypeId::ATTR_CONSTRUCT))
            {
                // Handle this attribute if it should not be
                // set here.
                if (!value)
                {
                    // Skip this attribute if it's not in the
                    // AttributeConstructionList.
                    NS_LOG_DEBUG("skipping, not settable at construction");
                    continue;
                }
                else
                {
                    // This is an error because this attribute is not
                    // settable in its constructor but is present in
                    // the AttributeConstructionList.
                    NS_FATAL_ERROR("Attribute name="
                                   << info.name << " tid=" << tid.GetName()
                                   << ": initial value cannot be set using attributes");
                }
            }

            if (!value)
            {
                NS_LOG_DEBUG("trying to set from environment variable NS_ATTRIBUTE_DEFAULT");
                auto [found, val] =
                    EnvironmentVariable::Get("NS_ATTRIBUTE_DEFAULT", tid.GetAttributeFullName(i));
                if (found)
                {
                    NS_LOG_DEBUG("found in environment: " << val);
                    value = Create<StringValue>(val);
                    where = "env var";
                }
            }

            bool initial{false};
            if (!value)
            {
                // This is guaranteed to exist
                NS_LOG_DEBUG("falling back to initial value from tid");
                value = info.initialValue;
                where = "initial value";
                initial = true;
            }

            // We have a matching attribute value, if only from the initialValue
            if (DoSet(info.accessor, info.checker, *value) || initial)
            {
                // Setting from initial value may fail, e.g. setting
                // ObjectVectorValue from ""
                // That's ok, so we still report success since construction is complete
                NS_LOG_DEBUG("construct \"" << tid.GetName() << "::" << info.name << "\" from "
                                            << where);
            }
            else
            {
                /*
                  One would think this is an error...

                  but there are cases where `attributes.Find(info.checker)`
                  returns a non-null value which still fails the `DoSet()` call.
                  For example, `value` is sometimes a real `PointerValue`
                  containing 0 as the pointed-to address.  Since value
                  is not null (it just contains null) the initial
                  value is not used, the DoSet fails, and we end up
                  here.

                  If we were adventurous we might try to fix this deep
                  below DoSet, but there be dragons.
                */
                /*
                NS_ASSERT_MSG(false,
                              "Failed to set attribute '" << info.name << "' from '"
                                                          << value->SerializeToString(info.checker)
                                                          << "'");
                */
            }

        } // for i attributes
        tid = tid.GetParent();
    } while (tid != ObjectBase::GetTypeId());
    NotifyConstructionCompleted();
}

bool
ObjectBase::DoSet(Ptr<const AttributeAccessor> accessor,
                  Ptr<const AttributeChecker> checker,
                  const AttributeValue& value)
{
    NS_LOG_FUNCTION(this << accessor << checker << &value);
    Ptr<AttributeValue> v = checker->CreateValidValue(value);
    if (!v)
    {
        return false;
    }
    bool ok = accessor->Set(this, *v);
    return ok;
}

void
ObjectBase::SetAttribute(std::string name, const AttributeValue& value)
{
    NS_LOG_FUNCTION(this << name << &value);
    TypeId::AttributeInformation info;
    TypeId tid = GetInstanceTypeId();
    if (!tid.LookupAttributeByName(name, &info))
    {
        NS_FATAL_ERROR(
            "Attribute name=" << name << " does not exist for this object: tid=" << tid.GetName());
    }
    if (!(info.flags & TypeId::ATTR_SET) || !info.accessor->HasSetter())
    {
        NS_FATAL_ERROR(
            "Attribute name=" << name << " is not settable for this object: tid=" << tid.GetName());
    }
    if (!DoSet(info.accessor, info.checker, value))
    {
        NS_FATAL_ERROR("Attribute name=" << name << " could not be set for this object: tid="
                                         << tid.GetName());
    }
}

bool
ObjectBase::SetAttributeFailSafe(std::string name, const AttributeValue& value)
{
    NS_LOG_FUNCTION(this << name << &value);
    TypeId::AttributeInformation info;
    TypeId tid = GetInstanceTypeId();
    if (!tid.LookupAttributeByName(name, &info))
    {
        return false;
    }
    if (!(info.flags & TypeId::ATTR_SET) || !info.accessor->HasSetter())
    {
        return false;
    }
    return DoSet(info.accessor, info.checker, value);
}

void
ObjectBase::GetAttribute(std::string name, AttributeValue& value) const
{
    NS_LOG_FUNCTION(this << name << &value);
    TypeId::AttributeInformation info;
    TypeId tid = GetInstanceTypeId();
    if (!tid.LookupAttributeByName(name, &info))
    {
        NS_FATAL_ERROR(
            "Attribute name=" << name << " does not exist for this object: tid=" << tid.GetName());
    }
    if (!(info.flags & TypeId::ATTR_GET) || !info.accessor->HasGetter())
    {
        NS_FATAL_ERROR(
            "Attribute name=" << name << " is not gettable for this object: tid=" << tid.GetName());
    }
    bool ok = info.accessor->Get(this, value);
    if (ok)
    {
        return;
    }
    auto str = dynamic_cast<StringValue*>(&value);
    if (str == nullptr)
    {
        NS_FATAL_ERROR("Attribute name=" << name << " tid=" << tid.GetName()
                                         << ": input value is not a string");
    }
    Ptr<AttributeValue> v = info.checker->Create();
    ok = info.accessor->Get(this, *PeekPointer(v));
    if (!ok)
    {
        NS_FATAL_ERROR("Attribute name=" << name << " tid=" << tid.GetName()
                                         << ": could not get value");
    }
    str->Set(v->SerializeToString(info.checker));
}

bool
ObjectBase::GetAttributeFailSafe(std::string name, AttributeValue& value) const
{
    NS_LOG_FUNCTION(this << name << &value);
    TypeId::AttributeInformation info;
    TypeId tid = GetInstanceTypeId();
    if (!tid.LookupAttributeByName(name, &info))
    {
        return false;
    }
    if (!(info.flags & TypeId::ATTR_GET) || !info.accessor->HasGetter())
    {
        return false;
    }
    bool ok = info.accessor->Get(this, value);
    if (ok)
    {
        return true;
    }
    auto str = dynamic_cast<StringValue*>(&value);
    if (str == nullptr)
    {
        return false;
    }
    Ptr<AttributeValue> v = info.checker->Create();
    ok = info.accessor->Get(this, *PeekPointer(v));
    if (!ok)
    {
        return false;
    }
    str->Set(v->SerializeToString(info.checker));
    return true;
}

bool
ObjectBase::TraceConnectWithoutContext(std::string name, const CallbackBase& cb)
{
    NS_LOG_FUNCTION(this << name << &cb);
    TypeId tid = GetInstanceTypeId();
    Ptr<const TraceSourceAccessor> accessor = tid.LookupTraceSourceByName(name);
    if (!accessor)
    {
        NS_LOG_DEBUG("Cannot connect trace " << name << " on object of type " << tid.GetName());
        return false;
    }
    bool ok = accessor->ConnectWithoutContext(this, cb);
    return ok;
}

bool
ObjectBase::TraceConnect(std::string name, std::string context, const CallbackBase& cb)
{
    NS_LOG_FUNCTION(this << name << context << &cb);
    TypeId tid = GetInstanceTypeId();
    Ptr<const TraceSourceAccessor> accessor = tid.LookupTraceSourceByName(name);
    if (!accessor)
    {
        NS_LOG_DEBUG("Cannot connect trace " << name << " on object of type " << tid.GetName());
        return false;
    }
    bool ok = accessor->Connect(this, context, cb);
    return ok;
}

bool
ObjectBase::TraceDisconnectWithoutContext(std::string name, const CallbackBase& cb)
{
    NS_LOG_FUNCTION(this << name << &cb);
    TypeId tid = GetInstanceTypeId();
    Ptr<const TraceSourceAccessor> accessor = tid.LookupTraceSourceByName(name);
    if (!accessor)
    {
        return false;
    }
    bool ok = accessor->DisconnectWithoutContext(this, cb);
    return ok;
}

bool
ObjectBase::TraceDisconnect(std::string name, std::string context, const CallbackBase& cb)
{
    NS_LOG_FUNCTION(this << name << context << &cb);
    TypeId tid = GetInstanceTypeId();
    Ptr<const TraceSourceAccessor> accessor = tid.LookupTraceSourceByName(name);
    if (!accessor)
    {
        return false;
    }
    bool ok = accessor->Disconnect(this, context, cb);
    return ok;
}

} // namespace ns3
