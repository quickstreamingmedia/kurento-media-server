/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

#ifndef __MIXER_PORT_HPP__
#define __MIXER_PORT_HPP__

#include "EndPoint.hpp"

namespace kurento
{

class MixerPort : public EndPoint
{
public:
  MixerPort (MediaSet &mediaSet, std::shared_ptr<Mixer> parent,
             const std::map<std::string, KmsMediaParam> &params = emptyParams);
  virtual ~MixerPort() throw ();

  int getHandlerId ();
private:
  std::shared_ptr<Mixer> mixer;
  int handlerId;

  class StaticConstructor
  {
  public:
    StaticConstructor();
  };

  static StaticConstructor staticConstructor;
};

} // kurento

#endif /* __MIXER_PORT_HPP__ */
