#pragma once

#include <QUdpSocket>
#include <QHostAddress>
#include <QSharedPointer>

class Receiver : public QObject
{
    Q_OBJECT

public:
    explicit Receiver(uint16_t group_port, const QString& ipv4_group, const QString& ipv6_group, QObject *parent = nullptr);
    virtual ~Receiver();

signals:
    void signal_datagram_avaialble(const QByteArray& dg);

private slots:
    void slot_process_datagrams();

private:
    QUdpSocket udp_socket_ipv4;
    QUdpSocket udp_socket_ipv6;

    QHostAddress group_address_ipv4;
    QHostAddress group_address_ipv6;

    uint16_t m_group_port{0};
};
