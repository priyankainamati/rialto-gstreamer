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

#include "MediaPlayerManager.h"
#include "ClientBackend.h"

std::mutex MediaPlayerManager::m_mediaPlayerClientsMutex;
std::map<const GstObject *, MediaPlayerManager::MediaPlayerClientInfo> MediaPlayerManager::m_mediaPlayerClientsInfo;

MediaPlayerManager::MediaPlayerManager() : m_currentGstBinParent(nullptr)
{
}

MediaPlayerManager::~MediaPlayerManager()
{
    releaseMediaPlayerClient();
}

bool MediaPlayerManager::attachMediaPlayerClient(const GstObject *gstBinParent, const uint32_t maxVideoWidth = 0, const uint32_t maxVideoHeight = 0)
{
    if (!m_client.lock())
    {
        createMediaPlayerClient(gstBinParent);
    }
    else if (gstBinParent != m_currentGstBinParent)
    {
        // New parent gst bin, release old client and create new
        releaseMediaPlayerClient();
        createMediaPlayerClient(gstBinParent);
    }

    if(!m_client.lock())
    {
        return false;
    }
    else
    {
        return true;
    }
}

std::shared_ptr<GStreamerMSEMediaPlayerClient> MediaPlayerManager::getMediaPlayerClient()
{
    return m_client.lock();
}

bool MediaPlayerManager::hasControl()
{
    if (m_client.lock())
    {
        std::lock_guard<std::mutex> guard(m_mediaPlayerClientsMutex);

        auto it = m_mediaPlayerClientsInfo.find(m_currentGstBinParent);
        if (it != m_mediaPlayerClientsInfo.end())
        {
            if (it->second.controller == this)
            {
                return true;
            }
            else
            { // in case there's no controller anymore
                return acquireControl(it->second);
            }
        }
    }

    return false;
}

void MediaPlayerManager::releaseMediaPlayerClient()
{
    if (m_client.lock())
    {
        std::lock_guard<std::mutex> guard(m_mediaPlayerClientsMutex);

        auto it = m_mediaPlayerClientsInfo.find(m_currentGstBinParent);
        if (it != m_mediaPlayerClientsInfo.end())
        {
            it->second.refCount--;
            if (it->second.refCount == 0)
            {
                it->second.client->stopStreaming();
                it->second.client->destroyClientBackend();
                m_mediaPlayerClientsInfo.erase(it);
            }
            else
            {
                if (it->second.controller == this)
                    it->second.controller = nullptr;
            }
            m_client.reset();
            m_currentGstBinParent = nullptr;
        }
    }
}

bool MediaPlayerManager::acquireControl(MediaPlayerClientInfo& mediaPlayerClientInfo)
{
    if (mediaPlayerClientInfo.controller == nullptr)
    {
        mediaPlayerClientInfo.controller = this;
        return true;
    }

    return false;
}

void MediaPlayerManager::createMediaPlayerClient(const GstObject *gstBinParent)
{
    std::lock_guard<std::mutex> guard(m_mediaPlayerClientsMutex);

    auto it = m_mediaPlayerClientsInfo.find(gstBinParent);
    if (it != m_mediaPlayerClientsInfo.end())
    {
        it->second.refCount++;
        m_client = it->second.client;
        m_currentGstBinParent = gstBinParent;
    }
    else
    {
        std::shared_ptr<firebolt::rialto::client::ClientBackendInterface> clientBackend =
            std::make_shared<firebolt::rialto::client::ClientBackend>();
        std::shared_ptr<GStreamerMSEMediaPlayerClient> client = std::make_shared<GStreamerMSEMediaPlayerClient>(clientBackend);

        if (client->createBackend())
        {
            // Store the new client in global map
            MediaPlayerClientInfo newClientInfo;
            newClientInfo.client = client;
            newClientInfo.controller = this;
            newClientInfo.refCount = 1;
            m_mediaPlayerClientsInfo.insert(std::pair<const GstObject *, MediaPlayerClientInfo>(gstBinParent, newClientInfo));

            // Store client info in object
            m_client = client;
            m_currentGstBinParent = gstBinParent;
        }
        else
        {
            // Failed to create backend
            return;
        }
    }
}
