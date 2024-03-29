// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2017-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <streams.h>
#include "script/standard.h"

#include "pubkey.h"
#include "script/script.h"
#include "util.h"
#include "utilstrencodings.h"

//////////qtum
#include <qtum/qtumtransaction.h>
#include <qtum/qtumDGP.h>

typedef std::vector<unsigned char> valtype;

unsigned nMaxDatacarrierBytes = MAX_OP_RETURN_RELAY;

CScriptID::CScriptID(const CScript& in) : BaseHash(Hash160(in)) {}
CScriptID::CScriptID(const ScriptHash& in) : BaseHash(static_cast<uint160>(in)) {}

ScriptHash::ScriptHash(const CScript& in) : BaseHash(Hash160(in)) {}
ScriptHash::ScriptHash(const CScriptID& in) : BaseHash(static_cast<uint160>(in)) {}

PKHash::PKHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
PKHash::PKHash(const CKeyID& pubkey_id) : BaseHash(pubkey_id) {}

WitnessV0KeyHash::WitnessV0KeyHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
WitnessV0KeyHash::WitnessV0KeyHash(const PKHash& pubkey_hash) : BaseHash(static_cast<uint160>(pubkey_hash)) {}

CKeyID ToKeyID(const PKHash& key_hash)
{
   return CKeyID{static_cast<uint160>(key_hash)};
}

CKeyID ToKeyID(const WitnessV0KeyHash& key_hash)
{
   return CKeyID{static_cast<uint160>(key_hash)};
}

WitnessV0ScriptHash::WitnessV0ScriptHash(const CScript& in)
{
   CSHA256().Write(in.data(), in.size()).Finalize(begin());
}

const char* GetTxnOutputType(txnouttype t)
{
    switch (t)
    {
    case TX_NONSTANDARD: return "nonstandard";
    case TX_PUBKEY: return "pubkey";
    case TX_PUBKEYHASH: return "pubkeyhash";
    case TX_SCRIPTHASH: return "scripthash";
    case TX_MULTISIG: return "multisig";
    case TX_COLDSTAKE: return "coldstake";
    case TX_LEASE_CLTV:
    case TX_LEASE: return "lease";
    case TX_LEASINGREWARD: return "leasingreward";
    case TX_NULL_DATA: return "nulldata";
    case TX_ZEROCOINMINT: return "zerocoinmint";
    case TX_CREATE_SENDER: return "create_sender";
    case TX_CALL_SENDER: return "call_sender";
    case TX_CREATE: return "create";
    case TX_CALL: return "call";
    case TX_WITNESS_V0_KEYHASH: return "witness_v0_keyhash";
    case TX_WITNESS_V0_SCRIPTHASH: return "witness_v0_scripthash";
    case TX_WITNESS_V1_TAPROOT: return "witness_v1_taproot";
    case TX_WITNESS_UNKNOWN: return "witness_unknown";
    }
    return NULL;
}

/**
 * Return public keys or hashes from scriptPubKey, for 'standard' transaction types.
 */
bool Solver(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<std::vector<unsigned char> >& vSolutionsRet, bool contractConsensus, bool allowEmptySenderSig)
{
    //contractConsesus is true when evaluating if a contract tx is "standard" for consensus purposes
    //It is false in all other cases, so to prevent a particular contract tx from being broadcast on mempool, but allowed in blocks,
    //one should ensure that contractConsensus is false

    // Templates
    static std::multimap<txnouttype, CScript> mTemplates;
    if (mTemplates.empty())
    {
        // Standard tx, sender provides pubkey, receiver adds signature
        mTemplates.insert(std::make_pair(TX_PUBKEY, CScript() << OP_PUBKEY << OP_CHECKSIG));

        // Bitcoin address tx, sender provides hash of pubkey, receiver provides signature and pubkey
        mTemplates.insert(std::make_pair(TX_PUBKEYHASH, CScript() << OP_DUP << OP_HASH160 << OP_PUBKEYHASH << OP_EQUALVERIFY << OP_CHECKSIG));

        // Sender provides N pubkeys, receivers provides M signatures
        mTemplates.insert(std::make_pair(TX_MULTISIG, CScript() << OP_SMALLINTEGER << OP_PUBKEYS << OP_SMALLINTEGER << OP_CHECKMULTISIG));

        // Cold Staking: sender provides P2CS scripts, receiver provides signature, staking-flag and pubkey
        mTemplates.insert(std::make_pair(TX_COLDSTAKE, CScript() << OP_DUP << OP_HASH160 << OP_ROT << OP_IF << OP_CHECKCOLDSTAKEVERIFY <<
                OP_PUBKEYHASH << OP_ELSE << OP_PUBKEYHASH << OP_ENDIF << OP_EQUALVERIFY << OP_CHECKSIG));

        // Leasing: sender provides P2L scripts, receiver provides signature, leasing-flag and pubkey
        mTemplates.insert(std::make_pair(TX_LEASE, CScript() << OP_DUP << OP_HASH160 << OP_ROT << OP_IF << OP_CHECKLEASEVERIFY <<
                OP_PUBKEYHASH << OP_ELSE << OP_PUBKEYHASH << OP_ENDIF << OP_EQUALVERIFY << OP_CHECKSIG));

       // Leasing with locktime: sender provides P2L scripts, receiver provides signature, leasing-flag and pubkey
       mTemplates.insert(std::make_pair(TX_LEASE_CLTV, CScript() << OP_INTEGER << OP_CHECKLEASELOCKTIMEVERIFY << OP_DROP <<
       OP_DUP << OP_HASH160 << OP_ROT << OP_IF << OP_CHECKLEASEVERIFY << OP_PUBKEYHASH <<
                OP_ELSE << OP_PUBKEYHASH << OP_ENDIF << OP_EQUALVERIFY << OP_CHECKSIG));


        // Leasing reward: sender provides TRXHASH N and receiver pubkey
        mTemplates.insert(std::make_pair(TX_LEASINGREWARD, CScript() << OP_TRXHASH << OP_INTEGER << OP_LEASINGREWARD <<
                OP_DUP << OP_HASH160 << OP_PUBKEYHASH << OP_EQUALVERIFY << OP_CHECKSIG));

       // Contract creation tx with sender
       mTemplates.insert(std::make_pair(TX_CREATE_SENDER, CScript() << OP_ADDRESS_TYPE << OP_ADDRESS << OP_SCRIPT_SIG << OP_SENDER << OP_VERSION << OP_GAS_LIMIT << OP_GAS_PRICE << OP_DATA << OP_CREATE));

       // Call contract tx with sender
       mTemplates.insert(std::make_pair(TX_CALL_SENDER, CScript() << OP_ADDRESS_TYPE << OP_ADDRESS << OP_SCRIPT_SIG << OP_SENDER << OP_VERSION << OP_GAS_LIMIT << OP_GAS_PRICE << OP_DATA << OP_PUBKEYHASH << OP_CALL));

       // Contract creation tx
       mTemplates.insert(std::make_pair(TX_CREATE, CScript() << OP_VERSION << OP_GAS_LIMIT << OP_GAS_PRICE << OP_DATA << OP_CREATE));

       // Call contract tx
       mTemplates.insert(std::make_pair(TX_CALL, CScript() << OP_VERSION << OP_GAS_LIMIT << OP_GAS_PRICE << OP_DATA << OP_PUBKEYHASH << OP_CALL));
    }

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash())
    {
        typeRet = TX_SCRIPTHASH;
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        vSolutionsRet.push_back(hashBytes);
        return true;
    }

    // Zerocoin
    if (scriptPubKey.IsZerocoinMint()) {
        typeRet = TX_ZEROCOINMINT;
        if(scriptPubKey.size() > 150) return false;
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.end());
        vSolutionsRet.push_back(hashBytes);
        return true;
    }

   int witnessversion;
   std::vector<unsigned char> witnessprogram;
   if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
      if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_KEYHASH_SIZE) {
         vSolutionsRet.push_back(std::move(witnessprogram));
         typeRet = TX_WITNESS_V0_KEYHASH;
         return true;
      }
      if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
         vSolutionsRet.push_back(std::move(witnessprogram));
         typeRet = TX_WITNESS_V0_SCRIPTHASH;
         return true;
      }
      if (witnessversion == 1 && witnessprogram.size() == WITNESS_V1_TAPROOT_SIZE) {
         vSolutionsRet.push_back(std::move(witnessprogram));
         typeRet = TX_WITNESS_V1_TAPROOT;
         return true;
      }
      if (witnessversion != 0) {
         vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
         vSolutionsRet.push_back(std::move(witnessprogram));
         typeRet = TX_WITNESS_UNKNOWN;
         return true;
      }
   }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN && scriptPubKey.IsPushOnly(scriptPubKey.begin()+1)) {
        typeRet = TX_NULL_DATA;
        return true;
    }

    // Scan templates
    const CScript& script1 = scriptPubKey;
    for (const PAIRTYPE(txnouttype, CScript)& tplate : mTemplates)
    {
        const CScript& script2 = tplate.second;
        vSolutionsRet.clear();

        opcodetype opcode1, opcode2;
        std::vector<unsigned char> vch1, vch2;

        uint64_t addressType = 0;

        VersionVM version;
        version.rootVM=20; //set to some invalid value

        // Compare
        CScript::const_iterator pc1 = script1.begin();
        CScript::const_iterator pc2 = script2.begin();
        while (true)
        {
            if (pc1 == script1.end() && pc2 == script2.end())
            {
                // Found a match
                typeRet = tplate.first;
                if (typeRet == TX_MULTISIG)
                {
                    // Additional checks for TX_MULTISIG:
                    unsigned char m = vSolutionsRet.front()[0];
                    unsigned char n = vSolutionsRet.back()[0];
                    if (m < 1 || n < 1 || m > n || vSolutionsRet.size()-2 != n)
                        return false;
                }
                return true;
            }
            if (!script1.GetOp(pc1, opcode1, vch1))
                break;
            if (!script2.GetOp(pc2, opcode2, vch2))
                break;

            // Template matching opcodes:
            if (opcode2 == OP_PUBKEYS)
            {
                while (vch1.size() >= 33 && vch1.size() <= 65)
                {
                    vSolutionsRet.push_back(vch1);
                    if (!script1.GetOp(pc1, opcode1, vch1))
                        break;
                }
                if (!script2.GetOp(pc2, opcode2, vch2))
                    break;
                // Normal situation is to fall through
                // to other if/else statements
            }

            if (opcode2 == OP_PUBKEY)
            {
                if (vch1.size() < 33 || vch1.size() > 65)
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_PUBKEYHASH)
            {
                if (vch1.size() != sizeof(uint160))
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_TRXHASH)
            {
                if (vch1.size() != sizeof(uint256))
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_INTEGER)
            {
                if (opcode1 == OP_0 ||
                    (opcode1 >= OP_1 && opcode1 <= OP_16))
                {
                    char n = (char)CScript::DecodeOP_N(opcode1);
                    vSolutionsRet.push_back(CScriptNum(n).getvch());
                }
                else if (vch1.size() > 1 && vch1.size() < 6)
                {
                    vSolutionsRet.push_back(vch1);
                }
                else
                    break;
            }
            else if (opcode2 == OP_SMALLINTEGER)
            {   // Single-byte small integer pushed onto vSolutions
                if (opcode1 == OP_0 ||
                    (opcode1 >= OP_1 && opcode1 <= OP_16))
                {
                    char n = (char)CScript::DecodeOP_N(opcode1);
                    vSolutionsRet.push_back(valtype(1, n));
                }
                else
                    break;
            }

            /////////////////////////////////////////////////////////// qtum
            else if (opcode2 == OP_VERSION)
            {
               if(0 <= opcode1 && opcode1 <= OP_PUSHDATA4)
               {
                  if(vch1.empty() || vch1.size() > 4 || (vch1.back() & 0x80))
                     break;

                  version = VersionVM::fromRaw(CScriptNum::vch_to_uint64(vch1));
                  if(!(version.toRaw() == VersionVM::GetEVMDefault().toRaw() || version.toRaw() == VersionVM::GetNoExec().toRaw())){
                     // only allow standard EVM and no-exec transactions to live in mempool
                     break;
                  }
               }
            }
            else if(opcode2 == OP_GAS_LIMIT) {
               try {
                  if (vch1.size() > 8) break;
                  if (!vch1.empty() && vch1.back() & 0x80) break;

                  uint64_t val = CScriptNum::vch_to_uint64(vch1);
                  if(contractConsensus) {
                     //consensus rules (this is checked more in depth later using DGP)
                     if (version.rootVM != 0 && val < 1) {
                        break;
                     }
                     if (val > MAX_BLOCK_GAS_LIMIT_DGP) {
                        //do not allow transactions that could use more gas than is in a block
                        break;
                     }
                  }else{
                     //standard mempool rules for contracts
                     //consensus rules for contracts
                     if (version.rootVM != 0 && val < STANDARD_MINIMUM_GAS_LIMIT) {
                        break;
                     }
                     if (val > DEFAULT_BLOCK_GAS_LIMIT_DGP / 2) {
                        //don't allow transactions that use more than 1/2 block of gas to be broadcast on the mempool
                        break;
                     }

                  }
               }
               catch (const scriptnum_error &err) {
                  break;
               }
            }
            else if(opcode2 == OP_GAS_PRICE) {
               try {
                  if (vch1.size() > 8) break;
                  if (!vch1.empty() && vch1.back() & 0x80) break;

                  uint64_t val = CScriptNum::vch_to_uint64(vch1);
                  if(contractConsensus) {
                     //consensus rules (this is checked more in depth later using DGP)
                     if (version.rootVM != 0 && val < 1) {
                        break;
                     }
                  }else{
                     //standard mempool rules
                     if (version.rootVM != 0 && val < STANDARD_MINIMUM_GAS_PRICE) {
                        break;
                     }
                  }
               }
               catch (const scriptnum_error &err) {
                  break;
               }
            }
            else if(opcode2 == OP_DATA)
            {
               if(0 <= opcode1 && opcode1 <= OP_PUSHDATA4)
               {
                  if(vch1.empty())
                     break;
               }
            }
            else if(opcode2 == OP_ADDRESS_TYPE)
            {
               try {
                  if (vch1.size() > 8) break;
                  if (!vch1.empty() && vch1.back() & 0x80) break;

                  uint64_t val = CScriptNum::vch_to_uint64(vch1);
                  if(val < addresstype::PUBKEYHASH || val > addresstype::NONSTANDARD)
                     break;

                  addressType = val;
               }
               catch (const scriptnum_error &err) {
                  break;
               }
            }
            else if(opcode2 == OP_ADDRESS)
            {
               // Get the destination
               CTxDestination dest;
               if(addressType == addresstype::PUBKEYHASH && vch1.size() == sizeof(CKeyID))
               {
                  dest = PKHash(uint160(vch1));
               }
               else
                  break;

               // Get the public key for the destination
               CScript senderPubKey = GetScriptForDestination(dest);
               CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
               ss << senderPubKey;
               vSolutionsRet.push_back(std::vector<unsigned char>(ss.begin(), ss.end()));
            }
            else if(opcode2 == OP_SCRIPT_SIG)
            {
               if(0 <= opcode1 && opcode1 <= OP_PUSHDATA4)
               {
                  if(!allowEmptySenderSig && vch1.empty())
                     break;

                  // Check the max size of the signature script
                  if(vch1.size() > MAX_BASE_SCRIPT_SIZE)
                     break;

                  vSolutionsRet.push_back(vch1);
               }
            }
            ///////////////////////////////////////////////////////////

            else if (opcode1 != opcode2 || vch1 != vch2)
            {
                // Others must match exactly
                break;
            }
        }
    }

    vSolutionsRet.clear();
    typeRet = TX_NONSTANDARD;
    return false;
}

int ScriptSigArgsExpected(txnouttype t, const std::vector<std::vector<unsigned char> >& vSolutions)
{
    switch (t)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
    case TX_ZEROCOINMINT:
        return -1;
    case TX_PUBKEY:
        return 1;
    case TX_PUBKEYHASH:
        return 2;
    case TX_COLDSTAKE:
        return 3;
    case TX_LEASE_CLTV:
    case TX_LEASE:
        return 3;
    case TX_LEASINGREWARD:
        return 2;
    case TX_MULTISIG:
        if (vSolutions.size() < 1 || vSolutions[0].size() < 1)
            return -1;
        return vSolutions[0][0] + 1;
    case TX_SCRIPTHASH:
        return 1; // doesn't include args needed by the script
    case TX_WITNESS_V0_KEYHASH:
    case TX_WITNESS_V0_SCRIPTHASH:
        return 1;
    case TX_WITNESS_UNKNOWN:
        return 2;
    }
    return -1;
}

bool IsStandard(const CScript& scriptPubKey, txnouttype& whichType)
{
    std::vector<valtype> vSolutions;
    if (!Solver(scriptPubKey, whichType, vSolutions))
        return false;

    if (whichType == TX_MULTISIG)
    {
        unsigned char m = vSolutions.front()[0];
        unsigned char n = vSolutions.back()[0];
        // Support up to x-of-3 multisig txns as standard
        if (n < 1 || n > 3)
            return false;
        if (m < 1 || m > n)
            return false;
    } else if (whichType == TX_NULL_DATA &&
                (!GetBoolArg("-datacarrier", true) || scriptPubKey.size() > nMaxDatacarrierBytes))
        return false;

    return whichType != TX_NONSTANDARD;
}
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet, txnouttype *typeRet)
{
    std::vector<valtype> vSolutions;
    txnouttype whichType;
    Solver(scriptPubKey,whichType, vSolutions);

    if(typeRet){
        *typeRet = whichType;
    }

    if (whichType == TX_PUBKEY) {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;

        addressRet = PKHash(pubKey.GetID());
        return true;
    }
    else if (whichType == TX_PUBKEYHASH)
    {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    }
    else if (whichType == TX_SCRIPTHASH)
    {
        addressRet = ScriptHash(uint160(vSolutions[0]));
        return true;
    } else if (whichType == TX_WITNESS_V0_KEYHASH) {
        WitnessV0KeyHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    } else if (whichType == TX_WITNESS_V0_SCRIPTHASH) {
        WitnessV0ScriptHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    } else if (whichType == TX_WITNESS_UNKNOWN) {
        WitnessUnknown unk;
        unk.version = vSolutions[0][0];
        std::copy(vSolutions[1].begin(), vSolutions[1].end(), unk.program);
        unk.length = vSolutions[1].size();
        addressRet = unk;
        return true;
    }
    // Multisig txns have more than one address...
    return false;
}
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet, bool fColdStake, bool fLease)
{
    std::vector<valtype> vSolutions;
    txnouttype whichType;
    if (!Solver(scriptPubKey, whichType, vSolutions))
        return false;

    if (whichType == TX_PUBKEY) {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;

        addressRet = PKHash(pubKey.GetID());
        return true;

    } else if (whichType == TX_PUBKEYHASH) {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;

    } else if (whichType == TX_SCRIPTHASH) {
        addressRet = ScriptHash(uint160(vSolutions[0]));
        return true;
    } else if (whichType == TX_COLDSTAKE) {
        addressRet = PKHash(uint160(vSolutions[!fColdStake]));
        return true;
    }else if (whichType == TX_WITNESS_V0_KEYHASH) {
       WitnessV0KeyHash hash;
       std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
       addressRet = hash;
       return true;
    } else if (whichType == TX_WITNESS_V0_SCRIPTHASH) {
       WitnessV0ScriptHash hash;
       std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
       addressRet = hash;
       return true;
    } else if (whichType == TX_WITNESS_UNKNOWN) {
       WitnessUnknown unk;
       unk.version = vSolutions[0][0];
       std::copy(vSolutions[1].begin(), vSolutions[1].end(), unk.program);
       unk.length = vSolutions[1].size();
       addressRet = unk;
       return true;
    } else if (whichType == TX_LEASE ) {
        addressRet = PKHash(uint160(vSolutions[!fLease]));
        return true;
    }else if (whichType == TX_LEASE_CLTV) {
       addressRet = PKHash(uint160(vSolutions[!fLease + 1]));
       return true;
    } else if (whichType == TX_LEASINGREWARD) {
        // 0 is TRXHASH
        // 1 is N
        addressRet = PKHash(uint160(vSolutions[2]));
    } else if (whichType == TX_WITNESS_V1_TAPROOT) {
       WitnessV1Taproot tap;
       std::copy(vSolutions[0].begin(), vSolutions[0].end(), tap.begin());
       addressRet = tap;
       return true;
    }
    // Multisig txns have more than one address...
    return false;
}

bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet)
{
    addressRet.clear();
    typeRet = TX_NONSTANDARD;
    std::vector<valtype> vSolutions;
    if (!Solver(scriptPubKey, typeRet, vSolutions))
        return false;
    if (typeRet == TX_NULL_DATA){
        // This is data, not addresses
        return false;
    }

    if (typeRet == TX_MULTISIG)
    {
        nRequiredRet = vSolutions.front()[0];
        for (unsigned int i = 1; i < vSolutions.size()-1; i++)
        {
            CPubKey pubKey(vSolutions[i]);
            if (!pubKey.IsValid())
                continue;

            CTxDestination address = PKHash(pubKey.GetID());
            addressRet.push_back(address);
        }

        if (addressRet.empty())
            return false;

    } else if (typeRet == TX_COLDSTAKE)
    {
        if (vSolutions.size() < 2)
            return false;
        nRequiredRet = 2;
        addressRet.push_back(PKHash(uint160(vSolutions[0])));
        addressRet.push_back(PKHash(uint160(vSolutions[1])));
        return true;
    } else if (typeRet == TX_LEASE)
    {
        if (vSolutions.size() < 2)
            return false;
        nRequiredRet = 2;
        addressRet.push_back(PKHash(uint160(vSolutions[0])));
        addressRet.push_back(PKHash(uint160(vSolutions[1])));
        return true;
    }else if (typeRet == TX_LEASE_CLTV)
    {
       if (vSolutions.size() < 3)
          return false;
       nRequiredRet = 2;
       addressRet.push_back(PKHash(uint160(vSolutions[1])));
       addressRet.push_back(PKHash(uint160(vSolutions[2])));
       return true;
    } else if (typeRet == TX_LEASINGREWARD)
    {
        if (vSolutions.size() < 3)
            return false;
        nRequiredRet = 1;
        // 0. TRXHASH
        // 1. N
        addressRet.push_back(PKHash(uint160(vSolutions[2])));
        return true;
    } else if (typeRet == TX_WITNESS_V1_TAPROOT) {
       WitnessV1Taproot tap;
       std::copy(vSolutions[0].begin(), vSolutions[0].end(), tap.begin());
       addressRet.push_back(std::move(tap));
       return true;
    } else
    {
        nRequiredRet = 1;
        CTxDestination address;
        if (!ExtractDestination(scriptPubKey, address))
           return false;
        addressRet.push_back(address);
    }

    return true;
}

namespace
{
class CScriptVisitor
{
private:
    CScript *script;
public:
    CScriptVisitor(CScript *scriptin) { script = scriptin; }

    bool operator()(const CNoDestination &dest) const {
        return false;
    }

    bool operator()(const PKHash &keyID) const {
        *script << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
        return true;
    }

    bool operator()(const ScriptHash& scriptID) const {
        *script << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
        return true;
    }

    bool operator()(const WitnessV0KeyHash& id) const
    {
      script->clear();
      *script << OP_0 << ToByteVector(id);
      return true;
    }

    bool operator()(const WitnessV0ScriptHash& id) const
    {
      script->clear();
      *script << OP_0 << ToByteVector(id);
      return true;
    }

    bool operator()(const WitnessV1Taproot& tap) const
    {
       *script << OP_1 << ToByteVector(tap);
       return true;
    }

    bool operator()(const WitnessUnknown& id) const
    {
      script->clear();
      *script << CScript::EncodeOP_N(id.version) << std::vector<unsigned char>(id.program, id.program + id.length);
      return true;
    }
};
}

CScript GetScriptForDestination(const CTxDestination& dest)
{
    CScript script;

    std::visit(CScriptVisitor(&script), dest);
    return script;
}

CScript GetScriptForStakeDelegation(const CKeyID& stakingKey, const CKeyID& spendingKey)
{
    CScript script;
    script << OP_DUP << OP_HASH160 << OP_ROT <<
            OP_IF << OP_CHECKCOLDSTAKEVERIFY << ToByteVector(stakingKey) <<
            OP_ELSE << ToByteVector(spendingKey) << OP_ENDIF <<
            OP_EQUALVERIFY << OP_CHECKSIG;
    return script;
}

CScript GetScriptForRawPubKey(const CPubKey& pubKey)
{
   return CScript() << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;
}

CScript GetScriptForLeasing(const CKeyID& leaserKey, const CKeyID& ownerKey)
{
    CScript script;
    script << OP_DUP << OP_HASH160 << OP_ROT <<
           OP_IF << OP_CHECKLEASEVERIFY << ToByteVector(leaserKey) <<
           OP_ELSE << ToByteVector(ownerKey) << OP_ENDIF <<
           OP_EQUALVERIFY << OP_CHECKSIG;
    return script;
}
CScript GetScriptForLeasingCLTV(const CKeyID& leaserKey, const CKeyID& ownerKey, uint32_t nLockTime)
{
   CScript script;
   script << nLockTime << OP_CHECKLEASELOCKTIMEVERIFY << OP_DROP <<
          OP_DUP << OP_HASH160 << OP_ROT <<
          OP_IF << OP_CHECKLEASEVERIFY << ToByteVector(leaserKey) <<
          OP_ELSE << ToByteVector(ownerKey) << OP_ENDIF <<
          OP_EQUALVERIFY << OP_CHECKSIG;
   return script;
}

CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys)
{
    CScript script;

    script << CScript::EncodeOP_N(nRequired);
    for (const CPubKey& key : keys)
        script << ToByteVector(key);
    script << CScript::EncodeOP_N(keys.size()) << OP_CHECKMULTISIG;
    return script;
}

bool ExtractSenderData(const CScript &outputPubKey, CScript *senderPubKey, CScript *senderSig)
{
   if(outputPubKey.HasOpSender())
   {
      try
      {
         // Solve the contract with or without contract consensus
         /***** Original from qtum:
                     std::vector<valtype> vSolutions;
            if (TX_NONSTANDARD == Solver(outputPubKey, vSolutions, true) &&
                    TX_NONSTANDARD == Solver(outputPubKey, vSolutions, false))
                return false;*/

         std::vector<valtype> vSolutions;
         txnouttype whichType1, whichType2;
         Solver(outputPubKey, whichType1, vSolutions, true);
         Solver(outputPubKey, whichType2, vSolutions, false);

         if(whichType1 == TX_NONSTANDARD && whichType1 == TX_NONSTANDARD)
            return false;

         // Check the size of the returned data
         if(vSolutions.size() < 2)
            return false;

         // Get the sender public key
         if(senderPubKey)
         {
            CDataStream ss(vSolutions[0], SER_NETWORK, PROTOCOL_VERSION);
            ss >> *senderPubKey;
         }

         // Get the sender signature
         if(senderSig)
         {
            CDataStream ss(vSolutions[1], SER_NETWORK, PROTOCOL_VERSION);
            ss >> *senderSig;
         }
      }
      catch(...)
      {
         return false;
      }

      return true;
   }
   return false;
}

bool GetSenderPubKey(const CScript &outputPubKey, CScript &senderPubKey)
{
   if(outputPubKey.HasOpSender())
   {
      try
      {
         // Solve the contract with or without contract consensus
         std::vector<valtype> vSolutions;
         txnouttype whichType1, whichType2;
         Solver(outputPubKey, whichType1, vSolutions, true);
         Solver(outputPubKey, whichType2, vSolutions, false);

         if(whichType1 == TX_NONSTANDARD && whichType1 == TX_NONSTANDARD)
            return false;

         // Check the size of the returned data
         if(vSolutions.size() < 1)
            return false;

         // Get the sender public key
         CDataStream ss(vSolutions[0], SER_NETWORK, PROTOCOL_VERSION);
         ss >> senderPubKey;
      }
      catch(...)
      {
         return false;
      }

      return true;
   }
   return false;
}

bool IsValidDestination(const CTxDestination& dest) {
   return dest.index() != 0;
}

bool IsValidContractSenderAddress(const CTxDestination &dest)
{
   return std::holds_alternative<PKHash>(dest);
}
CScript GetScriptForWitness(const CScript& redeemscript)
{
   std::vector<std::vector<unsigned char> > vSolutions;
   txnouttype typ;
   Solver(redeemscript, typ, vSolutions);
   if (typ ==  TX_PUBKEY) {
      return GetScriptForDestination(WitnessV0KeyHash(Hash160(vSolutions[0].begin(), vSolutions[0].end())));
   } else if (typ == TX_PUBKEYHASH) {
      return GetScriptForDestination(WitnessV0KeyHash(uint160{vSolutions[0]}));
   }
   return GetScriptForDestination(WitnessV0ScriptHash(redeemscript));
}

CScript GetScriptForLeasingReward(const COutPoint& outPoint, const CTxDestination& dest)
{
    CScript script;

    script << ToByteVector(outPoint.hash) << outPoint.n << OP_LEASINGREWARD;
    std::visit(CScriptVisitor(&script), dest);
    return script;
}

void TaprootSpendData::Merge(TaprootSpendData other)
{
   // TODO: figure out how to better deal with conflicting information
   // being merged.
   if (internal_key.IsNull() && !other.internal_key.IsNull()) {
      internal_key = other.internal_key;
   }
   if (merkle_root.IsNull() && !other.merkle_root.IsNull()) {
      merkle_root = other.merkle_root;
   }
   for (auto& [key, control_blocks] : other.scripts) {
      // Once P0083R3 is supported by all our targeted platforms,
      // this loop body can be replaced with:
      // scripts[key].merge(std::move(control_blocks));
      auto& target = scripts[key];
      for (auto& control_block: control_blocks) {
         target.insert(std::move(control_block));
      }
   }
}
