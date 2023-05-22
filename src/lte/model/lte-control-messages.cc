/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
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
 * Author: Giuseppe Piro  <g.piro@poliba.it>
 *         Marco Miozzo <marco.miozzo@cttc.es>
 */

#include "lte-control-messages.h"

#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteControlMessage");

LteControlMessage::LteControlMessage()
{
}

LteControlMessage::~LteControlMessage()
{
}

void
LteControlMessage::SetMessageType(LteControlMessage::MessageType type)
{
    m_type = type;
}

LteControlMessage::MessageType
LteControlMessage::GetMessageType()
{
    return m_type;
}

// ----------------------------------------------------------------------------------------------------------

DlDciLteControlMessage::DlDciLteControlMessage()
{
    SetMessageType(LteControlMessage::DL_DCI);
}

DlDciLteControlMessage::~DlDciLteControlMessage()
{
}

void
DlDciLteControlMessage::SetDci(DlDciListElement_s dci)
{
    m_dci = dci;
}

const DlDciListElement_s&
DlDciLteControlMessage::GetDci()
{
    return m_dci;
}

// ----------------------------------------------------------------------------------------------------------

UlDciLteControlMessage::UlDciLteControlMessage()
{
    SetMessageType(LteControlMessage::UL_DCI);
}

UlDciLteControlMessage::~UlDciLteControlMessage()
{
}

void
UlDciLteControlMessage::SetDci(UlDciListElement_s dci)
{
    m_dci = dci;
}

const UlDciListElement_s&
UlDciLteControlMessage::GetDci()
{
    return m_dci;
}

// ----------------------------------------------------------------------------------------------------------

DlCqiLteControlMessage::DlCqiLteControlMessage()
{
    SetMessageType(LteControlMessage::DL_CQI);
}

DlCqiLteControlMessage::~DlCqiLteControlMessage()
{
}

void
DlCqiLteControlMessage::SetDlCqi(CqiListElement_s dlcqi)
{
    m_dlCqi = dlcqi;
}

CqiListElement_s
DlCqiLteControlMessage::GetDlCqi()
{
    return m_dlCqi;
}

// ----------------------------------------------------------------------------------------------------------

BsrLteControlMessage::BsrLteControlMessage()
{
    SetMessageType(LteControlMessage::BSR);
}

BsrLteControlMessage::~BsrLteControlMessage()
{
}

void
BsrLteControlMessage::SetBsr(MacCeListElement_s bsr)
{
    m_bsr = bsr;
}

MacCeListElement_s
BsrLteControlMessage::GetBsr()
{
    return m_bsr;
}

// ----------------------------------------------------------------------------------------------------------

RachPreambleLteControlMessage::RachPreambleLteControlMessage()
{
    SetMessageType(LteControlMessage::RACH_PREAMBLE);
}

void
RachPreambleLteControlMessage::SetRapId(uint32_t rapId)
{
    m_rapId = rapId;
}

uint32_t
RachPreambleLteControlMessage::GetRapId() const
{
    return m_rapId;
}

// ----------------------------------------------------------------------------------------------------------

RarLteControlMessage::RarLteControlMessage()
{
    SetMessageType(LteControlMessage::RAR);
}

void
RarLteControlMessage::SetRaRnti(uint16_t raRnti)
{
    m_raRnti = raRnti;
}

uint16_t
RarLteControlMessage::GetRaRnti() const
{
    return m_raRnti;
}

void
RarLteControlMessage::AddRar(Rar rar)
{
    m_rarList.push_back(rar);
}

std::list<RarLteControlMessage::Rar>::const_iterator
RarLteControlMessage::RarListBegin() const
{
    return m_rarList.begin();
}

std::list<RarLteControlMessage::Rar>::const_iterator
RarLteControlMessage::RarListEnd() const
{
    return m_rarList.end();
}

// ----------------------------------------------------------------------------------------------------------

MibLteControlMessage::MibLteControlMessage()
{
    SetMessageType(LteControlMessage::MIB);
}

void
MibLteControlMessage::SetMib(LteRrcSap::MasterInformationBlock mib)
{
    m_mib = mib;
}

LteRrcSap::MasterInformationBlock
MibLteControlMessage::GetMib() const
{
    return m_mib;
}

// ----------------------------------------------------------------------------------------------------------

Sib1LteControlMessage::Sib1LteControlMessage()
{
    SetMessageType(LteControlMessage::SIB1);
}

void
Sib1LteControlMessage::SetSib1(LteRrcSap::SystemInformationBlockType1 sib1)
{
    m_sib1 = sib1;
}

LteRrcSap::SystemInformationBlockType1
Sib1LteControlMessage::GetSib1() const
{
    return m_sib1;
}

// ---------------------------------------------------------------------------

DlHarqFeedbackLteControlMessage::DlHarqFeedbackLteControlMessage()
{
    SetMessageType(LteControlMessage::DL_HARQ);
}

DlHarqFeedbackLteControlMessage::~DlHarqFeedbackLteControlMessage()
{
}

void
DlHarqFeedbackLteControlMessage::SetDlHarqFeedback(DlInfoListElement_s m)
{
    m_dlInfoListElement = m;
}

DlInfoListElement_s
DlHarqFeedbackLteControlMessage::GetDlHarqFeedback()
{
    return m_dlInfoListElement;
}

} // namespace ns3
