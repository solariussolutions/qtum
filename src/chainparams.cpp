// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"
//begin modif qtum
#include "arith_uint256.h"
//end modif qtum
#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"
#include "evm/libdevcore/SHA3.h"
#include "evm/libdevcore/RLP.h"

//begin modif qtum
//Method that can make any block be the genesis block, useful for development to get the correct nonce.
bool MakeItGenesis(CBlock& block, uint256 powLimit)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(powLimit))
        return false;

    uint256 thash;

    while(true)
    {
        thash = block.GetHash();
        if (UintToArith256(thash) <= bnTarget)
            break;
        if ((block.nNonce & 0xFFF) == 0)
        {
            printf("nonce %08X: hash = %s (target = %s)\n", block.nNonce, thash.ToString().c_str(), bnTarget.ToString().c_str());
        }
        ++block.nNonce;
        if (block.nNonce == 0)
        {
            printf("NONCE WRAPPED, incrementing time\n");
            ++block.nTime;
        }
    }

    printf("block.nTime = %u \n", block.nTime);
    printf("block.nNonce = %u \n", block.nNonce);
    printf("block.GetHash = %s\n", block.GetHash().ToString().c_str());
    printf("block.hashMerkleRoot = %s\n", block.hashMerkleRoot.ToString().c_str());
    return true;
}
//end modif qtum

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;
    //begin modif qtum
    txNew.nTime = nTime;
    //end modif qtum

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(txNew);
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashStateRoot  = uint256(h256Touint(dev::sha3(dev::rlp(""))));
    genesis.hashUTXORoot   = uint256(h256Touint(dev::sha3(dev::rlp("")))); // TODO temp rootQtum
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = 227931;
        consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        //begin modif qtum
        // consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // consensus.posLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        
        consensus.nTargetTimespan = 16 * 60;  // 16 mins
        consensus.nTargetSpacing = 64;
        //end modif qtum
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = true;
        //begin modif qtum
        consensus.nRuleChangeActivationThreshold = 14; // 95% of 15
        consensus.nMinerConfirmationWindow = 15; // nTargetTimespan / nTargetSpacing
        //end modif qtum
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 0; // Never / undefined

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xbe;
        pchMessageStart[2] = 0xb4;
        pchMessageStart[3] = 0xd9;
        //begin modif qtum
        nDefaultPort = 8889;
        //end modif qtum
        nPruneAfterHeight = 100000;

        //begin modif qtum
        // genesis = CreateGenesisBlock(1477579050, 4093737, 0x1e0fffff, 1, 50 * COIN);
        // consensus.hashGenesisBlock = genesis.GetHash();
        
        genesis = CreateGenesisBlock(1467160981, 1575083623, 0x1f00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000070346fef5d28de8be6d2d3a2a0c446111d2a937c0d29c469864a6f0cc24a")); // TODO temp rootQtum        
        assert(genesis.hashMerkleRoot == uint256S("0x10f368ee4115f5809b10e129bd71cd238489c6da22d76bbe6acf7c22b2dee145"));

        // assert(consensus.hashGenesisBlock == uint256S("0x0000013cdf3e564558d584e51dc7631af30dabbea9ff7a85fef4c75e74deb71f"));
        // assert(genesis.hashMerkleRoot == uint256S("0x3511552c6a68b62c7ba8ac6fd24ba1ec1ad615e38aa6b254f198d517f87205fc"));

//        vSeeds.push_back(CDNSSeedData("quantum.sipa.be", "seed.quantum.sipa.be")); // Pieter Wuille
//        vSeeds.push_back(CDNSSeedData("bluematt.me", "dnsseed.bluematt.me")); // Matt Corallo
//        vSeeds.push_back(CDNSSeedData("dashjr.org", "dnsseed.quantum.dashjr.org")); // Luke Dashjr
//        vSeeds.push_back(CDNSSeedData("quantumstats.com", "seed.quantumstats.com")); // Christian Decker
//        vSeeds.push_back(CDNSSeedData("xf2.org", "bitseed.xf2.org")); // Jeff Garzik
//        vSeeds.push_back(CDNSSeedData("quantum.jonasschnelli.ch", "seed.quantum.jonasschnelli.ch")); // Jonas Schnelli
        //end modif qtum

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        //begin modif qtum
		checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("00000e5e9fef0577210c46b7139bbbbebbf4ed3df5670ed404595a3dc3fa7ff3")),
            0, // * UNIX timestamp of last checkpoint block
            0, // * total number of transactions between genesis and last checkpoint
               //   (the tx=... number in the SetBestChain debug.log lines)
            0  // * estimated number of transactions per day after checkpoint
        };
		
        nLastPOWBlock = 5000;
        //end modif qtum
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        consensus.BIP34Height = 21111;
        consensus.BIP34Hash = uint256S("0x0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        //begin modif qtum
        consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nTargetTimespan = 16 * 60;  // 16 mins
        consensus.nTargetSpacing = 64;
        //end modif qtum
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        //begin modif qtum
        consensus.nRuleChangeActivationThreshold = 11; // 75% for testchains
        consensus.nMinerConfirmationWindow = 15; // nTargetTimespan / nTargetSpacing
        //end modif qtum
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1456790400; // March 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1462060800; // May 1st 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1493596800; // May 1st 2017

        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x09;
        pchMessageStart[3] = 0x07;
        //begin modif qtum
        nDefaultPort = 18889;
        //end modif qtum
        nPruneAfterHeight = 1000;

        //begin modif qtum
        // genesis = CreateGenesisBlock(1477579050, 325026, 0x1f00ffff, 1, 50 * COIN);
        // consensus.hashGenesisBlock = genesis.GetHash();

        // assert(consensus.hashGenesisBlock == uint256S("0x0000caf03d3d4d80e998ee09fbb281407476cc096915327a520eba9240333d24"));
        // assert(genesis.hashMerkleRoot == uint256S("0x3511552c6a68b62c7ba8ac6fd24ba1ec1ad615e38aa6b254f198d517f87205fc"));
        //end modif qtum

        genesis = CreateGenesisBlock(1467160981, 107231206, 0x1f00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0000c8d2c661ccca5a74079ddcbf30ef580928ddd07f09dccf0c86eec7ef6b01")); // TODO temp rootQtum
        assert(genesis.hashMerkleRoot == uint256S("0x10f368ee4115f5809b10e129bd71cd238489c6da22d76bbe6acf7c22b2dee145"));

        vFixedSeeds.clear();
        vSeeds.clear();
        //begin modif qtum
//        vSeeds.push_back(CDNSSeedData("quantum.petertodd.org", "testnet-seed.quantum.petertodd.org"));
//        vSeeds.push_back(CDNSSeedData("bluematt.me", "testnet-seed.bluematt.me"));
//        vSeeds.push_back(CDNSSeedData("quantum.schildbach.de", "testnet-seed.quantum.schildbach.de"));
        //end modif qtum

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        //begin modif qtum
        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0, uint256S("0000803698155bf158957dc6435eeb83648d016cbfee8fa28cb399eedcf7b7a6")),
            0,
            0,
            0
        };
		
        nLastPOWBlock = 50;
        //end modif qtum
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = -1; // BIP34 has not necessarily activated on regtest
        consensus.BIP34Hash = uint256();
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        //begin modif qtum
        consensus.posLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nTargetTimespan = 16 * 60;  // 16 mins
        consensus.nTargetSpacing = 64;
        //end modif qtum
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        //begin modif qtum
        consensus.nRuleChangeActivationThreshold = 7; // 75% for testchains
        consensus.nMinerConfirmationWindow = 10; // Faster than normal for regtest (10 instead of 15)
        //end modif qtum
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        //begin modif qtum
        nDefaultPort = 22889;
        //end modif qtum
        nPruneAfterHeight = 1000;

        //begin modif qtum
        // genesis = CreateGenesisBlock(1477579050, 3, 0x207fffff, 1, 50 * COIN);
        //end modif qtum
        // consensus.hashGenesisBlock = genesis.GetHash();
        //begin modif qtum

        genesis = CreateGenesisBlock(1467160981, 13, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x2c3e4a38307ca8610f31cdd77fce3ccd432328f64af94a1662af2ac7491c2ade")); // TODO temp rootQtum
        assert(genesis.hashMerkleRoot == uint256S("0x10f368ee4115f5809b10e129bd71cd238489c6da22d76bbe6acf7c22b2dee145"));


        // assert(consensus.hashGenesisBlock == uint256S("0x637de26b9bb33cde7ebfbc8b278cd03b38c5c1a5c876b52a011863593885f086"));
        // assert(genesis.hashMerkleRoot == uint256S("0x3511552c6a68b62c7ba8ac6fd24ba1ec1ad615e38aa6b254f198d517f87205fc"));
        //end modif qtum

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;

        //begin modif qtum
        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("55f542c97770bb12c0d947bbdab8a0bd63799f58a7d923e515253ca10d1047ca")),
            0,
            0,
            0
        };

        nLastPOWBlock = 100;
        //end modif qtum
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

////         If genesis block hash does not match, then generate new genesis hash.
//                printf("Searching for genesis block...\n");
//                // This will figure out a valid hash and Nonce if you're
//                // creating a different genesis block:
//                arith_uint256 hashTarget = hashTarget.SetCompact(genesis.nBits);
//                arith_uint256 thash;
//
//                while(true)
//                {
//                    thash = UintToArith256(genesis.GetHash());
//                    if (thash <= hashTarget)
//                         break;
//                    if ((genesis.nNonce & 0xFFF) == 0)
//                    {
//                         printf("nonce %08X: hash = %s (target = %s)\n", genesis.nNonce, thash.ToString().c_str(), hashTarget.ToString().c_str());
//                    }
//                    ++genesis.nNonce;
//                    if (genesis.nNonce == 0)
//                    {
//                         printf("NONCE WRAPPED, incrementing time\n");
//                         ++genesis.nTime;
//                    }
//                }
//                printf("block.nTime = %u \n", genesis.nTime);
//                printf("block.nNonce = %u \n", genesis.nNonce);
//                printf("block.GetHash = %s\n", genesis.GetHash().ToString().c_str());

 
