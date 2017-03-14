// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QUANTUM_UNDO_H
#define QUANTUM_UNDO_H

#include "compressor.h" 
#include "primitives/transaction.h"
#include "serialize.h"

/** Undo information for a CTxIn
 *
 *  Contains the prevout's CTxOut being spent, and if this was the
 *  last output of the affected transaction, its metadata as well
 *  (coinbase or not, height, transaction version)
 */
class CTxInUndo
{
public:
    CTxOut txout;         // the txout data before being spent
    bool fCoinBase;       // if the outpoint was the last unspent: whether it belonged to a coinbase
    //begin modif qtum
    bool fCoinStake;      // if the outpoint was the last unspent: whether it belonged to a coinstake
    //end modif qtum
    unsigned int nHeight; // if the outpoint was the last unspent: its height
    int nVersion;         // if the outpoint was the last unspent: its version

    //begin modif qtum
    CTxInUndo() : txout(), fCoinBase(false), fCoinStake(false), nHeight(0), nVersion(0) {}
    CTxInUndo(const CTxOut &txoutIn, bool fCoinBaseIn = false, bool fCoinStakeIn = false, unsigned int nHeightIn = 0, int nVersionIn = 0) : txout(txoutIn), fCoinBase(fCoinBaseIn), fCoinStake(fCoinStakeIn), nHeight(nHeightIn), nVersion(nVersionIn) { }
    //end modif qtum

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        //begin modif qtum
        return ::GetSerializeSize(VARINT(nHeight*4+(fCoinBase ? 1 : 0)+(fCoinStake ? 2 : 0)), nType, nVersion) +
        //end modif qtum
               (nHeight > 0 ? ::GetSerializeSize(VARINT(this->nVersion), nType, nVersion) : 0) +
               ::GetSerializeSize(CTxOutCompressor(REF(txout)), nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        //begin modif qtum
        ::Serialize(s, VARINT(nHeight*4+(fCoinBase ? 1 : 0)+(fCoinStake ? 2 : 0)), nType, nVersion);
        //end modif qtum
        if (nHeight > 0)
            ::Serialize(s, VARINT(this->nVersion), nType, nVersion);
        ::Serialize(s, CTxOutCompressor(REF(txout)), nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        unsigned int nCode = 0;
        ::Unserialize(s, VARINT(nCode), nType, nVersion);
        //begin modif qtum
        nHeight = nCode / 4;
        //end modif qtum
        fCoinBase = nCode & 1;
        //begin modif qtum
        fCoinStake = nCode & 2;
        //end modif qtum
        if (nHeight > 0)
            ::Unserialize(s, VARINT(this->nVersion), nType, nVersion);
        ::Unserialize(s, REF(CTxOutCompressor(REF(txout))), nType, nVersion);
    }
};

/** Undo information for a CTransaction */
class CTxUndo
{
public:
    // undo information for all txins
    std::vector<CTxInUndo> vprevout;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vprevout);
    }
};

/** Undo information for a CBlock */
class CBlockUndo
{
public:
    std::vector<CTxUndo> vtxundo; // for all but the coinbase

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vtxundo);
    }
};

#endif // QUANTUM_UNDO_H
