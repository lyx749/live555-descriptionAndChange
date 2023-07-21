/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// Framed Filters
// C++ header

#ifndef _FRAMED_FILTER_HH
#define _FRAMED_FILTER_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

/**
 * 它提供了对输入源的管理（获取、分配、分离），并定义了一些虚函数供派生类重写以定制过滤器的行为。
 * 派生类可以通过继承 FramedFilter 类来实现特定的帧过滤器，并根据需要重写虚函数以自定义其行为。
*/
class FramedFilter: public FramedSource {
public:

  /// @brief 回 fInputSource 成员变量，即该过滤器的输入源（FramedSource 对象
  /// @return FramedSource*指针
  FramedSource* inputSource() const { return fInputSource; }

  /// @brief 修改fInputSource 成员变量为newInputSource
  /// @param newInputSource 需要修改的值
  void reassignInputSource(FramedSource* newInputSource) { fInputSource = newInputSource; }

  // Call before destruction if you want to prevent the destructor from closing the input source
  void detachInputSource();

protected:
  FramedFilter(UsageEnvironment& env, FramedSource* inputSource);
	 // abstract base class
  virtual ~FramedFilter();

protected:
  // Redefined virtual functions (with default 'null' implementations):
  virtual char const* MIMEtype() const;
  virtual void getAttributes() const;
  virtual void doStopGettingFrames();

protected:
  FramedSource* fInputSource;
};

#endif
