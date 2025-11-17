/*
 * Copyright (C) 2021 CutefishOS.
 *
 * Author:     revenmartin <revenmartin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "launcher.h"
#include "launcheradaptor.h"
#include "iconthemeimageprovider.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QPixmapCache>
#include <QQmlContext>
#include <QScreen>
#include <QTimer>
#include <QUrl>
#include <QShowEvent>
#include <QResizeEvent>

#include <QQuickView>

Launcher::Launcher(bool firstShow, QQuickView *w)
    : QQuickView(w)
    , m_dockInterface("com.cutefish.Dock",
                      "/Dock",
                      "com.cutefish.Dock", QDBusConnection::sessionBus())
    , m_hideTimer(new QTimer(this))
    , m_showed(false)
    , m_leftMargin(0)
    , m_rightMargin(0)
    , m_bottomMargin(0)
{
    // DBus adaptor
    new LauncherAdaptor(this);

    // Expose to QML
    engine()->rootContext()->setContextProperty("launcher", this);

    // Transparent background for the view
    setColor(Qt::transparent);

    // Frameless and don't appear in taskbar (Qt::Tool helps hide from taskbar across platforms)
    setFlags(Qt::FramelessWindowHint | Qt::Tool);

    // Keep QQuickView in SizeRootObjectToView mode
    setResizeMode(QQuickView::SizeRootObjectToView);

    // Update geometry / screen
    onGeometryChanged();

    // Load QML
    setSource(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    setTitle(tr("Launcher"));

    // Visible state
    setVisible(firstShow);

    // Let the animation in qml be hidden after the execution is complete
    m_hideTimer->setInterval(200);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, [this] { setVisible(false); });

    // If dock service already exists, connect signals; otherwise watch for unregistration
    if (m_dockInterface.isValid() && !m_dockInterface.lastError().isValid()) {
        updateMargins();
        connect(&m_dockInterface, SIGNAL(primaryGeometryChanged()), this, SLOT(updateMargins()));
        connect(&m_dockInterface, SIGNAL(directionChanged()), this, SLOT(updateMargins()));
    } else {
        QDBusServiceWatcher *watcher = new QDBusServiceWatcher("com.cutefish.Dock",
                                                               QDBusConnection::sessionBus(),
                                                               QDBusServiceWatcher::WatchForUnregistration,
                                                               this);
        connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this] {
            updateMargins();
            connect(&m_dockInterface, SIGNAL(primaryGeometryChanged()), this, SLOT(updateMargins()));
            connect(&m_dockInterface, SIGNAL(directionChanged()), this, SLOT(updateMargins()));
        });
    }

    // Screen changes
    connect(qApp, &QApplication::primaryScreenChanged, this, [this] { onGeometryChanged(); });
    connect(this, &QQuickView::activeChanged, this, &Launcher::onActiveChanged);
}

int Launcher::leftMargin() const
{
    return m_leftMargin;
}

int Launcher::rightMargin() const
{
    return m_rightMargin;
}

int Launcher::bottomMargin() const
{
    return m_bottomMargin;
}

bool Launcher::showed()
{
    return m_showed;
}

void Launcher::showWindow()
{
    m_showed = true;
    emit showedChanged();

    setVisible(true);
}

void Launcher::hideWindow()
{
    setVisible(false);
    m_showed = false;
    emit showedChanged();
}

void Launcher::toggle()
{
    isVisible() ? hideWindow() : showWindow();
}

bool Launcher::dockAvailable()
{
    return m_dockInterface.isValid();
}

bool Launcher::isPinedDock(const QString &desktop)
{
    QDBusInterface iface("com.cutefish.Dock",
                         "/Dock",
                         "com.cutefish.Dock",
                         QDBusConnection::sessionBus());

    if (!iface.isValid())
        return false;

    return iface.call("pinned", desktop).arguments().first().toBool();
}

void Launcher::clearPixmapCache()
{
    QPixmapCache::clear();
}

QRect Launcher::screenRect()
{
    return m_screenRect;
}

void Launcher::updateMargins()
{
    QRect dockGeometry = m_dockInterface.property("primaryGeometry").toRect();
    int dockDirection = m_dockInterface.property("direction").toInt();

    m_leftMargin = 0;
    m_rightMargin = 0;
    m_bottomMargin = 0;

    if (dockDirection == 0) {
        m_leftMargin = dockGeometry.width();
    } else if (dockDirection == 1) {
        m_bottomMargin = dockGeometry.height();
    } else if (dockDirection == 2) {
        m_rightMargin = dockGeometry.width();
    }

    emit marginsChanged();
}

void Launcher::updateSize()
{
    if (m_screenRect != qApp->primaryScreen()->geometry()) {
        m_screenRect = qApp->primaryScreen()->geometry();
        setGeometry(m_screenRect);
        emit screenRectChanged();
    }
}

void Launcher::onGeometryChanged()
{
    // Ensure previous screen signals disconnected
    disconnect(screen());

    setScreen(qApp->primaryScreen());
    updateSize();

    connect(screen(), &QScreen::virtualGeometryChanged, this, &Launcher::updateSize);
    connect(screen(), &QScreen::geometryChanged, this, &Launcher::updateSize);
}

void Launcher::showEvent(QShowEvent *e)
{
    // We intentionally avoid legacy KWindowSystem / NET calls here (KF6 removed X11 NET APIs).
    // Using Qt::Tool window flag above will keep the window out of the taskbar on most platforms.
    // On Wayland, the compositor controls taskbar/pager behavior and Qt cannot force it.

    QQuickView::showEvent(e);
}

void Launcher::resizeEvent(QResizeEvent *e)
{
    // The window manager forces the size.
    e->ignore();
}

void Launcher::onActiveChanged()
{
    if (!isActive())
        hide();
}
