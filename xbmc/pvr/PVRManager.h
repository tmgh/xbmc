#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "threads/Thread.h"
#include "utils/Observer.h"
#include "utils/JobManager.h"
#include "windows/GUIWindowPVRCommon.h"
#include "addons/include/xbmc_pvr_types.h"

class CGUIDialogBusy;

namespace PVR
{
  class CPVRClients;
  class CPVRChannelGroupsContainer;
  class CPVRChannelGroup;
  class CPVRRecordings;
  class CPVRTimers;
  class CPVREpgContainer;
  class CPVRGUIInfo;
  class CPVRDatabase;

  #define g_PVRManager       CPVRManager::Get()
  #define g_PVRChannelGroups g_PVRManager.ChannelGroups()
  #define g_PVREpg           g_PVRManager.EPG()
  #define g_PVRTimers        g_PVRManager.Timers()
  #define g_PVRRecordings    g_PVRManager.Recordings()
  #define g_PVRClients       g_PVRManager.Clients()

  class CPVRManager : public Observer, private CThread, public IJobCallback
  {
    friend class CPVRClients;

  private:
    /*!
     * @brief Create a new CPVRManager instance, which handles all PVR related operations in XBMC.
     */
    CPVRManager(void);

  public:
    /*!
     * @brief Process a message from an observable.
     * @param obs The observable that sends the message.
     * @param msg The message.
     */
    void Notify(const Observable &obs, const CStdString& msg);

    /*!
     * @brief Stop the PVRManager and destroy all objects it created.
     */
    virtual ~CPVRManager(void);

    /*!
     * @brief Get the instance of the PVRManager.
     * @return The PVRManager instance.
     */
    static CPVRManager &Get(void);

    /*!
     * @brief Get the channel groups container.
     * @return The groups container.
     */
    CPVRChannelGroupsContainer *ChannelGroups(void) const { return m_channelGroups; }

    /*!
     * @brief Get the EPG container.
     * @return The EPG container.
     */
    CPVREpgContainer *EPG(void) const { return m_epg; }

    /*!
     * @brief Get the recordings container.
     * @return The recordings container.
     */
    CPVRRecordings *Recordings(void) const { return m_recordings; }

    /*!
     * @brief Get the timers container.
     * @return The timers container.
     */
    CPVRTimers *Timers(void) const { return m_timers; }

    /*!
     * @brief Get the timers container.
     * @return The timers container.
     */
    CPVRClients *Clients(void) const { return m_addons; }

    /*!
     * @brief Start the PVRManager, which loads all PVR data and starts some threads to update the PVR data.
     */
    void Start(void);

    /*!
     * @brief Stop the PVRManager and destroy all objects it created.
     */
    void Stop(void);

  public:

    /*!
     * @brief Get the TV database.
     * @return The TV database.
     */
    CPVRDatabase *GetTVDatabase(void) const { return m_database; }

    /*!
     * @brief Updates the recordings and the "now" and "next" timers.
     */
    void UpdateRecordingsCache(void);

    /*!
     * @brief Get a GUIInfoManager character string.
     * @param dwInfo The string to get.
     * @return The requested string or an empty one if it wasn't found.
     */
    bool TranslateCharInfo(DWORD dwInfo, CStdString &strValue) const;

    /*!
     * @brief Get a GUIInfoManager integer.
     * @param dwInfo The integer to get.
     * @return The requested integer or 0 if it wasn't found.
     */
    int TranslateIntInfo(DWORD dwInfo) const;

    /*!
     * @brief Get a GUIInfoManager boolean.
     * @param dwInfo The boolean to get.
     * @return The requested boolean or false if it wasn't found.
     */
    bool TranslateBoolInfo(DWORD dwInfo) const;

    /*!
     * @brief Reset the TV database to it's initial state and delete all the data inside.
     * @param bShowProgress True to show a progress bar, false otherwise.
     */
    void ResetDatabase(bool bShowProgress = true);

    /*!
     * @brief Delete all EPG data from the database and reload it from the clients.
     */
    void ResetEPG(void);

    /*!
     * @brief Check if a TV channel, radio channel or recording is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlaying(void) const;

    /*!
     * @return True if the thread is stopped, false otherwise.
     */
    bool IsRunning(void) const;

    /*!
     * @brief Return the channel that is currently playing.
     * @param channel The channel or NULL if none is playing.
     * @return True if a channel is playing, false otherwise.
     */
    bool GetCurrentChannel(CPVRChannel *channel) const;

    /*!
     * @brief Return the EPG for the channel that is currently playing.
     * @param channel The EPG or NULL if no channel is playing.
     * @return The amount of results that was added or -1 if none.
     */
    int GetCurrentEpg(CFileItemList *results) const;

    /*!
     * @brief Check whether the PVRManager has fully started.
     * @return True if started, false otherwise.
     */
    bool IsStarted(void) const { return m_bLoaded; }

    /*!
     * @brief Switch to the given channel.
     * @param channel The channel to switch to.
     * @param bPreview True to show a preview, false otherwise.
     * @return Trrue if the switch was successful, false otherwise.
     */
    bool PerformChannelSwitch(const CPVRChannel &channel, bool bPreview);

    /*!
     * @brief Close an open PVR stream.
     */
    void CloseStream(void);

    /*!
     * @brief Open a stream from the given channel.
     * @param tag The channel to open.
     * @return True if the stream was opened, false otherwise.
     */
    bool OpenLiveStream(const CPVRChannel &tag);

    /*!
     * @brief Open a stream from the given recording.
     * @param tag The recording to open.
     * @return True if the stream was opened, false otherwise.
     */
    bool OpenRecordedStream(const CPVRRecording &tag);

    /*!
     * @brief Start or stop recording on the channel that is currently being played.
     * @param bOnOff True to start recording, false to stop.
     * @return True if the recording was started or stopped successfully, false otherwise.
     */
    bool StartRecordingOnPlayingChannel(bool bOnOff);

    /*!
     * @brief Get the channel number of the previously selected channel.
     * @return The requested channel number or -1 if it wasn't found.
     */
    int GetPreviousChannel(void);

    /*!
     * @brief Check whether there are active timers.
     * @return True if there are active timers, false otherwise.
     */
    bool HasTimer(void) const;

    /*!
     * @brief Check whether there are active recordings.
     * @return True if there are active recordings, false otherwise.
     */
    bool IsRecording(void) const;

    /*!
     * @brief Set the current playing group, used to load the right channel.
     * @param group The new group.
     */
    void SetPlayingGroup(CPVRChannelGroup *group);

    /*!
     * @brief Get the current playing group, used to load the right channel.
     * @param bRadio True to get the current radio group, false to get the current TV group.
     * @return The current group or the group containing all channels if it's not set.
     */
    CPVRChannelGroup *GetPlayingGroup(bool bRadio = false);

    /*!
     * @brief Let the background thread update the recordings list.
     */
    void TriggerRecordingsUpdate(void);

    /*!
     * @brief Let the background thread update the timer list.
     */
    void TriggerTimersUpdate(void);

    /*!
     * @brief Let the background thread update the channel list.
     */
    void TriggerChannelsUpdate(void);

    /*!
     * @brief Let the background thread update the channel groups list.
     */
    void TriggerChannelGroupsUpdate(void);

    /*!
     * @brief Update the channel that is currently active.
     * @param item The new channel.
     * @return True if it was updated correctly, false otherwise.
     */
    bool UpdateItem(CFileItem& item);

    /*!
     * @brief Switch to a channel given it's channel number.
     * @param iChannelNumber The channel number to switch to.
     * @return True if the channel was switched, false otherwise.
     */
    bool ChannelSwitch(unsigned int iChannelNumber);

    /*!
     * @brief Switch to the next channel in this group.
     * @param iNewChannelNumber The new channel number after the switch.
     * @param bPreview If true, don't do the actual switch but just update channel pointers.
     *                Used to display event info while doing "fast channel switching"
     * @return True if the channel was switched, false otherwise.
     */
    bool ChannelUp(unsigned int *iNewChannelNumber, bool bPreview = false) { return ChannelUpDown(iNewChannelNumber, bPreview, true); }

    /*!
     * @brief Switch to the previous channel in this group.
     * @param iNewChannelNumber The new channel number after the switch.
     * @param bPreview If true, don't do the actual switch but just update channel pointers.
     *                Used to display event info while doing "fast channel switching"
     * @return True if the channel was switched, false otherwise.
     */
    bool ChannelDown(unsigned int *iNewChannelNumber, bool bPreview = false) { return ChannelUpDown(iNewChannelNumber, bPreview, false); }

    /*!
     * @brief Get the total duration of the currently playing LiveTV item.
     * @return The total duration in milliseconds or NULL if no channel is playing.
     */
    int GetTotalTime(void) const;

    /*!
     * @brief Get the current position in milliseconds since the start of a LiveTV item.
     * @return The position in milliseconds or NULL if no channel is playing.
     */
    int GetStartTime(void) const;

    /*!
     * @brief Start playback on a channel.
     * @param channel The channel to start to play.
     * @param bPreview If true, open minimised.
     * @return True if playback was started, false otherwise.
     */
    bool StartPlayback(const CPVRChannel *channel, bool bPreview = false);

    /*!
     * @brief Update the current playing file in the guiinfomanager and application.
     */
    void UpdateCurrentFile(void);

    /*!
     * @brief Update the data in a pvr window if that window is currently visible.
     * @param window The window to update.
     * @param bResetContents True to reset the contents of the given window, false to just mark all the window's items invalid.
     */
    void UpdateWindow(PVRWindow window, bool bResetContents = true);

    /*!
     * @brief Check whether names are still correct after the language settings changed.
     */
    void LocalizationChanged(void);

    /*!
     * @brief Check if a TV channel is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingTV(void) const;

    /*!
     * @brief Check if a radio channel is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingRadio(void) const;

    /*!
     * @brief Check if a recording is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingRecording(void) const;

    /*!
     * @return True when a channel scan is currently running, false otherwise.
     */
    bool IsRunningChannelScan(void) const;

    /*!
     * @brief Get the properties of the current playing client.
     * @return A pointer to the properties or NULL if no stream is playing.
     */
    PVR_ADDON_CAPABILITIES *GetCurrentClientProperties(void);

    /*!
     * @brief Open a selection dialog and start a channel scan on the selected client.
     */
    void StartChannelScan(void);

    /*!
     * @brief Try to find missing channel icons automatically
     */
    void SearchMissingChannelIcons(void);

    /*!
     * @brief Check whether a group is selected.
     * @param group The group to check.
     * @return True if it's selected, false otherwise.
     */
    bool IsSelectedGroup(const CPVRChannelGroup &group) const;

    bool IsUpdating(void) const;

  protected:
    /*!
     * @brief PVR update and control thread.
     */
    virtual void Process(void);

    /*!
     * @brief Disable the pvrmanager if no clients are enabled or active.
     * @return True if no clients are enabled, false otherwise.
     */
    bool DisableIfNoClients(void);

  private:

    /*!
     * @brief Load at least one client and load all other PVR data after loading the client.
     * If some clients failed to load here, the pvrmanager will retry to load them every second.
     * @return If at least one client and all pvr data was loaded, false otherwise.
     */
    bool Load(void);

    /*!
     * @brief Update all recordings.
     */
    void UpdateRecordings(void);

    /*!
     * @brief Update all timers.
     */
    void UpdateTimers(void);

    /*!
     * @brief Update all channels.
     */
    void UpdateChannels(void);

    /*!
     * @brief Update all channel groups and channels in them.
     */
    void UpdateChannelGroups(void);

    /*!
     * @see CPVRGUIInfo
     */
    //!{
    const char *CharInfoNowRecordingTitle(void);
    const char *CharInfoNowRecordingChannel(void);
    const char *CharInfoNowRecordingDateTime(void);
    const char *CharInfoNextRecordingTitle(void);
    const char *CharInfoNextRecordingChannel(void);
    const char *CharInfoNextRecordingDateTime(void);
    const char *CharInfoNextTimer(void);
    const char *CharInfoPlayingDuration(void);
    const char *CharInfoPlayingTime(void);
    //!}

    /*!
     * @brief Reset all properties.
     */
    void ResetProperties(void);

    /*!
     * @brief Switch to the given channel.
     * @param channel The new channel.
     * @param bPreview Don't reset quality data if true.
     * @return True if the switch was successful, false otherwise.
     */
    bool PerformChannelSwitch(const CPVRChannel *channel, bool bPreview);

    /*!
     * @brief Called by ChannelUp() and ChannelDown() to perform a channel switch.
     * @param iNewChannelNumber The new channel number after the switch.
     * @param bPreview Preview window if true.
     * @param bUp Go one channel up if true, one channel down if false.
     * @return True if the switch was successful, false otherwise.
     */
    bool ChannelUpDown(unsigned int *iNewChannelNumber, bool bPreview, bool bUp);

    /*!
     * @brief Stop the EPG and PVR threads but do not remove their data.
     */
    void StopUpdateThreads(void);

    /*!
     * @brief Restart the EPG and PVR threads after they've been stopped by StopUpdateThreads()
     */
    bool StartUpdateThreads(void);

    /*!
     * @brief Persist the current channel settings in the database.
     */
    void SaveCurrentChannelSettings(void);

    /*!
     * @brief Load the settings for the current channel from the database.
     */
    void LoadCurrentChannelSettings(void);

    /*!
     * @brief Continue playback on the last channel if it was stored in the database.
     * @return True if playback was continued, false otherwise.
     */
    bool ContinueLastChannel(void);

    /*!
     * @brief Show or hide the busy dialog.
     * @param bShow True to show the dialog, false to hide it.
     */
    void ShowBusyDialog(bool bShow);

    void StartRecordingsUpdateJob(void);
    void StartTimersUpdateJob(void);
    void StartChannelsUpdateJob(void);
    void StartChannelGroupsUpdateJob(void);
    void StartNextPendingJob(void);

    void OnJobComplete(unsigned int jobID, bool success, CJob* job);

    /** @name containers */
    //@{
    CPVRChannelGroupsContainer *    m_channelGroups;               /*!< pointer to the channel groups container */
    CPVREpgContainer *              m_epg;                         /*!< pointer to the EPG container */
    CPVRRecordings *                m_recordings;                  /*!< pointer to the recordings container */
    CPVRTimers *                    m_timers;                      /*!< pointer to the timers container */
    CPVRClients *                   m_addons;                      /*!< pointer to the pvr addon container */
    CPVRGUIInfo *                   m_guiInfo;                     /*!< pointer to the guiinfo data */
    //@}

    CCriticalSection                m_critSectionTriggers;         /*!< critical section for triggered updates */
    bool                            m_bRecordingsUpdating;         /*!< true when recordings are being updated */
    bool                            m_bRecordingsUpdatePending;    /*!< true when another recordings update will be performed after the last one finished */
    bool                            m_bTimersUpdating;             /*!< true when timers are being updated */
    bool                            m_bTimersUpdatePending;        /*!< true when another timers update will be performed after the last one finished */
    bool                            m_bChannelsUpdating;           /*!< true when channels are being updated */
    bool                            m_bChannelsUpdatePending;      /*!< true when another channels update will be performed after the last one finished */
    bool                            m_bChannelGroupsUpdating;      /*!< true when channel groups are being updated */
    bool                            m_bChannelGroupsUpdatePending; /*!< true when another channel groups update will be performed after the last one finished */
    CFileItem *                     m_currentFile;                 /*!< the PVR file that is currently playing */
    CPVRDatabase *                  m_database;                    /*!< the database for all PVR related data */
    CCriticalSection                m_critSection;                 /*!< critical section for all changes to this class, except for changes to triggers */
    bool                            m_bFirstStart;                 /*!< true when the PVR manager was started first, false otherwise */
    bool                            m_bLoaded;                     /*!< true if the pvrmanager has been loaded and can be used */
    CGUIDialogBusy *                m_loadingBusyDialog;           /*!< busy dialog that is displayed while the pvrmanager is loading */
    CPVRChannelGroup *              m_currentRadioGroup;           /*!< the currently selected radio channel group list */
    CPVRChannelGroup *              m_currentTVGroup;              /*!< the currently selected TV channel group list */

    int                             m_PreviousChannel[2];
    int                             m_PreviousChannelIndex;
    int                             m_LastChannel;
    unsigned int                    m_LastChannelChanged;
  };

  class CPVRRecordingsUpdateJob : public CJob
  {
  public:
    CPVRRecordingsUpdateJob(void) {}
    virtual ~CPVRRecordingsUpdateJob() {}
    virtual const char *GetType() const { return "pvr-update-recordings"; }

    virtual bool DoWork();
  };

  class CPVRTimersUpdateJob : public CJob
  {
  public:
    CPVRTimersUpdateJob(void) {}
    virtual ~CPVRTimersUpdateJob() {}
    virtual const char *GetType() const { return "pvr-update-timers"; }

    virtual bool DoWork();
  };

  class CPVRChannelsUpdateJob : public CJob
  {
  public:
    CPVRChannelsUpdateJob(void) {}
    virtual ~CPVRChannelsUpdateJob() {}
    virtual const char *GetType() const { return "pvr-update-channels"; }

    virtual bool DoWork();
  };

  class CPVRChannelGroupsUpdateJob : public CJob
  {
  public:
    CPVRChannelGroupsUpdateJob(void) {}
    virtual ~CPVRChannelGroupsUpdateJob() {}
    virtual const char *GetType() const { return "pvr-update-channelgroups"; }

    virtual bool DoWork();
  };
}
