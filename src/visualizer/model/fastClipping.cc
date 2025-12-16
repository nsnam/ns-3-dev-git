/*
 * Copyright (c) 2008 INESC Porto
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */

#include "fastClipping.h"

namespace ns3
{

FastClipping::FastClipping(Vector2 clipMin, Vector2 clipMax)
    : m_clipMin(clipMin),
      m_clipMax(clipMax)
{
}

void
FastClipping::ClipStartTop(Line& line) const
{
    line.start.x += line.dx * (m_clipMin.y - line.start.y) / line.dy;
    line.start.y = m_clipMin.y;
}

void
FastClipping::ClipStartBottom(Line& line) const
{
    line.start.x += line.dx * (m_clipMax.y - line.start.y) / line.dy;
    line.start.y = m_clipMax.y;
}

void
FastClipping::ClipStartRight(Line& line) const
{
    line.start.y += line.dy * (m_clipMax.x - line.start.x) / line.dx;
    line.start.x = m_clipMax.x;
}

void
FastClipping::ClipStartLeft(Line& line) const
{
    line.start.y += line.dy * (m_clipMin.x - line.start.x) / line.dx;
    line.start.x = m_clipMin.x;
}

void
FastClipping::ClipEndTop(Line& line) const
{
    line.end.x += line.dx * (m_clipMin.y - line.end.y) / line.dy;
    line.end.y = m_clipMin.y;
}

void
FastClipping::ClipEndBottom(Line& line) const
{
    line.end.x += line.dx * (m_clipMax.y - line.end.y) / line.dy;
    line.end.y = m_clipMax.y;
}

void
FastClipping::ClipEndRight(Line& line) const
{
    line.end.y += line.dy * (m_clipMax.x - line.end.x) / line.dx;
    line.end.x = m_clipMax.x;
}

void
FastClipping::ClipEndLeft(Line& line) const
{
    line.end.y += line.dy * (m_clipMin.x - line.end.x) / line.dx;
    line.end.x = m_clipMin.x;
}

bool
FastClipping::ClipLine(Line& line)
{
    uint8_t lineCode = 0;

    if (line.end.y < m_clipMin.y)
    {
        lineCode |= 8;
    }
    else if (line.end.y > m_clipMax.y)
    {
        lineCode |= 4;
    }

    if (line.end.x > m_clipMax.x)
    {
        lineCode |= 2;
    }
    else if (line.end.x < m_clipMin.x)
    {
        lineCode |= 1;
    }

    if (line.start.y < m_clipMin.y)
    {
        lineCode |= 128;
    }
    else if (line.start.y > m_clipMax.y)
    {
        lineCode |= 64;
    }

    if (line.start.x > m_clipMax.x)
    {
        lineCode |= 32;
    }
    else if (line.start.x < m_clipMin.x)
    {
        lineCode |= 16;
    }

    // 9 - 8 - A
    // |   |   |
    // 1 - 0 - 2
    // |   |   |
    // 5 - 4 - 6
    switch (lineCode)
    {
    // center
    case 0x00:
        return true;

    case 0x01:
        ClipEndLeft(line);
        return true;

    case 0x02:
        ClipEndRight(line);
        return true;

    case 0x04:
        ClipEndBottom(line);
        return true;

    case 0x05:
        ClipEndLeft(line);
        if (line.end.y > m_clipMax.y)
        {
            ClipEndBottom(line);
        }
        return true;

    case 0x06:
        ClipEndRight(line);
        if (line.end.y > m_clipMax.y)
        {
            ClipEndBottom(line);
        }
        return true;

    case 0x08:
        ClipEndTop(line);
        return true;

    case 0x09:
        ClipEndLeft(line);
        if (line.end.y < m_clipMin.y)
        {
            ClipEndTop(line);
        }
        return true;

    case 0x0A:
        ClipEndRight(line);
        if (line.end.y < m_clipMin.y)
        {
            ClipEndTop(line);
        }
        return true;

    // left
    case 0x10:
        ClipStartLeft(line);
        return true;

    case 0x12:
        ClipStartLeft(line);
        ClipEndRight(line);
        return true;

    case 0x14:
        ClipStartLeft(line);
        if (line.start.y > m_clipMax.y)
        {
            return false;
        }
        ClipEndBottom(line);
        return true;

    case 0x16:
        ClipStartLeft(line);
        if (line.start.y > m_clipMax.y)
        {
            return false;
        }
        ClipEndBottom(line);
        if (line.end.x > m_clipMax.x)
        {
            ClipEndRight(line);
        }
        return true;

    case 0x18:
        ClipStartLeft(line);
        if (line.start.y < m_clipMin.y)
        {
            return false;
        }
        ClipEndTop(line);
        return true;

    case 0x1A:
        ClipStartLeft(line);
        if (line.start.y < m_clipMin.y)
        {
            return false;
        }
        ClipEndTop(line);
        if (line.end.x > m_clipMax.x)
        {
            ClipEndRight(line);
        }
        return true;

    // right
    case 0x20:
        ClipStartRight(line);
        return true;

    case 0x21:
        ClipStartRight(line);
        ClipEndLeft(line);
        return true;

    case 0x24:
        ClipStartRight(line);
        if (line.start.y > m_clipMax.y)
        {
            return false;
        }
        ClipEndBottom(line);
        return true;

    case 0x25:
        ClipStartRight(line);
        if (line.start.y > m_clipMax.y)
        {
            return false;
        }
        ClipEndBottom(line);
        if (line.end.x < m_clipMin.x)
        {
            ClipEndLeft(line);
        }
        return true;

    case 0x28:
        ClipStartRight(line);
        if (line.start.y < m_clipMin.y)
        {
            return false;
        }
        ClipEndTop(line);
        return true;

    case 0x29:
        ClipStartRight(line);
        if (line.start.y < m_clipMin.y)
        {
            return false;
        }
        ClipEndTop(line);
        if (line.end.x < m_clipMin.x)
        {
            ClipEndLeft(line);
        }
        return true;

    // bottom
    case 0x40:
        ClipStartBottom(line);
        return true;

    case 0x41:
        ClipStartBottom(line);
        if (line.start.x < m_clipMin.x)
        {
            return false;
        }
        ClipEndLeft(line);
        if (line.end.y > m_clipMax.y)
        {
            ClipEndBottom(line);
        }
        return true;

    case 0x42:
        ClipStartBottom(line);
        if (line.start.x > m_clipMax.x)
        {
            return false;
        }
        ClipEndRight(line);
        return true;

    case 0x48:
        ClipStartBottom(line);
        ClipEndTop(line);
        return true;

    case 0x49:
        ClipStartBottom(line);
        if (line.start.x < m_clipMin.x)
        {
            return false;
        }
        ClipEndLeft(line);
        if (line.end.y < m_clipMin.y)
        {
            ClipEndTop(line);
        }
        return true;

    case 0x4A:
        ClipStartBottom(line);
        if (line.start.x > m_clipMax.x)
        {
            return false;
        }
        ClipEndRight(line);
        if (line.end.y < m_clipMin.y)
        {
            ClipEndTop(line);
        }
        return true;

    // bottom-left
    case 0x50:
        ClipStartLeft(line);
        if (line.start.y > m_clipMax.y)
        {
            ClipStartBottom(line);
        }
        return true;

    case 0x52:
        ClipEndRight(line);
        if (line.end.y > m_clipMax.y)
        {
            return false;
        }
        ClipStartBottom(line);
        if (line.start.x < m_clipMin.x)
        {
            ClipStartLeft(line);
        }
        return true;

    case 0x58:
        ClipEndTop(line);
        if (line.end.x < m_clipMin.x)
        {
            return false;
        }
        ClipStartBottom(line);
        if (line.start.x < m_clipMin.x)
        {
            ClipStartLeft(line);
        }
        return true;

    case 0x5A:
        ClipStartLeft(line);
        if (line.start.y < m_clipMin.y)
        {
            return false;
        }
        ClipEndRight(line);
        if (line.end.y > m_clipMax.y)
        {
            return false;
        }
        if (line.start.y > m_clipMax.y)
        {
            ClipStartBottom(line);
        }
        if (line.end.y < m_clipMin.y)
        {
            ClipEndTop(line);
        }
        return true;

    // bottom-right
    case 0x60:
        ClipStartRight(line);
        if (line.start.y > m_clipMax.y)
        {
            ClipStartBottom(line);
        }
        return true;

    case 0x61:
        ClipEndLeft(line);
        if (line.end.y > m_clipMax.y)
        {
            return false;
        }
        ClipStartBottom(line);
        if (line.start.x > m_clipMax.x)
        {
            ClipStartRight(line);
        }
        return true;

    case 0x68:
        ClipEndTop(line);
        if (line.end.x > m_clipMax.x)
        {
            return false;
        }
        ClipStartRight(line);
        if (line.start.y > m_clipMax.y)
        {
            ClipStartBottom(line);
        }
        return true;

    case 0x69:
        ClipEndLeft(line);
        if (line.end.y > m_clipMax.y)
        {
            return false;
        }
        ClipStartRight(line);
        if (line.start.y < m_clipMin.y)
        {
            return false;
        }
        if (line.end.y < m_clipMin.y)
        {
            ClipEndTop(line);
        }
        if (line.start.y > m_clipMax.y)
        {
            ClipStartBottom(line);
        }
        return true;

    // top
    case 0x80:
        ClipStartTop(line);
        return true;

    case 0x81:
        ClipStartTop(line);
        if (line.start.x < m_clipMin.x)
        {
            return false;
        }
        ClipEndLeft(line);
        return true;

    case 0x82:
        ClipStartTop(line);
        if (line.start.x > m_clipMax.x)
        {
            return false;
        }
        ClipEndRight(line);
        return true;

    case 0x84:
        ClipStartTop(line);
        ClipEndBottom(line);
        return true;

    case 0x85:
        ClipStartTop(line);
        if (line.start.x < m_clipMin.x)
        {
            return false;
        }
        ClipEndLeft(line);
        if (line.end.y > m_clipMax.y)
        {
            ClipEndBottom(line);
        }
        return true;

    case 0x86:
        ClipStartTop(line);
        if (line.start.x > m_clipMax.x)
        {
            return false;
        }
        ClipEndRight(line);
        if (line.end.y > m_clipMax.y)
        {
            ClipEndBottom(line);
        }
        return true;

    // top-left
    case 0x90:
        ClipStartLeft(line);
        if (line.start.y < m_clipMin.y)
        {
            ClipStartTop(line);
        }
        return true;

    case 0x92:
        ClipEndRight(line);
        if (line.end.y < m_clipMin.y)
        {
            return false;
        }
        ClipStartTop(line);
        if (line.start.x < m_clipMin.x)
        {
            ClipStartLeft(line);
        }
        return true;

    case 0x94:
        ClipEndBottom(line);
        if (line.end.x < m_clipMin.x)
        {
            return false;
        }
        ClipStartLeft(line);
        if (line.start.y < m_clipMin.y)
        {
            ClipStartTop(line);
        }
        return true;

    case 0x96:
        ClipStartLeft(line);
        if (line.start.y > m_clipMax.y)
        {
            return false;
        }
        ClipEndRight(line);
        if (line.end.y < m_clipMin.y)
        {
            return false;
        }
        if (line.start.y < m_clipMin.y)
        {
            ClipStartTop(line);
        }
        if (line.end.y > m_clipMax.y)
        {
            ClipEndBottom(line);
        }
        return true;

    // top-right
    case 0xA0:
        ClipStartRight(line);
        if (line.start.y < m_clipMin.y)
        {
            ClipStartTop(line);
        }
        return true;

    case 0xA1:
        ClipEndLeft(line);
        if (line.end.y < m_clipMin.y)
        {
            return false;
        }
        ClipStartTop(line);
        if (line.start.x > m_clipMax.x)
        {
            ClipStartRight(line);
        }
        return true;

    case 0xA4:
        ClipEndBottom(line);
        if (line.end.x > m_clipMax.x)
        {
            return false;
        }
        ClipStartRight(line);
        if (line.start.y < m_clipMin.y)
        {
            ClipStartTop(line);
        }
        return true;

    case 0xA5:
        ClipEndLeft(line);
        if (line.end.y < m_clipMin.y)
        {
            return false;
        }
        ClipStartRight(line);
        if (line.start.y > m_clipMax.y)
        {
            return false;
        }
        if (line.end.y > m_clipMax.y)
        {
            ClipEndBottom(line);
        }
        if (line.start.y < m_clipMin.y)
        {
            ClipStartTop(line);
        }
        return true;
    }

    return false;
}

} // namespace ns3
