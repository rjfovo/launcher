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

#include <QApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QPixmapCache>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include <QDebug>
#include <QTranslator>
#include <QLocale>
#include <QFile>            // ★ Qt6必须显式包含 ★

#include "launcher.h"
#include "launchermodel.h"
#include "pagemodel.h"
#include "iconitem.h"
#include "appmanager.h"

#define DBUS_NAME "com.cutefish.Launcher"
#define DBUS_PATH "/Launcher"
#define DBUS_INTERFACE "com.cutefish.Launcher"

int main(int argc, char *argv[])
{
    // Qt6 默认开启 High DPI，不需要 AA_EnableHighDpiScaling

    // QML 注册
    QByteArray uri = "Cutefish.Launcher";
    qmlRegisterType<LauncherModel>(uri, 1, 0, "LauncherModel");
    qmlRegisterType<PageModel>(uri, 1, 0, "PageModel");
    qmlRegisterType<IconItem>(uri, 1, 0, "IconItem");
    qmlRegisterType<AppManager>(uri, 1, 0, "AppManager");

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    qmlRegisterType<QAbstractItemModel>();
#else
    qmlRegisterAnonymousType<QAbstractItemModel>(uri, 0);
#endif

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("cutefish-launcher"));

    QPixmapCache::setCacheLimit(2048);

    // 命令行参数
    QCommandLineParser parser;
    QCommandLineOption showOption(QStringLiteral("show"), "Show Launcher");
    parser.addOption(showOption);
    parser.process(app.arguments());

    // DBus 单实例
    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (!dbus.registerService(DBUS_NAME)) {
        QDBusInterface iface(DBUS_NAME, DBUS_PATH, DBUS_INTERFACE, dbus);
        iface.call("toggle");
        return -1;
    }

    // 翻译文件加载
    QLocale locale;
    QString qmFilePath = QString("/usr/share/cutefish-launcher/translations/%1.qm").arg(locale.name());

    if (QFile::exists(qmFilePath)) {
        QTranslator *translator = new QTranslator(app.instance());
        if (translator->load(qmFilePath)) {
            app.installTranslator(translator);
        } else {
            translator->deleteLater();
        }
    }

    // 启动 Launcher
    bool firstShow = parser.isSet(showOption);
    Launcher launcher(firstShow);

    if (!dbus.registerObject(DBUS_PATH, DBUS_INTERFACE, &launcher))
        return -1;

    return app.exec();
}
