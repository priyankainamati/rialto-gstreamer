/*
 * Copyright (C) 2022 Sky UK
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MEDIAPLAYERMANAGER_H
#define MEDIAPLAYERMANAGER_H

#include "GStreamerMSEMediaPlayerClient.h"

class MediaPlayerManager
{
public:
    MediaPlayerManager();
    ~MediaPlayerManager();

    std::shared_ptr<GStreamerMSEMediaPlayerClient> getMediaPlayerClient(const GstObject *gstBinParent);
    void releaseMediaPlayerClient();
    bool hasControl();

private:
    void createMediaPlayerClient(const GstObject *gstBinParent);
    bool acquireControl(MediaPlayerClientInfo& mediaPlayerClientInfo);

    struct MediaPlayerClientInfo
    {
        std::shared_ptr<GStreamerMSEMediaPlayerClient> client;
        void *controller;
        uint32_t refCount;
    };

    std::weak_ptr<GStreamerMSEMediaPlayerClient> m_client;
    GstObject *m_currentGstBinParent;

    static std::mutex m_mediaPlayerClientsMutex;
    static std::map<const GstObject *gstBinParent, MediaPlayerClientInfo> m_mediaPlayerClientsInfo;
};

#endif // MEDIAPLAYERMANAGER_H