/*
 *  Copyright (c) 2012 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __KOVCMULTIARCHBUILDSUPPORT_H
#define __KOVCMULTIARCHBUILDSUPPORT_H

#include "config-vc.h"

#ifdef HAVE_SANE_VC

#include <Vc/Vc>
#include <Vc/common/support.h>

#else /* HAVE_SANE_VC */

namespace Vc {
    typedef enum {ScalarImpl} Implementation;
}

#define VC_IMPL ::Vc::ScalarImpl

#ifdef DO_PACKAGERS_BUILD
#warning "Packagers build is not available without the presence of Vc library. Disabling."
#undef DO_PACKAGERS_BUILD
#endif

#endif /* HAVE_SANE_VC */


#ifdef DO_PACKAGERS_BUILD

template<template<Vc::Implementation _impl> class FactoryType, class ReturnType>
    ReturnType* createOptimizedFactory()
{
    /*if (Vc::isImplementationSupported(Vc::Fma4Impl)) {
        return new FactoryType<Vc::Fma4Impl>();
    } else if (Vc::isImplementationSupported(Vc::XopImpl)) {
        return new FactoryType<Vc::XopImpl>();
        } else*/
    if (Vc::isImplementationSupported(Vc::AVXImpl)) {
        return new FactoryType<Vc::AVXImpl>();
    } else if (Vc::isImplementationSupported(Vc::SSE42Impl)) {
        return new FactoryType<Vc::SSE42Impl>();
    } else if (Vc::isImplementationSupported(Vc::SSE41Impl)) {
        return new FactoryType<Vc::SSE41Impl>();
    } else if (Vc::isImplementationSupported(Vc::SSE4aImpl)) {
        return new FactoryType<Vc::SSE4aImpl>();
    } else if (Vc::isImplementationSupported(Vc::SSSE3Impl)) {
        return new FactoryType<Vc::SSSE3Impl>();
    } else if (Vc::isImplementationSupported(Vc::SSE3Impl)) {
        return new FactoryType<Vc::SSE3Impl>();
    } else if (Vc::isImplementationSupported(Vc::SSE2Impl)) {
        return new FactoryType<Vc::SSE2Impl>();
    } else {
        return new FactoryType<Vc::ScalarImpl>();
    }
}

#define DECLARE_FOR_ALL_ARCHS(_DECL)             \
    _DECL(Vc::ScalarImpl);                       \
    _DECL(Vc::SSE2Impl);                         \
    _DECL(Vc::SSE3Impl);                         \
    _DECL(Vc::SSSE3Impl);                        \
    _DECL(Vc::SSE41Impl);                        \
    _DECL(Vc::SSE42Impl);                        \
    _DECL(Vc::SSE4aImpl);                        \
    _DECL(Vc::AVXImpl);/*                        \
    _DECL(Vc::XopImpl);                          \
    _DECL(Vc::Fma4Impl);*/

#else /* DO_PACKAGERS_BUILD */

/**
 * When doing not a packager's build we have one architecture only,
 * so the factory methods are simplified
 */

template<template<Vc::Implementation _impl> class FactoryType, class ReturnType>
    ReturnType* createOptimizedFactory()
{
    return new FactoryType<VC_IMPL>();
}

#define DECLARE_FOR_ALL_ARCHS(_DECL)             \
    _DECL(Vc::ScalarImpl);                       \
    _DECL(VC_IMPL);

#endif /* DO_PACKAGERS_BUILD */

#endif /* __KOVCMULTIARCHBUILDSUPPORT_H */