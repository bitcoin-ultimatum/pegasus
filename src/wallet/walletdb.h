// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Copyright (c) 2016-2020 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLETDB_H
#define BITCOIN_WALLETDB_H

#include "amount.h"
#include "wallet/db.h"
#include "key.h"
#include "keystore.h"
#include "script/keyorigin.h"
#include "zbtcu/zerocoin.h"
#include "libzerocoin/Accumulator.h"
#include "libzerocoin/Denominations.h"
#include "zbtcu/zbtcutracker.h"
#include "wallet/hdchain.h"
#include "crates/rustzcash-lib/src/librustzcash.h"

#include <list>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

class CAccount;
class CAccountingEntry;
class CBTCUAddress;
struct CBlockLocator;
class CKeyPool;
class CMasterKey;
class CScript;
class CWallet;
class CWalletTx;
class CDeterministicMint;
class CZerocoinMint;
class CZerocoinSpend;
class uint160;
class uint256;

/** Backend-agnostic database type. */
using WalletDatabase = BerkeleyDatabase;

/** Error statuses for the wallet database */
enum DBErrors {
    DB_LOAD_OK,
    DB_CORRUPT,
    DB_NONCRITICAL_ERROR,
    DB_TOO_NEW,
    DB_LOAD_FAIL,
    DB_NEED_REWRITE
};

class CKeyMetadata
{
public:
    static const int CURRENT_VERSION = 1;
    int nVersion;
    CKeyID hd_seed_id; //id of the HD seed used to derive this key
    int64_t nCreateTime; // 0 means unknown
    KeyOriginInfo key_origin; // Key origin info with path and fingerprint

    CKeyMetadata()
    {
        SetNull();
    }
    CKeyMetadata(int64_t nCreateTime_)
    {
        nVersion = CKeyMetadata::CURRENT_VERSION;
        nCreateTime = nCreateTime_;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nCreateTime);
    }

    void SetNull()
    {
        nVersion = CKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
    }
};

/** Access to the wallet database (wallet.dat) */
class CWalletDB : public CDB
{
public:
    CWalletDB(const std::string& strFilename, const char* pszMode = "r+", bool fFlushOnClose = true) : CDB(strFilename, pszMode, fFlushOnClose)
    {
    }

    bool WriteName(const std::string& strAddress, const std::string& strName);
    bool EraseName(const std::string& strAddress);

    bool WritePurpose(const std::string& strAddress, const std::string& purpose);
    bool ErasePurpose(const std::string& strAddress);

    bool WriteTx(uint256 hash, const CWalletTx& wtx);
    bool EraseTx(uint256 hash);

    bool WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CKeyMetadata& keyMeta);
    bool WriteCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, const CKeyMetadata& keyMeta);
    bool WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey);

    bool WriteCScript(const uint160& hash, const CScript& redeemScript);

    bool WriteWatchOnly(const CScript& script);
    bool EraseWatchOnly(const CScript& script);

    bool WriteMultiSig(const CScript& script);
    bool EraseMultiSig(const CScript& script);

    bool WriteBestBlock(const CBlockLocator& locator);
    bool ReadBestBlock(CBlockLocator& locator);

    bool WriteOrderPosNext(int64_t nOrderPosNext);

    // presstab
    bool WriteStakeSplitThreshold(uint64_t nStakeSplitThreshold);
    bool WriteMultiSend(std::vector<std::pair<std::string, int> > vMultiSend);
    bool EraseMultiSend(std::vector<std::pair<std::string, int> > vMultiSend);
    bool WriteMSettings(bool fMultiSendStake, bool fMultiSendMasternode, int nLastMultiSendHeight);
    bool WriteMSDisabledAddresses(std::vector<std::string> vDisabledAddresses);
    bool EraseMSDisabledAddresses(std::vector<std::string> vDisabledAddresses);
    bool WriteAutoCombineSettings(bool fEnable, CAmount nCombineThreshold);

    bool ReadPool(int64_t nPool, CKeyPool& keypool);
    bool WritePool(int64_t nPool, const CKeyPool& keypool);
    bool ErasePool(int64_t nPool);

    bool WriteMinVersion(int nVersion);

    /// This writes directly to the database, and will not update the CWallet's cached accounting entries!
    /// Use wallet.AddAccountingEntry instead, to write *and* update its caches.
    bool WriteAccountingEntry_Backend(const CAccountingEntry& acentry);

    bool ReadAccount(const std::string& strAccount, CAccount& account);
    bool WriteAccount(const std::string& strAccount, const CAccount& account);

    /// Write destination data key,value tuple to database
    bool WriteDestData(const std::string& address, const std::string& key, const std::string& value);
    /// Erase destination data tuple from wallet database
    bool EraseDestData(const std::string& address, const std::string& key);

    CAmount GetAccountCreditDebit(const std::string& strAccount);
    void ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& acentries);

    DBErrors ReorderTransactions(CWallet* pwallet);
    DBErrors LoadWallet(CWallet* pwallet);
    DBErrors FindWalletTx(CWallet* pwallet, std::vector<uint256>& vTxHash, std::vector<CWalletTx>& vWtx);
    DBErrors ZapWalletTx(CWallet* pwallet, std::vector<CWalletTx>& vWtx);
    static bool Recover(CDBEnv& dbenv, std::string filename, bool fOnlyKeys);
    static bool Recover(CDBEnv& dbenv, std::string filename);

    bool WriteDeterministicMint(const CDeterministicMint& dMint);
    bool ReadDeterministicMint(const uint256& hashPubcoin, CDeterministicMint& dMint);
    bool EraseDeterministicMint(const uint256& hashPubcoin);
    bool WriteZerocoinMint(const CZerocoinMint& zerocoinMint);

    bool EraseZerocoinMint(const CZerocoinMint& zerocoinMint);
    bool ReadZerocoinMint(const CBigNum &bnPubcoinValue, CZerocoinMint& zerocoinMint);
    bool ReadZerocoinMint(const uint256& hashPubcoin, CZerocoinMint& mint);
    bool ArchiveMintOrphan(const CZerocoinMint& zerocoinMint);
    bool ArchiveDeterministicOrphan(const CDeterministicMint& dMint);
    bool UnarchiveZerocoinMint(const uint256& hashPubcoin, CZerocoinMint& mint);
    bool UnarchiveDeterministicMint(const uint256& hashPubcoin, CDeterministicMint& dMint);
    std::list<CZerocoinMint> ListMintedCoins();
    std::list<CDeterministicMint> ListDeterministicMints();
    std::list<CZerocoinSpend> ListSpentCoins();
    std::list<CBigNum> ListSpentCoinsSerial();
    std::list<CZerocoinMint> ListArchivedZerocoins();
    std::list<CDeterministicMint> ListArchivedDeterministicMints();
    bool WriteZerocoinSpendSerialEntry(const CZerocoinSpend& zerocoinSpend);
    bool EraseZerocoinSpendSerialEntry(const CBigNum& serialEntry);
    bool ReadZerocoinSpendSerialEntry(const CBigNum& bnSerial);
    bool WriteCurrentSeedHash(const uint256& hashSeed);
    bool ReadCurrentSeedHash(uint256& hashSeed);
    bool WriteZBTCUSeed(const uint256& hashSeed, const std::vector<unsigned char>& seed);
    bool ReadZBTCUSeed(const uint256& hashSeed, std::vector<unsigned char>& seed);
    bool ReadZBTCUSeed_deprecated(uint256& seed);
    bool EraseZBTCUSeed();
    bool EraseZBTCUSeed_deprecated();

    bool WriteZBTCUCount(const uint32_t& nCount);
    bool ReadZBTCUCount(uint32_t& nCount);
    std::map<uint256, std::vector<std::pair<uint256, uint32_t> > > MapMintPool();
    bool WriteMintPoolPair(const uint256& hashMasterSeed, const uint256& hashPubcoin, const uint32_t& nCount);

private:
    CWalletDB(const CWalletDB&);
    void operator=(const CWalletDB&);

    bool WriteAccountingEntry(const uint64_t nAccEntryNum, const CAccountingEntry& acentry);
};

void NotifyBacked(const CWallet& wallet, bool fSuccess, std::string strMessage);
bool BackupWallet(const CWallet& wallet, const boost::filesystem::path& strDest, bool fEnableCustom = true);
bool AttemptBackupWallet(const CWallet& wallet, const boost::filesystem::path& pathSrc, const boost::filesystem::path& pathDest);

/** Access to the wallet database.
 * This represents a single transaction at the
 * database. It will be committed when the object goes out of scope.
 * Optionally (on by default) it will flush to disk as well.
 */
class WalletBatch
{
private:
    template <typename K, typename T>
    bool WriteIC(const K& key, const T& value, bool fOverwrite = true)
    {
        if (!m_batch.Write(key, value, fOverwrite)) {
            return false;
        }
        m_database.IncrementUpdateCounter();
        return true;
    }

    template <typename K>
    bool EraseIC(const K& key)
    {
        if (!m_batch.Erase(key)) {
            return false;
        }
        m_database.IncrementUpdateCounter();
        return true;
    }

public:
    explicit WalletBatch(WalletDatabase& database, const char* pszMode = "r+", bool _fFlushOnClose = true) :
            m_batch(database, pszMode, _fFlushOnClose),
            m_database(database)
    {
    }
    WalletBatch(const WalletBatch&) = delete;
    WalletBatch& operator=(const WalletBatch&) = delete;

    bool WriteName(const std::string& strAddress, const std::string& strName);
    bool EraseName(const std::string& strAddress);

    bool WritePurpose(const std::string& strAddress, const std::string& purpose);
    bool ErasePurpose(const std::string& strAddress);

    bool WriteTx(const CWalletTx& wtx);
    bool EraseTx(uint256 hash);

    bool WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CKeyMetadata& keyMeta);
    bool WriteCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, const CKeyMetadata& keyMeta);
    bool WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey);

    bool WriteCScript(const uint160& hash, const CScript& redeemScript);

    bool WriteWatchOnly(const CScript& script);
    bool EraseWatchOnly(const CScript& script);

    bool WriteBestBlock(const CBlockLocator& locator);
    bool ReadBestBlock(CBlockLocator& locator);

    bool WriteOrderPosNext(int64_t nOrderPosNext);

    bool WriteStakeSplitThreshold(const CAmount& nStakeSplitThreshold);
    bool WriteUseCustomFee(bool fUse);
    bool WriteCustomFeeValue(const CAmount& nCustomFee);
    bool WriteAutoCombineSettings(bool fEnable, CAmount nCombineThreshold);

    bool ReadPool(int64_t nPool, CKeyPool& keypool);
    bool WritePool(int64_t nPool, const CKeyPool& keypool);
    bool ErasePool(int64_t nPool);

    bool WriteMinVersion(int nVersion);

    //! write the hdchain model (external/internal chain child index counter)
    bool WriteHDChain(const CHDChain& chain);

    /// Write extended spending key to wallet database, where the key is the incoming viewing key
    bool WriteSaplingZKey(const libzcash::SaplingIncomingViewingKey &ivk,
                          const libzcash::SaplingExtendedSpendingKey &key,
                          const CKeyMetadata  &keyMeta);

    bool WriteSaplingPaymentAddress(const libzcash::SaplingPaymentAddress &addr,
                                    const libzcash::SaplingIncomingViewingKey &ivk);

    bool WriteCryptedSaplingZKey(const libzcash::SaplingExtendedFullViewingKey &extfvk,
                                 const std::vector<unsigned char>& vchCryptedSecret,
                                 const CKeyMetadata &keyMeta);

    /// Common output viewing key, used when shielding transparent funds
    bool WriteSaplingCommonOVK(const uint256& ovk);
    bool ReadSaplingCommonOVK(uint256& ovkRet);

    bool WriteWitnessCacheSize(int64_t nWitnessCacheSize);

    /// Write destination data key,value tuple to database
    bool WriteDestData(const std::string& address, const std::string& key, const std::string& value);
    /// Erase destination data tuple from wallet database
    bool EraseDestData(const std::string& address, const std::string& key);

    DBErrors ReorderTransactions(CWallet* pwallet);
    DBErrors LoadWallet(CWallet* pwallet);
    DBErrors FindWalletTx(CWallet* pwallet, std::vector<uint256>& vTxHash, std::vector<CWalletTx>& vWtx);
    DBErrors ZapWalletTx(CWallet* pwallet, std::vector<CWalletTx>& vWtx);
    /* Try to (very carefully!) recover wallet database (with a possible key type filter) */
    static bool Recover(const fs::path& wallet_path, void *callbackDataIn, bool (*recoverKVcallback)(void* callbackData, CDataStream ssKey, CDataStream ssValue), std::string& out_backup_filename);
    /* Recover convenience-function to bypass the key filter callback, called when verify fails, recovers everything */
    static bool Recover(const fs::path& wallet_path, std::string& out_backup_filename);
    /* Recover filter (used as callback), will only let keys (cryptographical keys) as KV/key-type pass through */
    static bool RecoverKeysOnlyFilter(void *callbackData, CDataStream ssKey, CDataStream ssValue);
    /* Function to determin if a certain KV/key-type is a key (cryptographical key) type */
    static bool IsKeyType(const std::string& strType);
    /* verifies the database environment */
    static bool VerifyEnvironment(const fs::path& wallet_path, std::string& errorStr);
    /* verifies the database file */
    static bool VerifyDatabaseFile(const fs::path& wallet_path, std::string& warningStr, std::string& errorStr);

    //! Begin a new transaction
    bool TxnBegin();
    //! Commit current transaction
    bool TxnCommit();
    //! Abort current transaction
    bool TxnAbort();
    //! Read wallet version
    bool ReadVersion(int& nVersion);
    //! Write wallet version
    bool WriteVersion(int nVersion);
private:
    BerkeleyBatch m_batch;
    WalletDatabase& m_database;
};


#endif // BITCOIN_WALLETDB_H
