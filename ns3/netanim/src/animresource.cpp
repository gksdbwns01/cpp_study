/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */


#include "animresource.h"

AnimResourceManager * pResourceManager = 0;

AnimResourceManager::AnimResourceManager ():
    m_maxResourceId (0)
{

}

AnimResourceManager *
AnimResourceManager::getInstance ()
{
  if (!pResourceManager)
    {
      pResourceManager = new AnimResourceManager;
    }
  return pResourceManager;
}


void
AnimResourceManager::add (uint32_t resourceId, QString resourcePath)
{
  m_maxResourceId = qMax (resourceId, m_maxResourceId);
  m_resources[resourceId] = resourcePath;
}

uint32_t
AnimResourceManager::getNewResourceId ()
{
  return m_maxResourceId+1;
}


QString
AnimResourceManager::get (uint32_t resourceid)
{
  if (m_resources.find (resourceid) == m_resources.end ())
    {
      NS_FATAL_ERROR ("Unable to find resource:" << resourceid);
    }
  return m_resources[resourceid];
}
