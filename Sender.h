#pragma once

#include <QtCore>
#include <QtNetwork>
#include <QSharedPointer>

class Sender : public QObject
{
    Q_OBJECT

public:
    explicit Sender(uint16_t group_port, const QString& ipv4_group, const QString& ipv6_group, QObject* parent = nullptr);

    void send_datagram(const QByteArray& datagram);

private:
    QTimer timer;

    QUdpSocket m_udp_socket_ipv4;
    QUdpSocket m_udp_socket_ipv6;

    QHostAddress m_group_address_ipv4;
    QHostAddress m_group_address_ipv6;

    uint16_t m_group_port{0};
/*
    bool m_ipv4_multicast_member{false};
    uint16_t m_ipv4_group_port{0};
    QUdpSocket* m_udp_socket_ipv4{nullptr};
    QHostAddress m_group_address_ipv4;

    bool m_ipv6_multicast_member{false};
    uint16_t m_ipv6_group_port{0};
    QUdpSocket* m_udp_socket_ipv6{nullptr};
    QHostAddress m_group_address_ipv6;
*/
};
