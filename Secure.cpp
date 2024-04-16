#include <random>
#include <fstream>
#include <iostream>

#ifdef SIMPLECRYPT
#include <QDataStream>
#include <QCryptographicHash>
#endif
#ifdef OBFUSCATION
#include <QDate>
#include <QTime>
#endif
#include <QSharedPointer>

#include "Secure.h"

#ifdef CRYPTOPP
static const std::vector<uint8_t> salt{'d', '5', 'y', 'N', '+', '/', '?', '/', ')', 'e', 'j', 'z', 'O', '9', 'Q'};
#endif

#ifdef OBFUSCATION
static const std::vector<uint8_t> salt{
0x2e, 0x24, 0x2c, 0x21, 0x2e, 0x78, 0x59, 0x31,
0x4a, 0x58, 0x24, 0x66, 0x78, 0x42, 0x7e, 0x4c,
0x77, 0x79, 0x2e, 0x53, 0x30, 0x27, 0x7b, 0x78,
0x57, 0x75, 0x44, 0x24, 0x7d, 0x43, 0x3f, 0x79
};
#endif

#if 0
static void print_hex(const uint8_t* data, size_t len);
#endif

//------------------------------------------------
// Factory methods

secure_ptr_t Secure::create(const QString& passphrase, const QString& cipher, const QString& hash)
{
#ifdef CRYPTOPP
    QByteArray key_buffer;

    if (!passphrase.isEmpty())
    {
        std::ifstream ifs(passphrase.toLatin1(), std::ios::in | std::ios::binary | std::ios::ate);
        if (ifs.good())
        {
            assert(ifs.is_open());

            ifs.seekg(0, std::ios::end);
            std::ifstream::pos_type file_size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            key_buffer.resize(int(salt.size()) + int(file_size));

            ifs.read(reinterpret_cast<char*>(key_buffer.data()) + salt.size(), file_size);
            ifs.close();

            ::memcpy(key_buffer.data(), salt.data(), salt.size());

            auto secure_ptr{secure_ptr_t(new Secure(cipher))};
            secure_ptr->set_key(key_buffer, hash);

            return secure_ptr;
        }
        else
        {
            // treat it as a passphrase

            auto buffer{passphrase.toUtf8()};
            key_buffer.resize(int(salt.size()) + int(buffer.size()));

            ::memcpy(key_buffer.data(), salt.data(), salt.size());
            ::memcpy(key_buffer.data() + salt.size(), buffer.constData(), size_t(buffer.size()));

            auto secure_ptr{secure_ptr_t(new Secure(cipher))};
            secure_ptr->set_key(key_buffer, hash);

            return secure_ptr;
        }
    }

    // if they are creating a Secure instance, it stands to reason the want
    // SOME kind of security.  therefore, if the instance does not have a
    // passphrase, then at least the the salt value will serve to provide
    // SOME protection.

    key_buffer.resize(int(salt.size()));
    ::memcpy(key_buffer.data(), salt.data(), salt.size());

    auto secure_ptr{secure_ptr_t(new Secure(cipher))};
    secure_ptr->set_key(key_buffer, hash);
#endif

#if defined (SIMPLECRYPT) || defined (OBFUSCATION)
    Q_UNUSED(passphrase)
    Q_UNUSED(cipher)
    Q_UNUSED(hash)

    auto secure_ptr{secure_ptr_t(new Secure())};
    // create the internal SimpleCrypt instance with the passphrase
    secure_ptr->set_key(passphrase.toUtf8());
#endif

    return secure_ptr;
}

//------------------------------------------------
// Instance methods

#ifdef CRYPTOPP
Secure::Secure(Cipher cipher, QObject* parent) : QObject(parent), m_cipher(cipher)
{
    init();
}
#endif

Secure::Secure(const QString& cipher, QObject* parent) : QObject(parent)
{
#ifdef CRYPTOPP
    auto lc{cipher.toLower()};

    if (lc == "aes")
        m_cipher = Cipher::aes;
#endif

#if defined (SIMPLECRYPT) || defined (OBFUSCATION)
    Q_UNUSED(cipher)
#endif

    init();
}

Secure::~Secure()
{
    close();
}

void Secure::init()
{
#ifdef CRYPTOPP
    ::memset(m_key, 0x00, MaxKeySize);
    ::memset(m_iv, 0x00, CryptoPP::AES::BLOCKSIZE);
#endif
}

void Secure::close()
{
#ifdef CRYPTOPP
    if (m_cfbEncryption.get())
        m_cfbEncryption.reset();

    if (m_cfbDecryption.get())
        m_cfbDecryption.reset();
#endif

#ifdef SIMPLECRYPT
    if(m_simplecrypt.get())
        m_simplecrypt.reset();
#endif
}

#ifdef CRYPTOPP
bool Secure::set_key(const QByteArray& passphrase, Hash hash)
{
    close();

    // the initialization vector (IV) must be as random as tolerable,
    // but also--since we are using different cryptographic providers
    // on different platforms--deterministically generated so it starts
    // the same on all platforms, otherwise, decryption across
    // cryptographic providers WILL NOT WORK!

    std::minstd_rand rand_generator(0xC0FFEE);
    for (uint8_t& crypto_val : m_iv)
        crypto_val = uint8_t(rand_generator() >> 16);

    // hash the passphrase into the key
    if (hash == Hash::sha256)
    {
        CryptoPP::SHA256 cpp_hash;
        CryptoPP::ArraySource as(
            reinterpret_cast<const CryptoPP::byte*>(passphrase.constData()),
            size_t(passphrase.size()),
            true,
            new CryptoPP::HashFilter(cpp_hash, new CryptoPP::ArraySink(&m_key[0], MaxKeySize)));
    }
    else
        assert(false && "Only SHA256 is currently supported!");

    return true;
}
#endif

#ifdef SIMPLECRYPT
quint64 Secure::sixty_four_hash(const QString& str)
{
    QByteArray hash =
        QCryptographicHash::hash(QByteArray::fromRawData(reinterpret_cast<const char*>(str.utf16()), str.length() * 2), QCryptographicHash::Md5);
    Q_ASSERT(hash.size() == 16);
    QDataStream stream(hash);
    quint64 a, b;
    stream >> a >> b;
    return a ^ b;
};
#endif

#ifdef OBFUSCATION
QByteArray Secure::newKey(int pepper, int start, int end)
{
    Q_UNUSED(start)
    Q_UNUSED(end)

    std::vector<uint8_t> local_salt;

    if((pepper & 0x1) == 1) // odd?
    {
        // walk-swap the bytes
        for(size_t i = 1;i < salt.size(); i += 2)
        {
            local_salt.push_back(salt[i]);
            local_salt.push_back(salt[i-1]);
        }
    }
    else    // rotate
    {
//        auto total_rotations = julian / salt.size();
        auto offset = pepper % salt.size();
        for(size_t count = 0; count < salt.size(); ++count)
        {
            local_salt.push_back(salt[offset]);
            if(++offset == salt.size())
                offset = 0;
        }
    }

    assert(local_salt.size() == salt.size());

    QByteArray key;

    size_t offset = 0;
    for(int i = 0;i < passphrase.length(); ++i)
    {
        key.push_back(passphrase[i] ^ local_salt[offset]);
        if(++offset == local_salt.size())
            offset = 0;
    }

    return key;
}

QByteArray Secure::newDoyKey()
{
    QDate now = QDate::currentDate();
    return newKey(now.dayOfYear(), 1, 366);
}

QByteArray Secure::newHourKey()
{
    QTime now = QTime::currentTime();
    return newKey(now.hour() + 1, 1, 24);
}

QByteArray Secure::newMonthKey()
{
    QDate now = QDate::currentDate();
    return newKey(now.month(), 1, 12);
}
#endif

bool Secure::set_key(const QByteArray& passphrase, const QString& hash_str)
{
#ifdef CRYPTOPP
    auto local_hash{Hash::sha256};

    auto hs{hash_str.toLower()};

    if (hs == "sha256")
        local_hash = Hash::sha256;

    return set_key(passphrase, local_hash);
#endif

#ifdef SIMPLECRYPT
    Q_UNUSED(hash_str)

    auto init{sixty_four_hash(passphrase)};
    m_simplecrypt = simplecrypt_ptr_t(new SimpleCrypt(init));

    return true;
#endif

#ifdef OBFUSCATION
    Q_UNUSED(hash_str)

    this->passphrase = passphrase;

    return true;
#endif
}

QByteArray Secure::encrypt(const QByteArray& in_buffer, bool& success)
{
#ifdef CRYPTOPP
    return encrypt(reinterpret_cast<const uint8_t*>(in_buffer.constData()), uint32_t(in_buffer.size()), success);
#endif

#ifdef SIMPLECRYPT
    auto encrypted{m_simplecrypt->encryptToString(QString(in_buffer))};
    success = true;
    return encrypted.toUtf8();
#endif

#ifdef OBFUSCATION
    QByteArray buffer(in_buffer.length(), 0);

    auto key = newDoyKey();

    int offset = 0;
    for(int i = 0;i < in_buffer.length(); ++i)
    {
        buffer[i] = in_buffer[i] ^ key[offset];
        if(++offset == key.size())
            offset = 0;
    }

    success = true;
    return buffer;
#endif
}

#ifdef CRYPTOPP
QByteArray Secure::encrypt(const uint8_t* p_data, uint32_t in_size, bool& success)
{
    success = true;

    QByteArray out_buffer;
    out_buffer.resize(static_cast<int>(in_size));

    try
    {
        if (!m_cfbEncryption)
            m_cfbEncryption.reset(new CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption);
        m_cfbEncryption->SetKeyWithIV(&m_key[0], sizeof(m_key), &m_iv[0]);
        m_cfbEncryption->ProcessData(reinterpret_cast<CryptoPP::byte*>(out_buffer.data()), p_data, in_size);
    }
    catch (const CryptoPP::Exception& e)
    {
        std::cerr << e.what() << std::endl;
        success = false;
    }

    assert(success);

    return out_buffer;
}
#endif

QByteArray Secure::decrypt(const QByteArray& in_buffer, bool& success)
{
    success = true;

#ifdef CRYPTOPP
    QByteArray out_buffer;
    out_buffer.resize(in_buffer.size());

    try
    {
        if (!m_cfbDecryption)
            m_cfbDecryption.reset(new CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption);
        m_cfbDecryption->SetKeyWithIV(&m_key[0], sizeof(m_key), &m_iv[0]);
        m_cfbDecryption->ProcessData(
            reinterpret_cast<CryptoPP::byte*>(out_buffer.data()), reinterpret_cast<const CryptoPP::byte*>(in_buffer.constData()), static_cast<size_t>(in_buffer.size()));
    }
    catch (const CryptoPP::Exception& e)
    {
        std::cerr << e.what() << std::endl;
        success = false;
    }

    assert(success);

    return out_buffer;
#endif

#ifdef SIMPLECRYPT
    auto decrypted{m_simplecrypt->decryptToString(QString(in_buffer))};
    success = true;
    return decrypted.toUtf8();
#endif

#ifdef OBFUSCATION
    QByteArray buffer(in_buffer.length(), 0);

    auto key = newDoyKey();

    int offset = 0;
    for(int i = 0;i < in_buffer.length(); ++i)
    {
        buffer[i] = in_buffer[i] ^ key[offset];
        if(++offset == key.size())
            offset = 0;
    }

    success = true;
    return buffer;
#endif
}

#if 0
#ifdef CRYPTOPP
// function to help with debugging encryption/decryption data
void print_hex(const uint8_t* data, size_t len)
{
    // display the hash for comparison (debugging)
    std::string encoded;

    CryptoPP::HexEncoder encoder;
    encoder.Put(reinterpret_cast<const CryptoPP::byte*>(data), len);
    encoder.MessageEnd();

    auto size{encoder.MaxRetrievable()};
    if (size)
    {
        encoded.resize(size);
        encoder.Get((CryptoPP::byte*)&encoded[0], encoded.size());
        std::cout << encoded << std::endl;
        std::cout.flush();
    }
}
#endif
#endif
