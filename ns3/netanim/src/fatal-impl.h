/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 NICTA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Quincy Tse <quincy.tse@nicta.com.au>
 */

#ifndef FATAL_IMPL_H
#define FATAL_IMPL_H

#include <ostream>

/**
 * \ingroup debugging
 * \defgroup fatalHandler Fatal Error Handler
 *
 * \brief Functions to help clean up when fatal error
 * is encountered.
 *
 * The functions in this group are used to perform
 * limited clean up, like flushing active streams, when
 * fatal error are encountered (through assertion fail,
 * calls to NS_ABORT_* and calls to NS_FATAL_ERROR.
 *
 * Currently, other than flushing active ostreams, these
 * functions does not interfere with outside memory. There
 * is still a residual risk that may be invalid ostream
 * pointers may be present, and may corrupt the memory
 * on the attempt to execute the flush() function.
 */

namespace ns3
{
namespace FatalImpl
{

/**
 * \ingroup fatalHandler
 * \param stream The stream to be flushed on abnormal exit.
 *
 * \brief Register a stream to be flushed on abnormal exit.
 *
 * If a std::terminate() call is encountered after the
 * stream had been registered and before it had been
 * unregistered, stream->flush() will be called. Users of
 * this function is to ensure stream remains valid until
 * it had been unregistered.
 */
void RegisterStream (std::ostream* stream);

/**
 * \ingroup fatalHandler
 * \param stream The stream to be unregistered.
 *
 * \brief Unregister a stream for flushing on abnormal exit.
 *
 * After a stream had been unregistered, stream->flush()
 * will no longer be called should abnormal termination is
 * encountered.
 *
 * If stream is not registered, nothing will happen.
 */
void UnregisterStream (std::ostream* stream);

/**
 * \ingroup fatalHandler
 *
 * \brief Flush all currently registered streams.
 *
 * This function iterates through each registered stream and
 * unregister them. The default SIGSEGV handler is overridden
 * when this function is being executed, and will be restored
 * when this function returns.
 *
 * If a SIGSEGV is encountered (most likely due to bad ostream*
 * being registered, or a registered osteam* pointing to an
 * ostream that had already been destroyed), this function will
 * skip the bad ostream* and continue to flush the next stram.
 * The function will then terminate raising SIGIOT (aka SIGABRT)
 *
 * DO NOT call this function until the program is ready to crash.
 */
void FlushStreams (void);

} //FatalImpl
} //ns3

#endif
