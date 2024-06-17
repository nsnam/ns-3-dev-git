/*
 * Copyright (c) 2009 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@cutebugs.net>
 */

#include "file-config.h"

namespace ns3
{

FileConfig::~FileConfig()
{
}

void
FileConfig::SetSaveDeprecated(bool saveDeprecated)
{
    m_saveDeprecated = saveDeprecated;
}

NoneFileConfig::NoneFileConfig()
{
}

NoneFileConfig::~NoneFileConfig()
{
}

void
NoneFileConfig::SetFilename(std::string filename)
{
}

void
NoneFileConfig::Default()
{
}

void
NoneFileConfig::Global()
{
}

void
NoneFileConfig::Attributes()
{
}

} // namespace ns3
