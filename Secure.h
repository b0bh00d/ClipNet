#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <QObject>
#include <QByteArray>
#include <QSharedPointer>

#ifdef CRYPTOPP
// Crypto++
#include "modes.h"
#include "aes.h"
#include "filters.h"
#include "osrng.h"
#include "hex.h"

const int MaxKeySize = 32U; // maximum of 256 bits (32 * 8) for key
#endif

#ifdef SIMPLECRYPT
#include "SimpleCrypt.h"
#endif

class Secure;
using secure_ptr_t = QSharedPointer<Secure>;

class Secure : public QObject
{
    Q_OBJECT

public: // aliases and enums
#ifdef CRYPTOPP
    // clang-format off
    // only use AES + SHA256 + CryptoPP::CFB for now; it is a step above
    // the obfuscation of SimpleCrypt.  if you feel your data requires
    // more industrial strength encryption than AES/SHA256, you can convert
    // this to something heavier.
    enum class Cipher
    {
        aes,        // The advanced encryption standard symmetric encryption algorithm. Standard: FIPS 197
    };

    enum class Hash
    {
        sha256,     // The 256-bit secure hash algorithm. Standard: FIPS 180-2, FIPS 198.
    };
    // clang-format on
#endif

public:
#ifdef CRYPTOPP
    explicit Secure(const Cipher cipher = Cipher::aes, QObject* parent = nullptr);
#endif
    explicit Secure(const QString& cipher = "AES", QObject* parent = nullptr);
    ~Secure();

    /*!
    This is a class-static method that serves as a Factory.  It
    accepts all the parameters, determines how to interpret the
    key value, and then generates a Secure instance configured
    for it.

    \param passphrase A text string representing the passphrase.
    \param cipher The cipher to use.
    \param hash The hash algorithm to use when generating the key.
    \returns A shared pointer to the Secure instance based on the arguments.
    */
    static secure_ptr_t create(const QString& passphrase, const QString& cipher = "AES", const QString& hash = "SHA256");

    /*!
    Set the passphrase value that will be used as the basis of key
    that will be used by the encryption and decryption actions.

    \param passphrase A text string representing the passphrase.
    \param hash The hash algorithm to use when generating the key.
    \returns A Boolean true if the key value was successfully generated.
    */
#ifdef CRYPTOPP
    bool set_key(const QByteArray& passphrase, Hash hash = Hash::sha256);
#endif

    /*!
    Set the passphrase value that will be used as the basis of key
    that will be used by the encryption and decryption actions.

    \param passphrase A text string representing the passphrase.
    \param hash A string version of the hash algorithm to use when generating the key.
    \returns A Boolean true if the key value was successfully generated.
    */
    bool set_key(const QByteArray& passphrase, const QString& hash_str = "AES");

    /*!
    Encrypt a buffer of data.  On success, the encrypted version of
    the data is returned as a separate buffer.

    \param in_buffer The data to be encrypted.
    \param success A Boolean value that will be set with the result of the encryption attempt.
    \returns A buffer that contains the successfully encrypted data.
    */
    QByteArray encrypt(const QByteArray& in_buffer, bool& success);
    QByteArray encrypt(const uint8_t* p_data, uint32_t size, bool& success);

    /*!
    Decrypt a buffer of data.  On success, the decrypted version of
    the data is returned as a separate buffer.

    \param in_buffer The data to be decrypted.
    \param success A Boolean value that will be set with the result of the decryption attempt.
    \returns A buffer that contains the successfully decrypted data.
    */
    QByteArray decrypt(const QByteArray& in_buffer, bool& success);

private: // aliases and enums
#ifdef CRYPTOPP
    // we are using CFB mode for simplicity (it is still stronger encryption
    // than SimpleCrypt).  you can go as far down the Crypto++ rabbit hole
    // as your heart desires, but this simplified AES encryption should be
    // more than sufficient.

    using cfb_aes_encryptor_ptr_t = std::unique_ptr<CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption>;
    using cfb_aes_decryptor_ptr_t = std::unique_ptr<CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption>;
#endif

#ifdef SIMPLECRYPT
    using simplecrypt_ptr_t = std::unique_ptr<SimpleCrypt>;
#endif

private: // methods
    void init();
    void close();

#ifdef SIMPLECRYPT
    quint64 sixty_four_hash(const QString& str);
#endif

#if 0
    void print_hex(const uint8_t* data, size_t len);
#endif

private: // data members
#ifdef CRYPTOPP
    CryptoPP::byte m_key[MaxKeySize]{0};
    CryptoPP::byte m_iv[CryptoPP::AES::BLOCKSIZE]{0};

    cfb_aes_encryptor_ptr_t m_cfbEncryption{nullptr};
    cfb_aes_decryptor_ptr_t m_cfbDecryption{nullptr};

    Cipher m_cipher{Cipher::aes};
    Hash m_hash{Hash::sha256};
#endif

#ifdef SIMPLECRYPT
    simplecrypt_ptr_t m_simplecrypt;
#endif
};
