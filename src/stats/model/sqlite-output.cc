/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Natale Patriciello <natale.patriciello@gmail.com>
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
 */
#include "sqlite-output.h"
#include <sys/stat.h>
#include <fcntl.h>
#include "ns3/abort.h"
#include "ns3/unused.h"
#include "ns3/log.h"
#include "ns3/nstime.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SQLiteOutput");

SQLiteOutput::SQLiteOutput (const std::string &name, const std::string &semName)
  : m_semName (semName)
{
  int rc = sqlite3_open (name.c_str (), &m_db);
  NS_ABORT_MSG_UNLESS (rc == SQLITE_OK, "Failed to open DB");
}

SQLiteOutput::~SQLiteOutput ()
{
  int rc = SQLITE_FAIL;

  rc = sqlite3_close_v2 (m_db);
  NS_ABORT_MSG_UNLESS (rc == SQLITE_OK, "Failed to close DB");
}

void
SQLiteOutput::SetJournalInMemory ()
{
  NS_LOG_FUNCTION (this);
  SpinExec ("PRAGMA journal_mode = MEMORY");
}

bool
SQLiteOutput::SpinExec (const std::string &cmd) const
{
  return (SpinExec (m_db, cmd) == SQLITE_OK);
}

bool
SQLiteOutput::SpinExec (sqlite3_stmt *stmt) const
{
  int rc = SpinExec (m_db, stmt);
  return !CheckError (m_db, rc, "", nullptr, false);
}

bool
SQLiteOutput::WaitExec (const std::string &cmd) const
{
  int rc = WaitExec (m_db, cmd);
  return !CheckError (m_db, rc, cmd, nullptr, false);
}

bool
SQLiteOutput::WaitExec (sqlite3_stmt *stmt) const
{
  return (WaitExec (m_db, stmt) == SQLITE_OK);
}

bool
SQLiteOutput::WaitPrepare (sqlite3_stmt **stmt, const std::string &cmd) const
{
  return (WaitPrepare (m_db, stmt, cmd) == SQLITE_OK);
}

bool
SQLiteOutput::SpinPrepare (sqlite3_stmt **stmt, const std::string &cmd) const
{
  return (SpinPrepare (m_db, stmt, cmd) == SQLITE_OK);
}

template<typename T>
T
SQLiteOutput::RetrieveColumn (sqlite3_stmt *stmt, int pos) const
{
  NS_UNUSED (stmt);
  NS_UNUSED (pos);
  NS_FATAL_ERROR ("Can't call generic fn");
}

template<>
int
SQLiteOutput::RetrieveColumn (sqlite3_stmt *stmt, int pos) const
{
  return sqlite3_column_int (stmt, pos);
}

template<>
uint32_t
SQLiteOutput::RetrieveColumn (sqlite3_stmt *stmt, int pos) const
{
  return static_cast<uint32_t> (sqlite3_column_int (stmt, pos));
}

template<>
double
SQLiteOutput::RetrieveColumn (sqlite3_stmt *stmt, int pos) const
{
  return sqlite3_column_double (stmt, pos);
}

template<typename T>
bool
SQLiteOutput::Bind (sqlite3_stmt *stmt, int pos, const T &value) const
{
  NS_UNUSED (stmt);
  NS_UNUSED (pos);
  NS_UNUSED (value);
  NS_FATAL_ERROR ("Can't call generic fn");
  return false;
}

template<>
bool
SQLiteOutput::Bind (sqlite3_stmt *stmt, int pos, const Time &value) const
{
  if (sqlite3_bind_double (stmt, pos, value.GetSeconds ()) == SQLITE_OK)
    {
      return true;
    }
  return false;
}

template<>
bool
SQLiteOutput::Bind (sqlite3_stmt *stmt, int pos, const double &value) const
{
  if (sqlite3_bind_double (stmt, pos, value) == SQLITE_OK)
    {
      return true;
    }
  return false;
}

template<>
bool
SQLiteOutput::Bind (sqlite3_stmt *stmt, int pos, const uint32_t &value) const
{
  if (sqlite3_bind_int (stmt, pos, static_cast<int> (value)) == SQLITE_OK)
    {
      return true;
    }
  return false;
}

template<>
bool
SQLiteOutput::Bind (sqlite3_stmt *stmt, int pos, const long &value) const
{
  if (sqlite3_bind_int64 (stmt, pos, value) == SQLITE_OK)
    {
      return true;
    }
  return false;
}

template<>
bool
SQLiteOutput::Bind (sqlite3_stmt *stmt, int pos, const long long &value) const
{
  if (sqlite3_bind_int64 (stmt, pos, value) == SQLITE_OK)
    {
      return true;
    }
  return false;
}

template<>
bool
SQLiteOutput::Bind (sqlite3_stmt *stmt, int pos, const uint16_t &value) const
{
  if (sqlite3_bind_int (stmt, pos, static_cast<int> (value)) == SQLITE_OK)
    {
      return true;
    }
  return false;
}

template<>
bool
SQLiteOutput::Bind (sqlite3_stmt *stmt, int pos, const uint8_t &value) const
{
  if (sqlite3_bind_int (stmt, pos, static_cast<int> (value)) == SQLITE_OK)
    {
      return true;
    }
  return false;
}

template<>
bool
SQLiteOutput::Bind (sqlite3_stmt *stmt, int pos, const int &value) const
{
  if (sqlite3_bind_int (stmt, pos, value) == SQLITE_OK)
    {
      return true;
    }
  return false;
}

template<>
bool
SQLiteOutput::Bind (sqlite3_stmt *stmt, int pos, const std::string &value) const
{
  if (sqlite3_bind_text (stmt, pos, value.c_str (), -1, SQLITE_STATIC) == SQLITE_OK)
    {
      return true;
    }
  return false;
}

int
SQLiteOutput::WaitPrepare (sqlite3 *db, sqlite3_stmt **stmt, const std::string &cmd) const
{
  int rc;
  bool ret;
  sem_t *sem = sem_open (m_semName.c_str (), O_CREAT, S_IRUSR | S_IWUSR, 1);

  NS_ABORT_MSG_IF (sem == SEM_FAILED,
                   "FAILED to open system semaphore, errno: " << errno);

  if (sem_wait (sem) == 0)
    {
      rc = sqlite3_prepare_v2 (db, cmd.c_str (),
                               static_cast<int> (cmd.size ()),
                               stmt, nullptr);

      ret = CheckError (db, rc, cmd, sem, false);
      if (ret)
        {
          return rc;
        }

      sem_post (sem);
    }
  else
    {
      NS_FATAL_ERROR ("Can't lock semaphore");
    }

  sem_close (sem);
  sem = nullptr;

  return rc;
}

int
SQLiteOutput::SpinPrepare (sqlite3 *db, sqlite3_stmt **stmt, const std::string &cmd)
{
  int rc;

  do
    {
      rc = sqlite3_prepare_v2 (db, cmd.c_str (),
                               static_cast<int> (cmd.size ()),
                               stmt, nullptr);
    }
  while (rc == SQLITE_BUSY || rc == SQLITE_LOCKED);
  return rc;
}

int
SQLiteOutput::SpinStep (sqlite3_stmt *stmt)
{
  int rc;
  do
    {
      rc = sqlite3_step (stmt);
    }
  while (rc == SQLITE_BUSY || rc == SQLITE_LOCKED);

  return rc;
}

int
SQLiteOutput::SpinFinalize (sqlite3_stmt *stmt)
{
  int rc;
  do
    {
      rc = sqlite3_finalize (stmt);
    }
  while (rc == SQLITE_BUSY || rc == SQLITE_LOCKED);

  return rc;
}

int SQLiteOutput::SpinReset (sqlite3_stmt *stmt)
{
  int rc;

  do
    {
      rc = sqlite3_reset (stmt);
    }
  while (rc == SQLITE_BUSY || rc == SQLITE_LOCKED);

  return rc;
}

void
SQLiteOutput::Error (sqlite3 *db, const std::string &cmd)
{
  NS_ABORT_MSG (cmd << " error " << sqlite3_errmsg (db));
}

bool
SQLiteOutput::CheckError (sqlite3 *db, int rc, const std::string &cmd,
                          sem_t *sem, bool hardExit)
{
  if (rc != SQLITE_OK && rc != SQLITE_DONE)
    {
      if (sem != nullptr)
        {
          sem_post (sem);
          sem_close (sem);
        }
      if (hardExit)
        {
          Error (db, cmd);
        }
      else
        {
          std::cerr << sqlite3_errmsg (db) << std::endl;
        }
      return true;
    }
  return false;
}

int
SQLiteOutput::SpinExec (sqlite3 *db, const std::string &cmd)
{
  sqlite3_stmt *stmt;
  bool ret;

  int rc = SpinPrepare (db, &stmt, cmd);
  ret = CheckError (db, rc, cmd, nullptr, false);
  if (ret)
    {
      return rc;
    }

  rc = SpinStep (stmt);
  ret = CheckError (db, rc, cmd, nullptr, false);
  if (ret)
    {
      return rc;
    }

  rc = SpinFinalize (stmt);
  CheckError (db, rc, cmd, nullptr, false);

  return rc;
}

int
SQLiteOutput::SpinExec (sqlite3 *db, sqlite3_stmt *stmt)
{
  bool ret;
  int rc = SpinStep (stmt);
  ret = CheckError (db, rc, "", nullptr, false);
  if (ret)
    {
      return rc;
    }

  rc = SpinFinalize (stmt);
  return rc;
}

int
SQLiteOutput::WaitExec (sqlite3 *db, sqlite3_stmt *stmt) const
{
  bool ret;
  int rc = SQLITE_ERROR;

  sem_t *sem = sem_open (m_semName.c_str (), O_CREAT, S_IRUSR | S_IWUSR, 1);

  NS_ABORT_MSG_IF (sem == SEM_FAILED,
                   "FAILED to open system semaphore, errno: " << errno);

  if (sem_wait (sem) == 0)
    {
      rc = SpinStep (stmt);

      ret = CheckError (db, rc, "", sem, false);
      if (ret)
        {
          return rc;
        }

      rc = SpinFinalize (stmt);

      sem_post (sem);
    }
  else
    {
      NS_FATAL_ERROR ("Can't lock system semaphore");
    }

  sem_close (sem);

  return rc;
}

int
SQLiteOutput::WaitExec (sqlite3 *db, const std::string &cmd) const
{
  sqlite3_stmt *stmt;
  bool ret;
  int rc = SQLITE_ERROR;

  sem_t *sem = sem_open (m_semName.c_str (), O_CREAT, S_IRUSR | S_IWUSR, 1);

  NS_ABORT_MSG_IF (sem == SEM_FAILED,
                   "FAILED to open system semaphore, errno: " << errno);

  if (sem_wait (sem) == 0)
    {
      rc = SpinPrepare (db, &stmt, cmd);
      ret = CheckError (db, rc, cmd, sem, false);
      if (ret)
        {
          return rc;
        }

      rc = SpinStep (stmt);

      ret = CheckError (db, rc, cmd, sem, false);
      if (ret)
        {
          return rc;
        }

      rc = SpinFinalize (stmt);

      sem_post (sem);
    }

  sem_close (sem);

  return rc;
}

} // namespace ns3;
