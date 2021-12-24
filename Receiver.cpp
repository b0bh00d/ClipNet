#include <QtWidgets>
#include <QtNetwork>

#include "Receiver.h"

// https://code.qt.io/cgit/qt/qtbase.git/tree/examples/network/multicastreceiver?h=5.15

Receiver::Receiver(uint16_t group_port, const QString& ipv4_group, const QString& ipv6_group, QObject* parent) :
    QObject(parent), group_address_ipv4(ipv4_group), group_address_ipv6(ipv6_group), m_group_port(group_port)
{
    udp_socket_ipv4.bind(QHostAddress::AnyIPv4, m_group_port, QUdpSocket::ShareAddress);
    udp_socket_ipv4.joinMulticastGroup(group_address_ipv4);

    if (udp_socket_ipv6.bind(QHostAddress::AnyIPv6, m_group_port, QUdpSocket::ShareAddress))
        udp_socket_ipv6.joinMulticastGroup(group_address_ipv6);

    connect(&udp_socket_ipv4, &QUdpSocket::readyRead, this, &Receiver::slot_process_datagrams);
    connect(&udp_socket_ipv6, &QUdpSocket::readyRead, this, &Receiver::slot_process_datagrams);
}

Receiver::~Receiver()
{
    udp_socket_ipv4.leaveMulticastGroup(group_address_ipv4);
    udp_socket_ipv6.leaveMulticastGroup(group_address_ipv6);
}

void Receiver::slot_process_datagrams()
{
    // using QUdpSocket::readDatagram (API since Qt 4)
    while (udp_socket_ipv4.hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(static_cast<int>(udp_socket_ipv4.pendingDatagramSize()));
        udp_socket_ipv4.readDatagram(datagram.data(), datagram.size());

        emit signal_datagram_avaialble(datagram);
    }

    // using QUdpSocket::receiveDatagram (API since Qt 5.8)
    while (udp_socket_ipv6.hasPendingDatagrams())
    {
        auto dgram{udp_socket_ipv6.receiveDatagram()};

        emit signal_datagram_avaialble(dgram.data());
    }
}
