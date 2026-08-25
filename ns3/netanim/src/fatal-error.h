/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA, 2010 NICTA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *         Quincy Tse <quincy.tse@nicta.com.au>
 */
#ifndef NS3_FATAL_ERROR_H
#define NS3_FATAL_ERROR_H

#include <iostream>
#include <exception>
#include <cstdlib>

#include "fatal-impl.h"

/**
 * \ingroup debugging
 * \brief fatal error handling
 *
 * When this macro is hit at runtime, details of filename
 * and line number is printed to stderr, and the program
 * is halted by calling std::terminate(). This will
 * trigger any clean up code registered by
 * std::set_terminate (NS3 default is a stream-flushing
 * code), but may be overridden.
 *
 * This macro is enabled unconditionally in all builds,
 * including debug and optimized builds.
 */
#define NS_FATAL_ERROR_NO_MSG()                           \
  do                                                      \
    {                                                     \
      std::cerr << "file=" << __FILE__ << ", line=" <<    \
      __LINE__ << std::endl;                            \
      ::ns3::FatalImpl::FlushStreams ();                  \
      std::terminate ();                                  \
    }                                                     \
  while (false)

/**
 * \ingroup debugging
 * \brief fatal error handling
 *
 * \param msg message to output when this macro is hit.
 *
 * When this macro is hit at runtime, the user-specified
 * error message is printed to stderr, followed by a call
 * to the NS_FATAL_ERROR_NO_MSG() macro which prints the
 * details of filename and line number to stderr. The
 * program will be halted by calling std::terminate(),
 * triggering any clean up code registered by
 * std::set_terminate (NS3 default is a stream-flushing
 * code, but may be overridden).
 *
 * This macro is enabled unconditionally in all builds,
 * including debug and optimized builds.
 */
#define NS_FATAL_ERROR(msg)                             \
  do                                                    \
    {                                                   \
      std::cerr << "msg=\"" << msg << "\", ";           \
      NS_FATAL_ERROR_NO_MSG ();                          \
    }                                                   \
  while (false)

#endif /* FATAL_ERROR_H */
