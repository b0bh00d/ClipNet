#include "Sender.h"

// https://code.qt.io/cgit/qt/qtbase.git/tree/examples/network/multicastsender?h=5.15

Sender::Sender(uint16_t group_port, const QString& ipv4_group, const QString& ipv6_group, QObject* parent) :
    QObject(parent), m_group_address_ipv4(ipv4_group), m_group_address_ipv6(ipv6_group), m_group_port(group_port)
{
    // force binding to their respective families
    m_udp_socket_ipv4.bind(QHostAddress(QHostAddress::AnyIPv4), 0);
    m_udp_socket_ipv6.bind(QHostAddress(QHostAddress::AnyIPv6), m_udp_socket_ipv4.localPort());

    // make sure packets remain in this subnet
    // (one is the default, but I'm doing it explicitly to remind
    // readers of the limitation)
    m_udp_socket_ipv4.setSocketOption(QAbstractSocket::MulticastTtlOption, 1);
}

void Sender::send_datagram(const QByteArray& datagram)
{
    if (!m_group_address_ipv4.toString().isEmpty())
        m_udp_socket_ipv4.writeDatagram(datagram, m_group_address_ipv4, m_group_port);

    if (!m_group_address_ipv6.toString().isEmpty())
    {
        if (m_udp_socket_ipv6.state() == QAbstractSocket::BoundState)
            m_udp_socket_ipv6.writeDatagram(datagram, m_group_address_ipv6, m_group_port);
    }
}
