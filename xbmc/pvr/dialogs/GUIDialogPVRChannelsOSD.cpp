/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "GUIDialogPVRChannelsOSD.h"
#include "Application.h"
#include "FileItem.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "GUIDialogPVRGuideInfo.h"
#include "ViewState.h"
#include "settings/GUISettings.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/PVREpg.h"
#include "pvr/timers/PVRTimerInfoTag.h"

using namespace std;
using namespace PVR;

#define CONTROL_LIST                  11

CGUIDialogPVRChannelsOSD::CGUIDialogPVRChannelsOSD()
    : CGUIDialog(WINDOW_DIALOG_PVR_OSD_CHANNELS, "DialogPVRChannelsOSD.xml")
{
  m_vecItems = new CFileItemList;
}

CGUIDialogPVRChannelsOSD::~CGUIDialogPVRChannelsOSD()
{
  delete m_vecItems;
}

bool CGUIDialogPVRChannelsOSD::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      Clear();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      /* Close dialog immediately if now TV or radio channel is playing */
      if (!g_PVRManager.IsPlaying())
      {
        Close();
        return true;
      }
      CGUIWindow::OnMessage(message);
      Update();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (m_viewControl.HasControl(iControl))   // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          /* Switch to channel */
          GotoChannel(iItem);
          return true;
        }
        else if (iAction == ACTION_SHOW_INFO || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          /* Show information Dialog */
          ShowInfo(iItem);
          return true;
        }
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRChannelsOSD::Update()
{
  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  // empty the list ready for population
  Clear();

  CPVRChannel channel;
  g_PVRManager.GetCurrentChannel(&channel);
  g_PVRManager.GetPlayingGroup(channel.IsRadio())->GetMembers(m_vecItems);
  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(channel.ChannelNumber() - 1);
  g_graphicsContext.Unlock();
}

void CGUIDialogPVRChannelsOSD::Clear()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
}

void CGUIDialogPVRChannelsOSD::CloseOrSelect(void)
{
  if (g_guiSettings.GetBool("pvrmenu.closechannelosdonswitch"))
  {
    Close();
  }
  else
  {
    CPVRChannel channel;
    g_PVRManager.GetCurrentChannel(&channel);
    m_viewControl.SetSelectedItem(channel.ChannelNumber() - 1);
  }
}

void CGUIDialogPVRChannelsOSD::GotoChannel(int item)
{
  /* Check file item is in list range and get his pointer */
  if (item < 0 || item >= (int)m_vecItems->Size()) return;
  CFileItemPtr pItem = m_vecItems->Get(item);

  if (pItem->m_strPath == g_application.CurrentFile())
  {
    CloseOrSelect();
    return;
  }

  if (!g_application.PlayFile(*pItem))
  {
    CGUIDialogOK::ShowAndGetInput(19033,0,19136,0);
    return;
  }

  CloseOrSelect();
}

void CGUIDialogPVRChannelsOSD::ShowInfo(int item)
{
  /* Check file item is in list range and get his pointer */
  if (item < 0 || item >= (int)m_vecItems->Size()) return;

  CFileItemPtr pItem = m_vecItems->Get(item);
  if (pItem && pItem->IsPVRChannel())
  {
    /* Get the current running show on this channel from the EPG storage */
    CPVREpgInfoTag *epgnow = pItem->GetPVRChannelInfoTag()->GetEPGNow();
    if (!epgnow)
      return;
    CFileItem *itemNow  = new CFileItem(*epgnow);

    /* Load programme info dialog */
    CGUIDialogPVRGuideInfo* pDlgInfo = (CGUIDialogPVRGuideInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
    if (!pDlgInfo)
      return;

    /* inform dialog about the file item and open dialog window */
    pDlgInfo->SetProgInfo(itemNow);
    pDlgInfo->DoModal();
    delete itemNow; /* delete previuosly created FileItem */
  }

  return;
}

void CGUIDialogPVRChannelsOSD::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST));
}

void CGUIDialogPVRChannelsOSD::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

CGUIControl *CGUIDialogPVRChannelsOSD::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();

  return CGUIWindow::GetFirstFocusableControl(id);
}
