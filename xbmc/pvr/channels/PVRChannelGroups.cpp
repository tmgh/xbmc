/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "FileItem.h"
#include "settings/GUISettings.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "URL.h"
#include "filesystem/File.h"
#include "music/tags/MusicInfoTag.h"

#include "PVRChannelGroupsContainer.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"

using namespace PVR;

CPVRChannelGroups::CPVRChannelGroups(bool bRadio)
{
  m_bRadio = bRadio;
}

CPVRChannelGroups::~CPVRChannelGroups(void)
{
  Clear();
}

void CPVRChannelGroups::Clear(void)
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
    delete at(iGroupPtr);

  clear();
}

bool CPVRChannelGroups::GetGroupsFromClients(void)
{
  /* get new groups from add-ons */
  PVR_ERROR error;
  CPVRChannelGroups groupsTmp(m_bRadio);
  g_PVRClients->GetChannelGroups(&groupsTmp, &error);
  return UpdateGroupsEntries(groupsTmp);
}

bool CPVRChannelGroups::UpdateFromClient(const CPVRChannelGroup &group)
{
  CPVRChannelGroup *newGroup = new CPVRChannelGroup(group.IsRadio(), -1, group.GroupName(), group.SortOrder());
  push_back(newGroup);

  return true;
}

bool CPVRChannelGroups::Update(const CPVRChannelGroup &group, bool bSaveInDb)
{
  CSingleLock lock(m_critSection);

  int iIndex = -1;
  /* try to find the group by id */
  if (group.GroupID() > 0)
    iIndex = GetIndexForGroupID(group.GroupID());
  /* try to find the group by name if we didn't find it yet */
  if (iIndex < 0)
    iIndex = GetIndexForGroupName(group.GroupName());

  if (iIndex < 0)
  {
    CPVRChannelGroup *newGroup = new CPVRChannelGroup(m_bRadio, group.GroupID(), group.GroupName(), group.SortOrder());
    if (bSaveInDb)
      newGroup->Persist();

    push_back(newGroup);
  }
  else
  {
    at(iIndex)->SetGroupID(group.GroupID());
    at(iIndex)->SetGroupName(group.GroupName());
    at(iIndex)->SetSortOrder(group.SortOrder());

    if (bSaveInDb)
      at(iIndex)->Persist();
  }

  return true;
}

const CPVRChannelGroup *CPVRChannelGroups::GetById(int iGroupId) const
{
  const CPVRChannelGroup *group = NULL;

  if (iGroupId == (m_bRadio ? XBMC_INTERNAL_GROUP_RADIO : XBMC_INTERNAL_GROUP_TV))
  {
    group = GetGroupAll();
  }
  else if (iGroupId > -1)
  {
    int iGroupIndex = GetIndexForGroupID(iGroupId);
    if (iGroupIndex != -1)
      group = at(iGroupIndex);
  }

  return group;
}

const CPVRChannelGroup *CPVRChannelGroups::GetByName(const CStdString &strName) const
{
  CPVRChannelGroup *group = NULL;

  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
  {
    if (at(iGroupPtr)->GroupName().Equals(strName))
    {
      group = at(iGroupPtr);
      break;
    }
  }

  return group;
}

int CPVRChannelGroups::GetIndexForGroupID(int iGroupId) const
{
  int iReturn = -1;

  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
  {
    if (at(iGroupPtr)->GroupID() == iGroupId)
    {
      iReturn = iGroupPtr;
      break;
    }
  }

  return iReturn;
}

int CPVRChannelGroups::GetIndexForGroupName(const CStdString &strName) const
{
  int iReturn = -1;

  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
  {
    if (at(iGroupPtr)->GroupName().Equals(strName))
    {
      iReturn = iGroupPtr;
      break;
    }
  }

  return iReturn;
}

void CPVRChannelGroups::RemoveFromAllGroups(CPVRChannel *channel)
{
  /* start at position 2 because channels are only deleted from non-system groups.
   system groups are entries 0 and 1 */
  for (unsigned int iGroupPtr = 2; iGroupPtr < size(); iGroupPtr++)
  {
    CPVRChannelGroup *group = (CPVRChannelGroup *) at(iGroupPtr);
    group->RemoveFromGroup(channel);
  }
}

bool CPVRChannelGroups::Update(bool bChannelsOnly /* = false */)
{
  bool bReturn = true;

  if (!bChannelsOnly)
    GetGroupsFromClients();

  /* only update the internal group if group syncing is disabled */
  unsigned int iUpdateGroups = !bChannelsOnly && g_guiSettings.GetBool("pvrmanager.syncchannelgroups") ? size() : 1;

  /* system groups are updated first, so new channels are added before anything is done with user defined groups */
  for (unsigned int iGroupPtr = 0; iGroupPtr < iUpdateGroups; iGroupPtr++)
    bReturn = at(iGroupPtr)->Update() && bReturn;

  return bReturn;
}

bool CPVRChannelGroups::UpdateGroupsEntries(const CPVRChannelGroups &groups)
{
  /* go through the groups list and check for new groups */
  for (unsigned int iGroupPtr = 0; iGroupPtr < groups.size(); iGroupPtr++)
  {
    CPVRChannelGroup *group = groups.at(iGroupPtr);

    /* check if this group is present in this container */
    CPVRChannelGroup *existingGroup = (CPVRChannelGroup *) GetByName(group->GroupName());
    if (existingGroup == NULL)
    {
      CPVRChannelGroup *newGroup = new CPVRChannelGroup(m_bRadio);
      newGroup->SetGroupName(group->GroupName());
      newGroup->SetSortOrder(group->SortOrder());

      if (newGroup->Persist())
      {
        push_back(newGroup);
        CLog::Log(LOGDEBUG, "PVRChannelGroups - %s - new %s channel group '%s', id %d",
            __FUNCTION__, m_bRadio ? "radio" : "TV", newGroup->GroupName().c_str(), newGroup->GroupID());
      }
      else
      {
        CLog::Log(LOGERROR, "PVRChannelGroups - %s - couldn't persist new %s channel group '%s'",
            __FUNCTION__, m_bRadio ? "radio" : "TV", newGroup->GroupName().c_str());
      }
    }
  }

  return true;
}

bool CPVRChannelGroups::LoadUserDefinedChannelGroups(void)
{
  CPVRDatabase *database = OpenPVRDatabase();
  if (!database)
    return false;

  /* load the other groups from the database */
  database->Get(*this);
  int iSize = size();
  CLog::Log(LOGDEBUG, "PVRChannelGroups - %s - %d user defined groups %s fetched from the database",
      __FUNCTION__, iSize - 1, m_bRadio ? "radio" : "TV");

  GetGroupsFromClients();
  CLog::Log(LOGDEBUG, "PVRChannelGroups - %s - %d user defined groups %s fetched from clients",
      __FUNCTION__, (int) (size() - iSize - 1), m_bRadio ? "radio" : "TV");

  /* load group members */
  for (unsigned int iGroupPtr = 1; iGroupPtr < size(); iGroupPtr++)
    at(iGroupPtr)->Load();

  return true;
}

bool CPVRChannelGroups::Load(void)
{
  CLog::Log(LOGDEBUG, "PVRChannelGroups - %s - loading all %s channel groups",
      __FUNCTION__, m_bRadio ? "radio" : "TV");

  Clear();

  /* create and load the internal channel group */
  CPVRChannelGroupInternal *internalChannels = new CPVRChannelGroupInternal(m_bRadio);
  push_back(internalChannels);
  internalChannels->Load();

  /* load the other groups from the database */
  LoadUserDefinedChannelGroups();

  CLog::Log(LOGDEBUG, "PVRChannelGroups - %s - %d %s channel groups loaded",
      __FUNCTION__, (int) size(), m_bRadio ? "radio" : "TV");

  return size() > 0;
}

bool CPVRChannelGroups::PersistAll(void)
{
  bool bReturn = true;

  CLog::Log(LOGDEBUG, "CPVRChannelGroups - %s - persisting all changes in channel groups", __FUNCTION__);

  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
    bReturn = at(iGroupPtr)->Persist() && bReturn;

  return bReturn;
}

CPVRChannelGroupInternal *CPVRChannelGroups::GetGroupAll(void) const
{
  if (size() > 0)
    return (CPVRChannelGroupInternal *) at(0);
  else
    return NULL;
}

int CPVRChannelGroups::GetGroupList(CFileItemList* results) const
{
  int iReturn = 0;

  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
  {
    CFileItemPtr group(new CFileItem(at(iGroupPtr)->GroupName()));
    group->m_strTitle = at(iGroupPtr)->GroupName();
    group->m_strPath.Format("%i", at(iGroupPtr)->GroupID());
    results->Add(group);
    ++iReturn;
  }

  return iReturn;
}

int CPVRChannelGroups::GetFirstChannelForGroupID(int iGroupId) const
{
  int iReturn = 1;

  const CPVRChannelGroup *group = GetById(iGroupId);

  if (group)
    iReturn = group->GetFirstChannel()->ChannelID();

  return iReturn;
}

int CPVRChannelGroups::GetPreviousGroupID(int iGroupId) const
{
  const CPVRChannelGroup *currentGroup = GetById(iGroupId);
  if (!currentGroup)
    currentGroup = GetGroupAll();

  return GetPreviousGroup(*currentGroup)->GroupID();
}

const CPVRChannelGroup *CPVRChannelGroups::GetPreviousGroup(const CPVRChannelGroup &group) const
{
  const CPVRChannelGroup *returnGroup = NULL;

  int iCurrentGroupIndex = GetIndexForGroupID(group.GroupID());
  if (iCurrentGroupIndex - 1 < 0)
    returnGroup = at(size() - 1);
  else
    returnGroup = at(iCurrentGroupIndex - 1);

  if (!returnGroup)
    returnGroup = GetGroupAll();

  return returnGroup;
}

int CPVRChannelGroups::GetNextGroupID(int iGroupId) const
{
  const CPVRChannelGroup *currentGroup = GetById(iGroupId);
  if (!currentGroup)
    currentGroup = GetGroupAll();

  return GetNextGroup(*currentGroup)->GroupID();
}

CPVRChannelGroup *CPVRChannelGroups::GetNextGroup(const CPVRChannelGroup &group) const
{
  CPVRChannelGroup *returnGroup = NULL;

  int iCurrentGroupIndex = GetIndexForGroupID(group.GroupID());
  if (iCurrentGroupIndex + 1 >= (int)size())
    returnGroup = at(0);
  else
    returnGroup = at(iCurrentGroupIndex + 1);

  if (!returnGroup)
    returnGroup = GetGroupAll();

  return returnGroup;
}

bool CPVRChannelGroups::AddGroup(const CStdString &strName)
{
  bool bReturn = false;

  CPVRChannelGroup *group = (CPVRChannelGroup *) GetByName(strName);
  if (!group)
  {
    group = new CPVRChannelGroup(m_bRadio);
    group->SetGroupName(strName);
    push_back(group);

    bReturn = group->Persist();
  }
  else
  {
    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannelGroups::DeleteGroup(const CPVRChannelGroup &group)
{
  bool bReturn = false;

  if (group.IsInternalGroup())
  {
    CLog::Log(LOGERROR, "CPVRChannelGroups - %s - cannot delete internal group '%s'",
        __FUNCTION__, group.GroupName().c_str());
    return bReturn;
  }

  CPVRDatabase *database = OpenPVRDatabase();
  if (!database)
    return bReturn;

  /* remove all channels from the group */
  database->RemoveChannelsFromGroup(group);

  /* delete the group from the database */
  bReturn = database->Delete(group);

  database->Close();

  /* delete the group in this container */
  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
  {
    if (at(iGroupPtr)->GroupID() == group.GroupID())
    {
      delete at(iGroupPtr);
      erase(begin() + iGroupPtr);
      break;
    }
  }

  return bReturn;
}

const CStdString &CPVRChannelGroups::GetGroupName(int iGroupId) const
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
  {
    if (iGroupId == at(iGroupPtr)->GroupID())
      return at(iGroupPtr)->GroupName();
  }

  return g_localizeStrings.Get(13205);
}

int CPVRChannelGroups::GetGroupId(CStdString strGroupName) const
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
  {
    if (strGroupName == at(iGroupPtr)->GroupName())
      return at(iGroupPtr)->GroupID();
  }
  return -1;
}

bool CPVRChannelGroups::AddChannelToGroup(CPVRChannel *channel, int iGroupId)
{
  bool bReturn = false;
  CPVRChannelGroup *group = (CPVRChannelGroup *) GetById(iGroupId);
  if (group)
  {
    bReturn = group->AddToGroup(channel);
  }

  return bReturn;
}
