/*
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2006
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

#ifndef KIS_OPENGL_CANVAS_2_P_H
#define KIS_OPENGL_CANVAS_2_P_H

#include <opengl/kis_opengl.h>

#ifdef HAVE_OPENGL

/**
 * This is a workaround for a very slow updates in OpenGL canvas (~6ms).
 * The delay happens because of VSync in the swapBuffers() call. At first
 * we try to disable VSync. If it fails we just disable Double Buffer
 * completely.
 *
 * This file is effectively a bit of copy-paste from qgl_x11.cpp
 */


#if defined Q_OS_LINUX

#include <QByteArray>
#include <QVector>
#include <QLibrary>
#include <QX11Info>
#include <GL/glx.h>
#include <dlfcn.h>

#ifndef GL_NUM_EXTENSIONS
#define GL_NUM_EXTENSIONS 0x821D
#endif

extern const QString qt_gl_library_name();

namespace VSyncWorkaround {

    class QGLExtensionMatcher
    {
    public:
        QGLExtensionMatcher(const char *str);
        QGLExtensionMatcher();

        bool match(const char *str) const {
            int str_length = qstrlen(str);

            Q_ASSERT(str);
            Q_ASSERT(str_length > 0);
            Q_ASSERT(str[str_length-1] != ' ');

            for (int i = 0; i < m_offsets.size(); ++i) {
                const char *extension = m_extensions.constData() + m_offsets.at(i);
                if (qstrncmp(extension, str, str_length) == 0 && extension[str_length] == ' ')
                    return true;
            }
            return false;
        }

    private:
        void init(const char *str);

        QByteArray m_extensions;
        QVector<int> m_offsets;
    };


    typedef const GLubyte * (*qt_glGetStringi)(GLenum, GLuint);

    QGLExtensionMatcher::QGLExtensionMatcher(const char *str)
    {
        init(str);
    }

    QGLExtensionMatcher::QGLExtensionMatcher()
    {
        const char *extensionStr = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));

        if (extensionStr) {
            init(extensionStr);
        } else {
            // clear error state
            while (glGetError()) {}

            const QGLContext *ctx = QGLContext::currentContext();
            if (ctx) {
                qt_glGetStringi glGetStringi = (qt_glGetStringi)ctx->getProcAddress(QLatin1String("glGetStringi"));

                GLint numExtensions;
                glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

                for (int i = 0; i < numExtensions; ++i) {
                    const char *str = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, i));

                    m_offsets << m_extensions.size();

                    while (*str != 0)
                        m_extensions.append(*str++);
                    m_extensions.append(' ');
                }
            }
        }
    }

    void QGLExtensionMatcher::init(const char *str)
    {
        m_extensions = str;

        // make sure extension string ends with a space
        if (!m_extensions.endsWith(' '))
            m_extensions.append(' ');

        int index = 0;
        int next = 0;
        while ((next = m_extensions.indexOf(' ', index)) >= 0) {
            m_offsets << index;
            index = next + 1;
        }
    }

    void* qglx_getProcAddress(const char* procName)
    {
        // On systems where the GL driver is pluggable (like Mesa), we have to use
        // the glXGetProcAddressARB extension to resolve other function pointers as
        // the symbols wont be in the GL library, but rather in a plugin loaded by
        // the GL library.
        typedef void* (*qt_glXGetProcAddressARB)(const char *);
        static qt_glXGetProcAddressARB glXGetProcAddressARB = 0;
        static bool triedResolvingGlxGetProcAddress = false;
        if (!triedResolvingGlxGetProcAddress) {
            triedResolvingGlxGetProcAddress = true;
            QGLExtensionMatcher extensions(glXGetClientString(QX11Info::display(), GLX_EXTENSIONS));
            if (extensions.match("GLX_ARB_get_proc_address")) {
                void *handle = dlopen(NULL, RTLD_LAZY);
                if (handle) {
                    glXGetProcAddressARB = (qt_glXGetProcAddressARB) dlsym(handle, "glXGetProcAddressARB");
                    dlclose(handle);
                }
                if (!glXGetProcAddressARB)
                {
                    QLibrary lib(::qt_gl_library_name());
                    //lib.setLoadHints(QLibrary::ImprovedSearchHeuristics);
                    glXGetProcAddressARB = (qt_glXGetProcAddressARB) lib.resolve("glXGetProcAddressARB");
                }
            }
        }

        void *procAddress = 0;
        if (glXGetProcAddressARB) {
            procAddress = glXGetProcAddressARB(procName);
        }

        // If glXGetProcAddress didn't work, try looking the symbol up in the GL library
        if (!procAddress) {
            void *handle = dlopen(NULL, RTLD_LAZY);
            if (handle) {
                procAddress = dlsym(handle, procName);
                dlclose(handle);
            }
        }
        if (!procAddress) {

            QLibrary lib(::qt_gl_library_name());
            //lib.setLoadHints(QLibrary::ImprovedSearchHeuristics);
            procAddress = lib.resolve(procName);
        }

        return procAddress;
    }

    bool tryDisableVSync(QWidget *widget) {
        bool result = false;
        bool triedDisable = false;
        QX11Info info = widget->x11Info();
        Display *dpy = info.display();
        WId wid = widget->winId();

        QGLExtensionMatcher extensions(glXQueryExtensionsString(dpy, info.screen()));

        if (extensions.match("GLX_EXT_swap_control")) {
            typedef void (*kis_glXSwapIntervalEXT)(Display*, WId, int);
            kis_glXSwapIntervalEXT glXSwapIntervalEXT = (kis_glXSwapIntervalEXT)qglx_getProcAddress("glXSwapIntervalEXT");

            if (glXSwapIntervalEXT) {
                glXSwapIntervalEXT(dpy, wid, 0);
                triedDisable = true;

                unsigned int swap = 1;

#ifdef GLX_SWAP_INTERVAL_EXT
                glXQueryDrawable(dpy, wid, GLX_SWAP_INTERVAL_EXT, &swap);
#endif

                result = !swap;
            } else {
                qDebug() << "Couldn't load glXSwapIntervalEXT extension function";
            }
        } else if (extensions.match("GLX_MESA_swap_control")) {
            typedef int (*kis_glXSwapIntervalMESA)(unsigned int);
            typedef int (*kis_glXGetSwapIntervalMESA)(void);

            kis_glXSwapIntervalMESA glXSwapIntervalMESA = (kis_glXSwapIntervalMESA)qglx_getProcAddress("glXSwapIntervalMESA");
            kis_glXGetSwapIntervalMESA glXGetSwapIntervalMESA = (kis_glXGetSwapIntervalMESA)qglx_getProcAddress("glXGetSwapIntervalMESA");

            if (glXSwapIntervalMESA) {
                int retval = glXSwapIntervalMESA(0);
                triedDisable = true;

                int swap = 1;

                if (glXGetSwapIntervalMESA) {
                    swap = glXGetSwapIntervalMESA();
                } else {
                    qDebug() << "Couldn't load glXGetSwapIntervalMESA extension function";
                }

                result = !retval && !swap;
            } else {
                qDebug() << "Couldn't load glXSwapIntervalMESA extension function";
            }
        } else {
            qDebug() << "There is neither GLX_EXT_swap_control or GLX_MESA_swap_control extension supported";
        }

        if (triedDisable && !result) {
            qCritical();
            qCritical() << "CRITICAL: Your video driver forbids disabling VSync!";
            qCritical() << "CRITICAL: Try toggling some VSync- or VBlank-related options in your driver configuration dialog.";
            qCritical() << "CRITICAL: NVIDIA users can do:";
            qCritical() << "CRITICAL: sudo nvidia-settings  >  (tab) OpenGL settings > Sync to VBlank  ( unchecked )";
            qCritical();
        }

        return result;
    }
}

#elif defined Q_WS_WIN

#include <GL/wglew.h>

namespace VSyncWorkaround {
    bool tryDisableVSync(QWidget *) {
        bool retval = false;

#ifdef WGL_EXT_swap_control
        if (WGLEW_EXT_swap_control) {
            wglSwapIntervalEXT(0);
            int interval = wglGetSwapIntervalEXT();

            if (interval) {
                qWarning() << "Failed to disable VSync with WGLEW_EXT_swap_control";
            }

            retval = !interval;
        } else {
            qWarning() << "WGL_EXT_swap_control extension is not available";
        }
#else
        qWarning() << "GLEW WGL_EXT_swap_control extension is not compiled in";
#endif

        return retval;
    }
}

#else  // !defined Q_OS_LINUX && !defined Q_WS_WIN

namespace VSyncWorkaround {
    bool tryDisableVSync(QWidget *) {
        return false;
    }
}

#endif // defined Q_OS_LINUX

#endif // HAVE_OPENGL
#endif // KIS_OPENGL_CANVAS_2_P_H
