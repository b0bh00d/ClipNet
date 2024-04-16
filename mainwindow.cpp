#include <random>
#include <limits>
#include <functional>

#ifdef QT_WIN
#define WIN32_MEAN_AND_LEAN // necessary to avoid compiler errors
#include <Windows.h>

#define AS_LPCWSTR(str) reinterpret_cast<const wchar_t*>(str.utf16())
#define AS_LPBYTE(str) reinterpret_cast<const BYTE*>(str.utf16())
#endif
#ifdef QT_LINUX
#include <unistd.h>
#endif

#include <QTimer>
#include <QDateTime>
#include <QMimeData>
#include <QSettings>
#include <QDataStream>
#include <QMessageBox>
#include <QStandardPaths>
#include <QNetworkDatagram>
#include <QNetworkInterface>

#include <QJsonDocument>

#include "mainwindow.h"
#include "ui_mainwindow.h"

static const QString& settings_version = "1.0";

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    setWindowTitle(tr("ClipNet  by Bob Hood"));
    setWindowIcon(QIcon(":/images/ClipNet.png"));

    // generate a random integer to identify this sender
    std::random_device rd;
    std::mt19937 rd_mt(rd());
    std::uniform_int_distribution<> sender_id(1, std::numeric_limits<int>::max());
    m_sender_id = sender_id(rd_mt);

#ifdef QT_WIN
    TCHAR buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD buffer_size{MAX_COMPUTERNAME_LENGTH};
    if(GetComputerName(buffer, &buffer_size))
        m_host_name = QString::fromWCharArray(buffer);
    else
        m_host_name = QString::number(m_sender_id, 16);
#endif
#ifdef QT_LINUX
    QByteArray host(256, 0);
    gethostname(static_cast<char*>(host.data()), 255);
    m_host_name = QString::fromLatin1(host.data());
#endif

    m_housekeeping_timer.setInterval(1000);
    m_housekeeping_timer.callOnTimeout(this, &MainWindow::slot_housekeeping);

    load_settings();

    QFont f(font());
    f.setFamily("Consolas");
    m_ui->edit_Log->setFont(f);

#ifdef QT_WIN
    connect(m_ui->check_AutoStart, &QCheckBox::clicked, this, &MainWindow::slot_set_startup);
#endif
#ifdef QT_LINUX
    m_ui->check_AutoStart->setEnabled(false);
#endif

    connect(m_ui->check_AudioCue, &QCheckBox::clicked, this, &MainWindow::slot_set_control_states);
    connect(m_ui->check_VisualCue, &QCheckBox::clicked, this, &MainWindow::slot_set_control_states);

    connect(m_ui->check_Channels_IPv4, &QCheckBox::clicked, this, &MainWindow::slot_set_control_states);
    connect(m_ui->button_MulticastGroupIPv4_Randomize, &QPushButton::clicked, this, &MainWindow::slot_randomize_ipv4);
    connect(m_ui->check_Channels_IPv6, &QCheckBox::clicked, this, &MainWindow::slot_set_control_states);
    connect(m_ui->button_MulticastGroupIPv6_Randomize, &QPushButton::clicked, this, &MainWindow::slot_randomize_ipv6);

    connect(m_ui->button_Channels_Join, &QPushButton::clicked, this, &MainWindow::slot_multicast_group_join);
    connect(m_ui->check_Channels_AutoRejoin, &QCheckBox::clicked, this, &MainWindow::slot_set_control_states);

#if defined(USE_ENCRYPTION)
    connect(m_ui->check_Encryption, &QCheckBox::clicked, this, &MainWindow::slot_set_control_states);
#else
    m_ui->check_Encryption->setVisible(false);
    m_ui->line_Passphrase->setVisible(false);
#endif

    connect(m_ui->check_ClearClipboard, &QCheckBox::clicked, this, &MainWindow::slot_clear_clipboard);

    m_restore_action = new QAction(QIcon(":/images/Restore.png"), tr("&Restore"), this);
    connect(m_restore_action, &QAction::triggered, this, &MainWindow::showNormal);

    m_quit_action = new QAction(QIcon(":/images/Quit.png"), tr("&Quit"), this);
    connect(m_quit_action, &QAction::triggered, this, &MainWindow::slot_quit);

    m_trayIcon = new QSystemTrayIcon(this);
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::slot_tray_message_clicked);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::slot_tray_icon_activated);

    m_trayIcon->setIcon(QIcon(":/images/ClipNet.png"));
    m_trayIcon->setToolTip(tr("ClipNet"));
    build_tray_menu();

    m_trayIcon->show();

    m_clipboard = QGuiApplication::clipboard();

    QDir::setCurrent(qApp->applicationDirPath());

    m_cue = CuePointer(new Cue());
}

void MainWindow::build_tray_menu()
{
    if (m_trayIconMenu)
    {
        disconnect(m_trayIconMenu, &QMenu::triggered, this, &MainWindow::slot_tray_menu_action);
        delete m_trayIconMenu;
    }

    m_trayIconMenu = new QMenu(this);

    //trayIconMenu->addSeparator();

    m_trayIconMenu->addAction(m_restore_action);
    m_trayIconMenu->addAction(m_quit_action);

    m_trayIcon->setContextMenu(m_trayIconMenu);

    connect(m_trayIconMenu, &QMenu::triggered, this, &MainWindow::slot_tray_menu_action);
}

void MainWindow::setVisible(bool visible)
{
    //restoreAction->setEnabled(isMaximized() || !visible);
    QWidget::setVisible(visible);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_trayIcon->isVisible())
    {
        //QMessageBox::information(this,
        //                         tr("Tray"),
        //                         tr("The program will keep running in the "
        //                            "system tray. To terminate the program, "
        //                            "choose <b>Quit</b> in the context menu "
        //                            "of the system tray entry."));
        hide();
        event->ignore();
    }
}

void MainWindow::load_settings()
{
    m_multicast_group_member = false;

    QString settings_file_name;
#ifdef QT_WIN
    settings_file_name = QDir::toNativeSeparators(QString("%1/ClipNet/Settings.ini").arg(qgetenv("APPDATA").constData()));
#endif
#ifdef QT_LINUX
    settings_file_name = QDir::toNativeSeparators(QString("%1/ClipNet.ini").arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
#endif
    QSettings settings(settings_file_name, QSettings::IniFormat);

    m_ui->check_AutoStart->setChecked(settings.value("startup_enabled", false).toBool());

    m_ui->check_AudioCue->setChecked(settings.value("audio_cue", false).toBool());
    m_ui->check_VisualCue->setChecked(settings.value("visual_cue", false).toBool());

    m_ui->line_MulticastGroupPort->setText(settings.value("group_port", "").toString());

    //    m_protocol = static_cast<Protocol>(settings.value("protocol", 0).toInt());

    //    m_ui->group_MulticastGroupIPv4->setChecked(settings.value("ipv4_multicast_group_enabled", true).toBool());
    m_ui->check_Channels_IPv4->setChecked(settings.value("ipv4_multicast_group_enabled", false).toBool());
    m_ui->line_MulticastGroupIPv4->setText(settings.value("ipv4_multicast_group_address", "").toString());

    m_ui->check_Channels_IPv6->setChecked(settings.value("ipv6_multicast_group_enabled", false).toBool());
    m_ui->line_MulticastGroupIPv6->setText(settings.value("ipv6_multicast_group_address", "").toString());

    m_ui->check_Channels_AutoRejoin->setChecked(settings.value("channels_autorejoin", false).toBool());

#ifdef USE_ENCRYPTION
    m_ui->check_Encryption->setChecked(settings.value("use_encryption", false).toBool());
    auto passphrase{settings.value("passphrase", "").toString()};
    m_ui->line_Passphrase->setText(passphrase);
#endif

    m_ui->check_ClearClipboard->setChecked(settings.value("clear_clipboard", false).toBool());
    m_ui->line_ClearClipboardSeconds->setText(settings.value("clear_clipboard_seconds", "").toString());

    if (m_ui->check_ClearClipboard->isChecked())
        m_housekeeping_timer.start();

    QTimer::singleShot(0, this, &MainWindow::slot_set_control_states);

    if (!m_ui->check_Channels_AutoRejoin->isChecked())
        // open the main window to remind them that they need
        // to initiate the connection manually
        showNormal();
    else
        QTimer::singleShot(0, this, &MainWindow::slot_multicast_group_join);
}

void MainWindow::save_settings()
{
    QString settings_file_name;
#ifdef QT_WIN
    settings_file_name = QDir::toNativeSeparators(QString("%1/ClipNet/Settings.ini").arg(qgetenv("APPDATA").constData()));
#endif
#ifdef QT_LINUX
    settings_file_name = QDir::toNativeSeparators(QString("%1/ClipNet.ini").arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
#endif
    QSettings settings(settings_file_name, QSettings::IniFormat);

    settings.clear();

    settings.setValue("startup_enabled", m_ui->check_AutoStart->isChecked());

    settings.setValue("audio_cue", m_ui->check_AudioCue->isChecked());
    settings.setValue("visual_cue", m_ui->check_VisualCue->isChecked());

    settings.setValue("group_port", m_ui->line_MulticastGroupPort->text());

    settings.setValue("ipv4_multicast_group_enabled", m_ui->check_Channels_IPv4->isChecked());
    settings.setValue("ipv4_multicast_group_address", m_ui->line_MulticastGroupIPv4->text());

    settings.setValue("ipv6_multicast_group_enabled", m_ui->check_Channels_IPv6->isChecked());
    settings.setValue("ipv6_multicast_group_address", m_ui->line_MulticastGroupIPv6->text());

    settings.setValue("channels_autorejoin", m_ui->check_Channels_AutoRejoin->isChecked());

#if defined(USE_ENCRYPTION)
    settings.setValue("use_encryption", m_ui->check_Encryption->isChecked());
    settings.setValue("passphrase", m_ui->line_Passphrase->text());
#endif

    settings.setValue("clear_clipboard", m_ui->check_ClearClipboard->isChecked());
    settings.setValue("clear_clipboard_seconds", m_ui->line_ClearClipboardSeconds->text());
}

void MainWindow::notify_clipboard_event(const QByteArray& payload, const QString& display)
{
    auto packet_size{static_cast<int>(sizeof(Packet)) + static_cast<int>(payload.length())};
    auto data{QByteArray(packet_size, 0)};
    auto packet{reinterpret_cast<Packet*>(data.data())};

    packet->magic = magic_number;
    packet->sender = m_sender_id;
    packet->action = static_cast<int>(Action::ClipData);
    packet->payload_size = static_cast<int>(payload.length());
    ::memcpy(&packet->payload, payload.constData(), static_cast<size_t>(packet->payload_size));

    m_multicast_sender->send_datagram(data);

    if(m_ui->check_AudioCue->isChecked())
        QTimer::singleShot(0, m_cue.data(), &Cue::slot_trigger_audio);
    if(m_ui->check_VisualCue->isChecked() && !display.isEmpty())
        QTimer::singleShot(0, m_cue.data(), std::bind(&Cue::slot_trigger_visual, m_cue.data(), display));
}

void MainWindow::slot_housekeeping()
{
    if (m_clear_clipboard_countdown != -1)
    {
        if (--m_clear_clipboard_countdown == 0)
        {
            // clear the clipbaord
            m_clipboard->setText("");
            m_clear_clipboard_countdown = -1;
        }
    }
}

void MainWindow::slot_set_control_states()
{
    bool ipv4_enabled{m_ui->check_Channels_IPv4->isChecked()};
    bool ipv6_enabled{m_ui->check_Channels_IPv6->isChecked()};

    m_ui->line_MulticastGroupPort->setEnabled(!m_multicast_group_member);

    m_ui->line_MulticastGroupIPv4->setEnabled(ipv4_enabled && !m_multicast_group_member);
    m_ui->button_MulticastGroupIPv4_Randomize->setEnabled(ipv4_enabled && !m_multicast_group_member);

    m_ui->line_MulticastGroupIPv6->setEnabled(ipv6_enabled && !m_multicast_group_member);
    m_ui->button_MulticastGroupIPv6_Randomize->setEnabled(ipv6_enabled && !m_multicast_group_member);

    m_ui->check_Channels_AutoRejoin->setEnabled(!m_multicast_group_member);

#if defined(USE_ENCRYPTION)
    m_use_encryption = m_ui->check_Encryption->isChecked();
    m_ui->check_Encryption->setEnabled(!m_multicast_group_member);
    m_ui->line_Passphrase->setEnabled(m_use_encryption && !m_multicast_group_member);
#endif

    auto clear_clipboard{m_ui->check_ClearClipboard->isChecked()};
    m_ui->check_ClearClipboard->setEnabled(!m_multicast_group_member);
    m_ui->line_ClearClipboardSeconds->setEnabled(clear_clipboard && !m_multicast_group_member);
}

void MainWindow::slot_tray_menu_action(QAction* /*action*/)
{}

void MainWindow::slot_tray_message_clicked()
{
    QMessageBox::information(
        nullptr,
        tr("ClipNet"),
        tr("Sorry, I already gave what help I could.\n"
           "Maybe you should try asking a human?"));
}

void MainWindow::slot_tray_icon_activated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
        case QSystemTrayIcon::Trigger:
            setVisible(!isVisible());
            if (isVisible())
                activateWindow();
            break;

        case QSystemTrayIcon::DoubleClick:
        case QSystemTrayIcon::MiddleClick:
        default:
            break;
    }
}

void MainWindow::slot_process_peer_event(const QByteArray& datagram)
{
    auto packet{reinterpret_cast<const Packet*>(datagram.constData())};
    if (packet->magic == magic_number && packet->sender != m_sender_id)
    {
        auto timestamp{QDateTime::currentDateTime().toString()};

        switch (static_cast<Action>(packet->action))
        {
            case Action::ClipData:
                {
                    QString peer_id;
                    QString payload_text, payload_html;

#ifdef USE_ENCRYPTION
                    bool success{false};

                    peer_id = QString::number(packet->sender, 16);

                    QByteArray buffer(reinterpret_cast<const char*>(&packet->payload[0]), packet->payload_size);
                    auto decrypted{m_security->decrypt(buffer, success)};
                    if (success)
                    {
                        auto json{QJsonDocument::fromJson(decrypted)};
                        peer_id = json["host"].toString();
                        payload_text = json["text"].toString();
                        payload_html = json["html"].toString();
                    }
#else
                    auto buffer{QString::fromUtf8(reinterpret_cast<const char*>(&packet->payload[0]), packet->payload_size)};
                    auto json{QJsonDocument::fromJson(buffer)};
                    peer_id = json["host"].toString();
                    payload_text = json["text"].toString();
                    payload_html = json["html"].toString();
#endif

                    ++m_clipboard_debt;

                    {
                        QSignalBlocker blocker(m_clipboard);

                        auto data = new QMimeData();

                        if(!payload_text.isEmpty())
                            data->setText(payload_text);
                        if(!payload_html.isEmpty())
                            data->setHtml(payload_html);

                        m_clipboard->setMimeData(data);
                    }

                    if (m_ui->check_ClearClipboard->isChecked())
                    {
                        m_clear_clipboard_countdown = m_ui->line_ClearClipboardSeconds->text().toLongLong();
                        if (!m_clear_clipboard_countdown)
                            m_clear_clipboard_countdown = -1;
                    }

                    QStringList info;
                    info << timestamp << QString("Peer %1: Clipboard event").arg(peer_id);
                    m_ui->edit_Log->insertPlainText(QString("%1\n").arg(info.join(" :: ")));
                    m_ui->edit_Log->ensureCursorVisible();
                }
                break;

            default:
                break;
        }
    }
}

void MainWindow::slot_read_clipboard()
{
    if(m_clipboard_debt)
        --m_clipboard_debt;
    else
    {
        auto mime_data{m_clipboard->mimeData()};
        if (mime_data->hasText() || mime_data->hasHtml())
        {
            auto timestamp{QDateTime::currentDateTime().toString()};
            QStringList info;

            auto text{mime_data->text()};
            if(!text.isEmpty())
            {
                info << timestamp << tr("Sending clipboard data to multicast group");

                QJsonObject json;
                json["host"] = m_host_name;
                json["text"] = mime_data->hasText() ? mime_data->text() : "";
                json["html"] = mime_data->hasHtml() ? mime_data->html() : "";

                auto payload{QJsonDocument(json).toJson()};

                // braodcast new clipboard text to peers
#if defined(USE_ENCRYPTION)
                bool success{false};
                auto encrypted{m_security->encrypt(payload, success)};
                if (success)
                    notify_clipboard_event(encrypted, text);
#else
                notify_clipboard_event(payload.toUtf8(), text);
#endif

                m_ui->edit_Log->insertPlainText(QString("%1\n").arg(info.join(" :: ")));
                m_ui->edit_Log->ensureCursorVisible();
            }
        }
    }
}

void MainWindow::slot_quit()
{
    // do any cleanup needed...
    if (m_multicast_sender)
    {
        m_multicast_sender->deleteLater();
        m_multicast_sender = nullptr;
    }

    if (m_multicast_receiver)
    {
        m_multicast_receiver->deleteLater();
        m_multicast_receiver = nullptr;
    }

    save_settings();

    // ...and leave.
    QTimer::singleShot(100, qApp, &QApplication::quit);
}

void MainWindow::slot_multicast_group_join()
{
    save_settings();

    if (m_multicast_group_member)
    {
        disconnect(m_clipboard, &QClipboard::dataChanged, this, &MainWindow::slot_read_clipboard);

        m_ui->button_Channels_Join->setText(tr("Join"));

        m_multicast_sender->deleteLater();
        m_multicast_sender = nullptr;

        m_multicast_receiver->deleteLater();
        m_multicast_receiver = nullptr;

        m_security.clear();
    }
    else
    {
        connect(m_clipboard, &QClipboard::dataChanged, this, &MainWindow::slot_read_clipboard);

#if defined(USE_ENCRYPTION)
        m_use_encryption = m_ui->group_Encryption->isChecked() && !m_ui->line_Passphrase->text().isEmpty();

        auto passphrase{m_ui->line_Passphrase->text()};
        if (passphrase.isEmpty())
            // is there placeholder text?
            passphrase = m_ui->line_Passphrase->placeholderText();

        m_security = Secure::create(passphrase);
#endif

        m_ui->button_Channels_Join->setText(tr("Leave"));

        auto group_port_str{m_ui->line_MulticastGroupPort->text()};
        if (group_port_str.isEmpty())
            group_port_str = m_ui->line_MulticastGroupPort->placeholderText();
        auto group_port{static_cast<uint16_t>(group_port_str.toInt())};

        QString ipv4_multcast_group;
        if (m_ui->check_Channels_IPv4->isChecked())
        {
            ipv4_multcast_group = m_ui->line_MulticastGroupIPv4->text();
            if (ipv4_multcast_group.isEmpty())
                ipv4_multcast_group = m_ui->line_MulticastGroupIPv4->placeholderText();
        }

        QString ipv6_multcast_group;
        if (m_ui->check_Channels_IPv6->isChecked())
        {
            ipv6_multcast_group = m_ui->line_MulticastGroupIPv6->text();
            if (ipv6_multcast_group.isEmpty())
                ipv6_multcast_group = m_ui->line_MulticastGroupIPv6->placeholderText();
        }

        if (m_randomized_addresses && (!m_ui->line_MulticastGroupIPv4->text().isEmpty() || !m_ui->line_MulticastGroupIPv6->text().isEmpty()))
        {
            QMessageBox::warning(
                this,
                "Randomized Addresses",
                "You have randomized one or more multicast addresses.\n\n"
                "Please ensure other member machines are using the\n"
                "same new address or your ClipNet may not function.");

            m_randomized_addresses = false;
        }

        m_multicast_sender = new Sender(group_port, ipv4_multcast_group, ipv6_multcast_group, this);
        m_multicast_receiver = new Receiver(group_port, ipv4_multcast_group, ipv6_multcast_group, this);
        connect(m_multicast_receiver, &Receiver::signal_datagram_avaialble, this, &MainWindow::slot_process_peer_event);
    }

    m_multicast_group_member = !m_multicast_group_member;

    QTimer::singleShot(0, this, &MainWindow::slot_set_control_states);
}

void MainWindow::slot_randomize_ipv4()
{
    std::random_device rd;
    std::mt19937 rd_mt(rd());
    std::uniform_int_distribution<> byte(0, 255);

    auto b1{byte(rd_mt)};
    auto b2{byte(rd_mt)};
    auto b3{byte(rd_mt)};

    m_ui->line_MulticastGroupIPv4->setText(QString("239.%1.%2.%3").arg(b1).arg(b2).arg(b3));

    m_randomized_addresses = !m_randomized_addresses && true;
}

void MainWindow::slot_randomize_ipv6()
{
    std::random_device rd;
    std::mt19937 rd_mt(rd());
    std::uniform_int_distribution<> byte(1, 65535);

    m_ui->line_MulticastGroupIPv6->setText(QString("ff12::%1").arg(byte(rd_mt)));

    m_randomized_addresses = !m_randomized_addresses && true;
}

void MainWindow::slot_set_startup()
{
#ifdef QT_WIN
    HKEY hkey;

    QString key_str = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    QString label_str = "ClipNet";

    if (m_ui->check_AutoStart->isChecked())
    {
        QString buffer_str;

        wchar_t path[MAX_PATH];
        GetModuleFileName(nullptr, path, MAX_PATH);
        QString path_str = QString::fromWCharArray(path);

        if (!path_str.startsWith('"'))
            path_str = QString("\"%1\"").arg(path_str);

        if (RegOpenKeyEx(HKEY_CURRENT_USER, AS_LPCWSTR(key_str), 0, KEY_SET_VALUE, &hkey) == ERROR_SUCCESS)
        {
            int len = path_str.size();
#ifdef UNICODE
            len *= 2;
#endif
            RegSetValueEx(hkey, AS_LPCWSTR(label_str), 0, REG_SZ, AS_LPBYTE(path_str), len);
            RegCloseKey(hkey);
        }
    }
    else
    {
        if (RegOpenKeyEx(HKEY_CURRENT_USER, AS_LPCWSTR(key_str), 0, KEY_SET_VALUE, &hkey) == ERROR_SUCCESS)
        {
            RegDeleteValue(hkey, AS_LPCWSTR(label_str));
            RegCloseKey(hkey);
        }
    }
#endif
}

void MainWindow::slot_clear_clipboard()
{
    if (!m_ui->check_ClearClipboard->isChecked())
        m_housekeeping_timer.stop();
    else if (!m_housekeeping_timer.isActive())
        m_housekeeping_timer.start();

    QTimer::singleShot(0, this, &MainWindow::slot_set_control_states);
}
