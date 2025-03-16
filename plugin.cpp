#include <QDir>
#include <QFile>
#include <QIcon>
#include <QSettings>

#include <optional>

#include <extensionsystem/iplugin.h>

#include <QStringList>

class IconThemePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "nomoicontheme.json")

public:
    static QByteArray getDesktopEnvironment()
    {
        const auto xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP");
        if (!xdgCurrentDesktop.isEmpty()) {
            return xdgCurrentDesktop.toUpper();
        }
        if (!qEnvironmentVariableIsEmpty("KDE_FULL_SESSION")) {
            return QByteArrayLiteral("KDE");
        }
        if (!qEnvironmentVariableIsEmpty("GNOME_DESKTOP_SESSION_ID")) {
            return QByteArrayLiteral("GNOME");
        }
        auto desktopSession = qgetenv("DESKTOP_SESSION");
        int slash = desktopSession.lastIndexOf('/');
        if (slash != -1) {
            QSettings desktopFile(QFile::decodeName(desktopSession + ".desktop"),
                                  QSettings::IniFormat);
            desktopFile.beginGroup(QStringLiteral("Desktop Entry"));
            auto desktopName = desktopFile.value(QStringLiteral("DesktopNames")).toByteArray();
            if (!desktopName.isEmpty()) {
                return desktopName;
            }
            desktopSession = desktopSession.mid(slash + 1);
        }
        if (desktopSession == "gnome") {
            return QByteArrayLiteral("GNOME");
        }
        if (desktopSession == "xfce") {
            return QByteArrayLiteral("XFCE");
        }
        if (desktopSession == "kde") {
            return QByteArrayLiteral("KDE");
        }
        return QByteArrayLiteral("UNKNOWN");
    }

    explicit IconThemePlugin() noexcept
    {
        Q_INIT_RESOURCE(icontheme);

        const auto fallbackThemeName = []() -> std::optional<QString> {
            if (getDesktopEnvironment() == QByteArrayLiteral("KDE")) {
                auto configDir = qEnvironmentVariable("XDG_CONFIG_DIRS");
                if (configDir.isEmpty()) {
                    configDir = QDir::home().absolutePath() + "/.config";
                }

                auto settings = QSettings(configDir + "/kdeglobals", QSettings::IniFormat);
                auto val = settings.value("Icons/Theme");

                if (val.canConvert<QString>()) {
                    return val.toString();
                }

                return std::nullopt;
            }

            return QIcon::themeName();
        }();

        QIcon::setThemeName("qtcreator-nomo");
        if (fallbackThemeName.has_value()) {
            QIcon::setFallbackThemeName(*fallbackThemeName);
        }
    }

    bool initialize(const QStringList &arguments, QString *errorString) override { return true; }

    ExtensionSystem::IPlugin::ShutdownFlag aboutToShutdown() override
    {
        return SynchronousShutdown;
    }
};

#include <plugin.moc>
