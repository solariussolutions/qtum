// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QUANTUM_PRIMITIVES_BLOCK_H
#define QUANTUM_PRIMITIVES_BLOCK_H

#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"
//begin modif qtum
#include "arith_uint256.h"
//end modif qtum

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // header
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;
    //begin modif qtum
    // block signature - proof-of-stake protect the block by signing the block using a stake holder private key
    std::vector<unsigned char> vchBlockSig;
    /** Blackcoin use CBlockHeader and CBlock as one class called CBlock that contain header parameters
     * that are dependent on the coinstake transaction that was performed.
     * Blackcoin distinguish between the header and the block by the presence of transactions
     * (the transactions are not read for the header). The problem is that the CBlockHeader and CBlock
     * are widely used in Quantum (10, 36 files), so merging them will require a lot of checks.
     * So the solution that is implemented is updating CBlockHeader with new parameters that will be
     * updated from CBlock using virtual functions, the new parameters are intended to be used indirectly
     * by calling the virtual functions to read there content except in the constructions phase where will 
     * be used directly.*/
    bool fStake;
    // proof-of-stake specific fields
    COutPoint prevoutStake;
    uint32_t nStakeTime;
    //end modif qtum
    uint256 hashStateRoot;
    uint256 hashUTXORoot; // TODO temp rootQtum

    CBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
        //begin modif qtum
        fStake = IsProofOfStake();
        prevoutStake = PrevoutStake();
        nStakeTime = StakeTime();
        if(!(nType & SER_GETHASH))
        {
            READWRITE(vchBlockSig);
            READWRITE(fStake);
            READWRITE(prevoutStake);
            READWRITE(nStakeTime);
        }
        //end modif qtum
	    READWRITE(hashStateRoot);
        READWRITE(hashUTXORoot); // TODO temp rootQtum
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
	//begin modif qtum
        vchBlockSig.clear();
        fStake = 0;
        prevoutStake.SetNull();
        nStakeTime = 0;
        //end modif qtum
        hashStateRoot.SetNull();
        hashUTXORoot.SetNull(); // TODO temp rootQtum
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;

    //begin modif qtum
    uint256 GetPoWHash() const
    {
        return GetHash();
    }
    //end modif qtum

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
    
    //begin modif qtum
    // entropy bit for stake modifier if chosen by modifier
    unsigned int GetStakeEntropyBit() const
    {
        // Take last bit of block hash as entropy bit
        unsigned int nEntropyBit = ((UintToArith256(GetHash()).GetLow64()) & 1llu);
        return nEntropyBit;
    }
    
    // ppcoin: two types of block: proof-of-work or proof-of-stake
    virtual bool IsProofOfStake() const
    {
        return fStake;
    }

    virtual bool IsProofOfWork() const
    {
        return !IsProofOfStake();
    }
    
    virtual COutPoint PrevoutStake() const
    {
        return prevoutStake;
    }

    virtual uint32_t StakeTime() const
    {
        return nStakeTime;
    }

    CBlockHeader& operator=(const CBlockHeader& other)
    {
        if (this != &other)
        {
            this->nVersion       = other.nVersion;
            this->hashPrevBlock  = other.hashPrevBlock;
            this->hashMerkleRoot = other.hashMerkleRoot;
            this->nTime          = other.nTime;
            this->nBits          = other.nBits;
            this->nNonce         = other.nNonce;
            this->vchBlockSig    = other.vchBlockSig;
            this->fStake         = other.IsProofOfStake();
            this->prevoutStake   = other.PrevoutStake();
            this->nStakeTime     = other.StakeTime();
            this->hashStateRoot  = other.hashStateRoot;
            this->hashUTXORoot   = other.hashUTXORoot;
        }
        return *this;
    }
    //end modif qtum
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransaction> vtx;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

    //begin modif qtum
    // ppcoin: two types of block: proof-of-work or proof-of-stake
    bool IsProofOfStake() const
    {
        if(vtx.size() == 0) return fStake;

        return (vtx.size() > 1 && vtx[1].IsCoinStake());
    }

    COutPoint PrevoutStake() const
    {
        if(vtx.size() == 0) return prevoutStake;

        COutPoint ret;
        if(IsProofOfStake())
        {
            ret = vtx[1].vin[0].prevout;
        }
        return ret;
    }
    
    uint32_t StakeTime() const
    {
        if(vtx.size() == 0) return nStakeTime;

        uint32_t ret = 0;
        if(IsProofOfStake())
        {
            ret = vtx[1].nTime;;
        }
        return ret;
    }
    
    std::pair<COutPoint, unsigned int> GetProofOfStake() const
    {
        return IsProofOfStake()? std::make_pair(vtx[1].vin[0].prevout, vtx[1].nTime) : std::make_pair(COutPoint(), (unsigned int)0);
    }
    //end modif qtum
    
    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        //begin modif qtum
        block.vchBlockSig    = vchBlockSig;
        block.fStake         = IsProofOfStake();
        block.prevoutStake   = PrevoutStake();
        block.nStakeTime     = StakeTime();
        //end modif qtum
        block.hashStateRoot  = hashStateRoot;
        block.hashUTXORoot   = hashUTXORoot; // TODO temp rootQtum
        return block;
    }

    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    CBlockLocator(const std::vector<uint256>& vHaveIn)
    {
        vHave = vHaveIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

/** Compute the consensus-critical block weight (see BIP 141). */
int64_t GetBlockWeight(const CBlock& tx);

#endif // QUANTUM_PRIMITIVES_BLOCK_H
