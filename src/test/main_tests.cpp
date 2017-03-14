// Copyright (c) 2014-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "main.h"

#include "test/test_quantum.h"

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

static void TestBlockSubsidyHalvings(const Consensus::Params& consensusParams)
{
    int maxHalvings = 64;
    CAmount nInitialSubsidy = 50 * COIN;

    CAmount nPreviousSubsidy = nInitialSubsidy * 2; // for height == 0
    BOOST_CHECK_EQUAL(nPreviousSubsidy, nInitialSubsidy * 2);
    for (int nHalvings = 0; nHalvings < maxHalvings; nHalvings++) {
        //begin modif qtum
        //int nHeight = nHalvings * consensusParams.nSubsidyHalvingInterval;
        CAmount nSubsidy = GetProofOfWorkReward();
        //end modif qtum
        BOOST_CHECK(nSubsidy <= nInitialSubsidy);
        BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy / 2);
        nPreviousSubsidy = nSubsidy;
    }
    //begin modif qtum
    BOOST_CHECK_EQUAL(GetProofOfWorkReward(), 0);
   //end modif qtum
}

static void TestBlockSubsidyHalvings(int nSubsidyHalvingInterval)
{
    Consensus::Params consensusParams;
    //begin modif qtum
    //consensusParams.nSubsidyHalvingInterval = nSubsidyHalvingInterval;
    //end modif qtum
    TestBlockSubsidyHalvings(consensusParams);
}

// BOOST_AUTO_TEST_CASE(block_subsidy_test)
// {
//     TestBlockSubsidyHalvings(Params(CBaseChainParams::MAIN).GetConsensus()); // As in main
//     TestBlockSubsidyHalvings(150); // As in regtest
//     TestBlockSubsidyHalvings(1000); // Just another interval
// }

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    //begin modif qtum
    //const Consensus::Params& consensusParams = Params(CBaseChainParams::MAIN).GetConsensus();
    //end modif qtum
    CAmount nSum = 0;
    for (int nHeight = 0; nHeight < Params().LastPOWBlock(); nHeight += 1000) {
        //begin modif qtum
        CAmount nSubsidy = GetProofOfWorkReward();
        //end modif qtum
        BOOST_CHECK(nSubsidy <= 10000 * COIN);
        nSum += nSubsidy * 1000;
        BOOST_CHECK(MoneyRange(nSum));
    }
    BOOST_CHECK_EQUAL(nSum, 5000000000000000ULL);
}

bool ReturnFalse() { return false; }
bool ReturnTrue() { return true; }

BOOST_AUTO_TEST_CASE(test_combiner_all)
{
    boost::signals2::signal<bool (), CombinerAll> Test;
    BOOST_CHECK(Test());
    Test.connect(&ReturnFalse);
    BOOST_CHECK(!Test());
    Test.connect(&ReturnTrue);
    BOOST_CHECK(!Test());
    Test.disconnect(&ReturnFalse);
    BOOST_CHECK(Test());
    Test.disconnect(&ReturnTrue);
    BOOST_CHECK(Test());
}
BOOST_AUTO_TEST_SUITE_END()
