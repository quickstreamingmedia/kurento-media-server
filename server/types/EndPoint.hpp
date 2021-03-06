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

#ifndef __END_POINT_HPP__
#define __END_POINT_HPP__

#include "MediaElement.hpp"

namespace kurento
{

class EndPoint : public MediaElement
{
public:
  EndPoint (MediaSet &mediaSet, std::shared_ptr<MediaObjectImpl> parent,
            const std::string &endPointType,
            const std::map<std::string, KmsMediaParam> &params,
            const std::string &factoryName);
  virtual ~EndPoint() throw () = 0;

private:
  class StaticConstructor
  {
  public:
    StaticConstructor();
  };

  static StaticConstructor staticConstructor;
};

} // kurento

#endif /* __END_POINT_HPP__ */
