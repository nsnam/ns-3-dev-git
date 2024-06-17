/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-protection.h"

#include "wifi-utils.h"

namespace ns3
{

/*
 * WifiProtection
 */

WifiProtection::WifiProtection(Method m)
    : method(m)
{
}

WifiProtection::~WifiProtection()
{
}

/*
 * WifiNoProtection
 */

WifiNoProtection::WifiNoProtection()
    : WifiProtection(NONE)
{
    protectionTime = Seconds(0);
}

std::unique_ptr<WifiProtection>
WifiNoProtection::Copy() const
{
    return std::make_unique<WifiNoProtection>(*this);
}

void
WifiNoProtection::Print(std::ostream& os) const
{
    os << "NONE";
}

/*
 * WifiRtsCtsProtection
 */

WifiRtsCtsProtection::WifiRtsCtsProtection()
    : WifiProtection(RTS_CTS)
{
}

std::unique_ptr<WifiProtection>
WifiRtsCtsProtection::Copy() const
{
    return std::make_unique<WifiRtsCtsProtection>(*this);
}

void
WifiRtsCtsProtection::Print(std::ostream& os) const
{
    os << "RTS_CTS";
}

/*
 * WifiCtsToSelfProtection
 */

WifiCtsToSelfProtection::WifiCtsToSelfProtection()
    : WifiProtection(CTS_TO_SELF)
{
}

std::unique_ptr<WifiProtection>
WifiCtsToSelfProtection::Copy() const
{
    return std::make_unique<WifiCtsToSelfProtection>(*this);
}

void
WifiCtsToSelfProtection::Print(std::ostream& os) const
{
    os << "CTS_TO_SELF";
}

/*
 * WifiMuRtsCtsProtection
 */

WifiMuRtsCtsProtection::WifiMuRtsCtsProtection()
    : WifiProtection(MU_RTS_CTS)
{
}

std::unique_ptr<WifiProtection>
WifiMuRtsCtsProtection::Copy() const
{
    return std::make_unique<WifiMuRtsCtsProtection>(*this);
}

void
WifiMuRtsCtsProtection::Print(std::ostream& os) const
{
    os << "MU_RTS_CTS";
}

std::ostream&
operator<<(std::ostream& os, const WifiProtection* protection)
{
    protection->Print(os);
    return os;
}

} // namespace ns3
