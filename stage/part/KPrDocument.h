/* This file is part of the KDE project
   Copyright (C) 2006-2008 Thorsten Zachmann <zachmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KPRDOCUMENT_H
#define KPRDOCUMENT_H

#include <QObject>

#include <KoPADocument.h>
#include "KPrCustomSlideShows.h"

class KPrDeclarations;
class KPrShapeAnimation;
class KPrShapeAnimations;

#define STAGE_MIME_TYPE "application/vnd.oasis.opendocument.presentation"

class STAGE_EXPORT KPrDocument : public KoPADocument
{
    Q_OBJECT
public:
    explicit KPrDocument(KoPart *part=0);
    ~KPrDocument();

    /// reimplemented
    virtual KoPAPage *newPage(KoPAMasterPage *masterPage);
    /// reimplemented
    virtual KoPAMasterPage * newMasterPage();

    /// reimplemented
    virtual KoOdf::DocumentType documentType() const;

    /// reimplemented from KoDocument
    virtual QByteArray nativeFormatMimeType() const { return STAGE_MIME_TYPE; }
    /// reimplemented from KoDocument
    virtual QByteArray nativeOasisMimeType() const {return STAGE_MIME_TYPE;}
    /// reimplemented from KoDocument
    virtual QStringList extraNativeMimeTypes() const
    {
        return QStringList() << "application/vnd.oasis.opendocument.presentation-template";
    }

    /**
     * @brief Add animation to shape
     *
     * @param animation animation to add to shape
     */
    void addAnimation( KPrShapeAnimation * animation );

    /**
     * @brief Remove animation from shape
     *
     * @param animation animation to remove from shape
     * @param removeFromApplicationData if true the animation will also be removed from the
     *        application data
     */
    void removeAnimation( KPrShapeAnimation * animation, bool removeFromApplicationData = true );

    void replaceAnimation(KPrShapeAnimation *oldAnimation, KPrShapeAnimation *newAnimation);

    /**
     * @brief get the slideShows defined for this document
     */
    KPrCustomSlideShows* customSlideShows();
    void setCustomSlideShows( KPrCustomSlideShows* replacement );

    /**
     * Get the presentation monitor (screen) used for presentation
     *
     * @return the screen used for presentation, starting from screen 0
     */
    int presentationMonitor();

    /**
     * Set the presentation monitor (screen) used for presentation
     *
     * @param monitor the new screen number used for presentation
     */
    Q_INVOKABLE void setPresentationMonitor( int monitor );

    /**
     * Check whether the presenter view feature is enabled for presentation
     *
     * @return true if presenter view is enabled, false otherwise
     */
    bool isPresenterViewEnabled();

    /**
     * Enable / disable the presenter view features
     *
     * @param enabled whether the presenter view should be enabled or disabled
     */
    Q_INVOKABLE void setPresenterViewEnabled( bool enabled );

    /**
     * Get the list of pages for slide show. It is possible that the pages for
     * slideshow are different from KoPADocument::pages() due to custom slide show
     *
     * @return the list of pages for slide show
     */
    QList<KoPAPageBase*> slideShow() const;

    /**
     * Get the name of currently active custom slide show, or an empty string
     * if "all slides" is used for the slide show and no active custom slide show
     *
     * @return the name of currently active custom slide show, or empty string if none
     */
    QString activeCustomSlideShow() const;

    /**
     * Set the currently active custom slide show. The custom slide show name should
     * be valid, i.e. KPrCustomSlideShow::names() contains the name
     *
     * @param customSlideShow the new active custom slide show
     */
    void setActiveCustomSlideShow( const QString &customSlideShow );

    /// reimplemented
    virtual void saveOdfDocumentStyles( KoPASavingContext & context );

    /// reimplemented
    virtual bool loadOdfDocumentStyles( KoPALoadingContext & context );

    /// reimplemented
    virtual bool loadOdfProlog( const KoXmlElement & body, KoPALoadingContext & context );

    /**
     * Get the page type used in the document
     *
     * The default page type KoPageApp::Page is returned
     */
    virtual KoPageApp::PageType pageType() const;

    /**
     * Get the KPrDeclarations pointer
     */
    KPrDeclarations * declarations() const;

public Q_SLOTS:
    virtual void initEmpty();

Q_SIGNALS:
    /**
     * Emitted when the active custom slide show changes.
     * This is to allow for signalling dbus interfaces.
     *
     * @param customSlideShow the new active custom slide show
     */
    void activeCustomSlideShowChanged( const QString &customSlideShow );

    /**
     * Emitted when the custom slide shows have been modified.
     * This is to allow for signalling dbus interfaces.
     */
    void customSlideShowsModified();

protected:
    /// reimplemented
    virtual const char *odfTagName( bool withNamespace );

    /// reimplemented
    virtual bool loadOdfEpilogue( const KoXmlElement & body, KoPALoadingContext & context );

    /// reimplemented
    virtual bool saveOdfProlog( KoPASavingContext & paContext );

    /// reimplemented
    virtual bool saveOdfEpilogue( KoPASavingContext & context );

    /// reimplemented
    virtual void postAddShape( KoPAPageBase * page, KoShape * shape );
    /// reimplemented
    virtual void postRemoveShape( KoPAPageBase * page, KoShape * shape );

    /// reimplemented
    virtual void removePages(QList<KoPAPageBase*> &pages);

    /// load configuration specific to KPresenter
    void loadKPrConfig();

    /// save configuration specific to KPresenter
    void saveKPrConfig();

    /**
     * @brief get the animations of the page
     */
    KPrShapeAnimations & animationsByPage( KoPAPageBase * page );

    KPrCustomSlideShows *m_customSlideShows;

private:
    int m_presentationMonitor;
    bool m_presenterViewEnabled;
    QString m_activeCustomSlideShow;
    /// Message shown before KPresenter quits with an error if something is wrong
    KPrDeclarations *m_declarations;
};

#endif /* KPRDOCUMENT_H */
