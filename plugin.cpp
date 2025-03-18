#include <QDir>
#include <QFile>
#include <QIcon>
#include <QIconEngine>
#include <QPainter>
#include <QSettings>
#include <QStringList>
#include <QSvgRenderer>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qiconloader_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#include <extensionsystem/iplugin.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/hostosinfo.h>

#include <optional>

Q_LOGGING_CATEGORY(debugLog, "qtc.nomo.icontheme", QtDebugMsg)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SVGIconOnOffEngine
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class SVGIconOffOnEngine : public QIconEngine
{
    QByteArray dataOff;
    QByteArray dataOn;

public:
    explicit SVGIconOffOnEngine(const QString &offIcon, const QString &onIcon)
    {
        auto readFile = [](const QString fileName) -> QByteArray {
            if (fileName.isEmpty())
                return {};
            QFile file(fileName);
            file.open(QIODevice::ReadOnly);
            return file.readAll();
        };

        dataOff = readFile(offIcon);
        dataOn = readFile(onIcon);
    }
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
    {
        QSvgRenderer renderer(state == QIcon::Off ? dataOff : dataOn);
        renderer.render(painter, rect);
    }
    QIconEngine *clone() const override { return new SVGIconOffOnEngine(*this); }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        // This function is necessary to create an EMPTY pixmap. It's called always
        // before paint()
        QImage img(size, QImage::Format_ARGB32);
        img.fill(qRgba(0, 0, 0, 0));
        QPixmap pix = QPixmap::fromImage(img, Qt::NoFormatConversion);
        {
            QPainter painter(&pix);
            QRect r(QPoint(0.0, 0.0), size);
            this->paint(&painter, r, mode, state);
        }
        return pix;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NomoPlatformTheme
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class NomoPlatformTheme : public QPlatformTheme
{
public:
    QPlatformTheme *thePlatformTheme = nullptr;

    NomoPlatformTheme(QPlatformTheme *platformTheme)
        : thePlatformTheme(platformTheme)
    {}

    virtual QVariant themeHint(ThemeHint hint) const
    {
        if (hint == QPlatformTheme::PreferFileIconFromTheme) {
            qCDebug(debugLog) << "themeHint" << "QPlatformTheme::PreferFileIconFromTheme";
            return false;
        }
        return thePlatformTheme->themeHint(hint);
    }

    virtual QPlatformMenuItem *createPlatformMenuItem() const
    {
        return thePlatformTheme->createPlatformMenuItem();
    }
    virtual QPlatformMenu *createPlatformMenu() const
    {
        return thePlatformTheme->createPlatformMenu();
    }
    virtual QPlatformMenuBar *createPlatformMenuBar() const
    {
        return thePlatformTheme->createPlatformMenuBar();
    }
    virtual void showPlatformMenuBar() { return thePlatformTheme->showPlatformMenuBar(); }

    virtual bool usePlatformNativeDialog(DialogType type) const
    {
        return thePlatformTheme->usePlatformNativeDialog(type);
    }
    virtual QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const
    {
        return thePlatformTheme->createPlatformDialogHelper(type);
    }

    virtual QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const
    {
        return thePlatformTheme->createPlatformSystemTrayIcon();
    }
    virtual Qt::ColorScheme colorScheme() const { return thePlatformTheme->colorScheme(); }

    virtual const QPalette *palette(Palette type = SystemPalette) const
    {
        return thePlatformTheme->palette(type);
    }

    virtual const QFont *font(Font type = SystemFont) const { return thePlatformTheme->font(type); }

    virtual QPixmap standardPixmap(StandardPixmap sp, const QSizeF &size) const
    {
        qCDebug(debugLog) << "standardPixmap" << sp << size;

        static QHash<StandardPixmap, QIcon> standardPixmapHash{
                                                               {QPlatformTheme::DirIcon,
                                                                QIcon(":/icons/qtcreator-nomo/scalable/places/folder-hack.svg")},
                                                               {QPlatformTheme::DirOpenIcon,
                                                                QIcon(":/icons/qtcreator-nomo/scalable/places/folder-open.svg")},
                                                               {QPlatformTheme::FileIcon,
                                                                QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/text-x-generic.svg")}};

        if (standardPixmapHash.contains(sp))
            return standardPixmapHash.value(sp).pixmap(size.width(), size.height());

        return thePlatformTheme->standardPixmap(sp, size);
    }
    virtual QIcon fileIcon(const QFileInfo &fileInfo,
                           QPlatformTheme::IconOptions iconOptions = {}) const
    {
        qCDebug(debugLog) << "fileIcon" << fileInfo.filePath();

        static std::optional<QIcon> directoryIcon;
        if (!directoryIcon.has_value()) {
            QIcon icon(":/icons/qtcreator-nomo/scalable/places/folder-hack.svg");
            icon.addFile(":/icons/qtcreator-nomo/scalable/places/folder-open.svg",
                         QSize(),
                         QIcon::Normal,
                         QIcon::On);
            directoryIcon = icon;
        }
        if (fileInfo.isDir())
            return *directoryIcon;

        static QIcon libraryIcon(":/icons/qtcreator-nomo/scalable/mimetypes/library.svg");
        const QString suffix = fileInfo.suffix();
        if (Utils::HostOsInfo::isMacHost()) {
            if (suffix == "a" || suffix == "dylib")
                return libraryIcon;
        } else if (Utils::HostOsInfo::isWindowsHost()) {
            if (suffix == "a" || suffix == "lib" || suffix == "dll")
                return libraryIcon;
        } else {
            if (suffix == "a" || "so")
                return libraryIcon;
        }

        static QIcon genericFileIcon(
            ":/icons/qtcreator-nomo/scalable/mimetypes/text-x-generic.svg");
        return genericFileIcon;
        //return thePlatformTheme->fileIcon(fileInfo, iconOptions);
    }
    virtual QIconEngine *createIconEngine(const QString &iconName) const
    {
        qCDebug(debugLog) << "createIconEngine" << iconName;

        // The "folder-hack.svg" was needed so that QIconLoader would miss "folder.svg" as
        // part of the theme and then try to get the QPlatformTheme IconEngine to load it.
        // All of this was required due to the QThemeIconEngine doesn't associate QIcon::On states.
        if (iconName == "folder") {
            return new SVGIconOffOnEngine(":/icons/qtcreator-nomo/scalable/places/folder-hack.svg",
                                          ":/icons/qtcreator-nomo/scalable/places/folder-open.svg");
        }
        if (iconName == "text-x-generic") {
            return new SVGIconOffOnEngine(
                ":/icons/qtcreator-nomo/scalable/mimetypes/text-x-generic.svg", QString());
        }

        return thePlatformTheme->createIconEngine(iconName);
    }

    virtual QList<QKeySequence> keyBindings(QKeySequence::StandardKey key) const
    {
        return thePlatformTheme->keyBindings(key);
    }

    virtual QString standardButtonText(int button) const
    {
        return thePlatformTheme->standardButtonText(button);
    }
    virtual QKeySequence standardButtonShortcut(int button) const
    {
        return thePlatformTheme->standardButtonShortcut(button);
    }
    virtual void requestColorScheme(Qt::ColorScheme scheme)
    {
        return thePlatformTheme->requestColorScheme(scheme);
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IconThemePlugin
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class IconThemePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "NomoIconTheme.json")

    NomoPlatformTheme nomoPlatformTheme;

public:
    explicit IconThemePlugin()
        : nomoPlatformTheme(QGuiApplicationPrivate::platformTheme())
    {
        Q_INIT_RESOURCE(icontheme);

        // Code needed to get asked about Icons from the QGuiApplication
        QGuiApplicationPrivate::platform_theme = &nomoPlatformTheme;

        if (Utils::HostOsInfo::isMacHost())
            QIcon::setThemeName("qtcreator-nomo");
    }

    bool initialize(const QStringList &arguments, QString *errorString) override {
        Utils::FileIconProvider::registerIconForMimeType(
            QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/text-markdown.svg"), "text/markdown");
        Utils::FileIconProvider::registerIconForMimeType(
            QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/text-x-lua.svg"), "text/x-lua");
        Utils::FileIconProvider::registerIconForMimeType(
            QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/application-json.svg"),
            "application/json");
        Utils::FileIconProvider::registerIconForMimeType(
            QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/application-x-yaml.svg"),
            "application/x-yaml");
        Utils::FileIconProvider::registerIconForMimeType(
            QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/application-xml.svg"),
            "application/xml");
        Utils::FileIconProvider::registerIconForMimeType(
            QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/text-plain.svg"), "text/plain");
        Utils::FileIconProvider::registerIconForMimeType(
            QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/text-html.svg"), "text/html");

        for (auto imageMimeType : {"image/svg+xml",
                                   "image/png",
                                   "image/jpeg",
                                   "image/gif",
                                   "image/webp",
                                   "image/x-xpixmap",
                                   "image/vnd.microsoft.icon"}) {
            Utils::FileIconProvider::registerIconForMimeType(
                QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/image.svg"),
                QString::fromLatin1(imageMimeType));
        }
        for (auto shMimeType : {"application/x-sh", "application/x-bat", "text/x-applescript"}) {
            Utils::FileIconProvider::registerIconForMimeType(
                QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/application-x-sh.svg"),
                QString::fromLatin1(shMimeType));
        }
        for (auto fontMimeType : {"application/x-font-ttf",
                                  "application/x-font-otf",
                                  "application/x-font-type1",
                                  "application/x-font-ghostscript"}) {
            Utils::FileIconProvider::registerIconForMimeType(
                QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/application-x-font-ttf.svg"),
                QString::fromLatin1(fontMimeType));
        }
        for (auto archiveMimeType : {"application/zip",
                                     "application/gzip",
                                     "application/zstd",
                                     "application/x-bzip2",
                                     "application/x-bzip2-compressed-tar",
                                     "application/x-7z-compressed",
                                     "application/x-lzma",
                                     "application/x-compressed-tar",
                                     "application/x-xz-compressed-tar"}) {
            Utils::FileIconProvider::registerIconForMimeType(
                QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/application-zip.svg"),
                QString::fromLatin1(archiveMimeType));
        }
        for (auto exeMimeType : {"application/x-executable", "application/x-dosexec"}) {
            Utils::FileIconProvider::registerIconForMimeType(
                QIcon(":/icons/qtcreator-nomo/scalable/mimetypes/application-x-executable.svg"),
                QString::fromLatin1(exeMimeType));
        }
        return true;
    }

    ShutdownFlag aboutToShutdown() override
    {
        QGuiApplicationPrivate::platform_theme = nomoPlatformTheme.thePlatformTheme;
        return SynchronousShutdown;
    }
};

#include <plugin.moc>
