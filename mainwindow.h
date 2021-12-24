#pragma

#include <QMainWindow>

#include <QMenu>
#include <QTimer>
#include <QAction>
#include <QClipboard>
#include <QUdpSocket>
#include <QCloseEvent>
#include <QSystemTrayIcon>

#include "Secure.h"
#include "Sender.h"
#include "Receiver.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

namespace Ui
{
    class MainWindow;
}

constexpr int magic_number{('N' << 24) | ('T' << 16) | ('C' << 8) | 'L'};
constexpr int multicast_port{45454};

const int BroadcastPort = 59451;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:     // methods
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setVisible(bool visible = true);

protected: // methods
    void closeEvent(QCloseEvent *event);

private slots:
    void slot_set_control_states();

    void slot_tray_icon_activated(QSystemTrayIcon::ActivationReason reason);
    void slot_tray_message_clicked();
    void slot_tray_menu_action(QAction* action);

    void slot_process_peer_event(const QByteArray& datagram);

    void slot_read_clipboard();

    void slot_quit();

    void slot_multicast_group_join();

    void slot_randomize_ipv4();
    void slot_randomize_ipv6();

    void slot_set_startup();

    void slot_clear_clipboard();

    void slot_housekeeping();

private: // aliases and enums
    enum class Action : uint32_t
    {
        None,
        ClipData,
    };

    struct Packet
    {
        int magic{magic_number}; // uniquely identifies this data as belonging to ClipNet

        int sender{0}; // value unique to a sender; used to filter/discard captured packets

        int action{static_cast<uint32_t>(Action::None)};
        int payload_size;
        uint8_t payload[1];
    };

#ifdef SIMPLECRYPT
    using simplecrypt_ptr_t = QSharedPointer<SimpleCrypt>;
#endif

private: // methods
    void build_tray_menu();

    void load_settings();
    void save_settings();

    void notify_clipboard_event(const QByteArray& payload);

private: // data members
    Ui::MainWindow* m_ui{nullptr};

    QString m_host_name;

    QSystemTrayIcon* m_trayIcon{nullptr};
    QMenu* m_trayIconMenu{nullptr};
    QAction* m_restore_action{nullptr};
    QAction* m_quit_action{nullptr};

    Sender* m_multicast_sender{nullptr};
    Receiver* m_multicast_receiver{nullptr};

    bool m_multicast_group_member{false};

    QClipboard* m_clipboard;
    QString m_last_text;

    int m_sender_id{0};

    bool m_is_visible{true};
    bool m_randomized_addresses{false};

    QTimer m_housekeeping_timer;

    int64_t m_clear_clipboard_countdown{0};

    bool m_use_encryption{false};
    secure_ptr_t m_security{nullptr};
};
