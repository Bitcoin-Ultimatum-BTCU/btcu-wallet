// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMPAT_H
#define BITCOIN_COMPAT_H

#if defined(HAVE_CONFIG_H)
#include "config/btcu-config.h"
#endif

#ifdef WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef FD_SETSIZE
#undef FD_SETSIZE // prevent redefinition compiler warning
#endif
#define FD_SETSIZE 1024 // max number of fds in fd_set

#include <winsock2.h> // Must be included before mswsock.h and windows.h

#include <mswsock.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <climits>
#include <ifaddrs.h>
#include <limits.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef WIN32
#define MSG_DONTWAIT 0
#else
typedef u_int SOCKET;
typedef unsigned int SOCKET;
#include <errno.h>
#define WSAGetLastError() errno
#define WSAEINVAL EINVAL
#define WSAEALREADY EALREADY
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAEMSGSIZE EMSGSIZE
#define WSAEINTR EINTR
#define WSAEINPROGRESS EINPROGRESS
#define WSAEADDRINUSE EADDRINUSE
#define WSAENOTSOCK EBADF
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR -1
#endif

#ifdef WIN32
#ifndef S_IRUSR
#define S_IRUSR 0400
#define S_IWUSR 0200
#endif
#else
#define MAX_PATH 1024
#endif
#ifdef _MSC_VER
#ifdef _WIN64
#define ssize_t int64_t
#else
#define ssize_t int32_t
#endif
#endif
// As Solaris does not have the MSG_NOSIGNAL flag for send(2) syscall, it is defined as 0
#if !defined(HAVE_MSG_NOSIGNAL) && !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

#ifndef WIN32
// PRIO_MAX is not defined on Solaris
#ifndef PRIO_MAX
#define PRIO_MAX 20
#endif
#define THREAD_PRIORITY_LOWEST PRIO_MAX
#define THREAD_PRIORITY_BELOW_NORMAL 2
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_ABOVE_NORMAL (-2)
#endif

#if HAVE_DECL_STRNLEN == 0
size_t strnlen(const char *start, size_t max_len);
#endif // HAVE_DECL_STRNLEN

#ifndef WIN32
typedef void *sockopt_arg_type;
#else
typedef char *sockopt_arg_type;
#endif

// Note these both should work with the current usage of poll, but best to be
// safe
// WIN32 poll is broken
// https://daniel.haxx.se/blog/2012/10/10/wsapoll-is-broken/
// __APPLE__ poll is broke
// https://github.com/bitcoin/bitcoin/pull/14336#issuecomment-437384408
#if defined(__linux__)
#define USE_POLL
#endif

static bool inline IsSelectableSocket(const SOCKET &s) {
#if defined(USE_POLL) || defined(WIN32)
    return true;
#else
    return (s < FD_SETSIZE);
#endif
}

#endif // BITCOIN_COMPAT_H
