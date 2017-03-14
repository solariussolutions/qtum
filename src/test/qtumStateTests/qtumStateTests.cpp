#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

#include <libethereum/State.h>
#include <libethashseal/GenesisInfo.h>
#include <libevm/ExtVMFace.h>
#include <libdevcore/CommonIO.h>
#include "evm/libethereum/ChainParams.h"
#include "utilstrencodings.h"

#include "util.h"

#include <fstream>
#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_reader_template.h>
#include <json_spirit/json_spirit_writer_template.h>

using namespace std;
using namespace dev;
using namespace dev::eth;

using vinInfo = std::pair<COutPoint, CAmount>;
using VinsInfo = std::vector<std::pair<COutPoint, CAmount>>;

dev::eth::QtumState* StateTest;

struct InfoForTx{
    int64_t version;
    u256 value;

    u256 gasLimit;

    std::vector<unsigned char> code;
    Address receiveAddress;
    Address sender;
    h256 hash;
};

struct ResultAccountInfo{
    Address accountAddress;
    u256 balance;
    VinsInfo vins;
};

enum TypeFile{ TEST, RESULT };

std::string GetPathTestFile(std::string nameMethod, TypeFile type = TEST){
    boost::filesystem::path full_path(boost::filesystem::current_path());
    if(type == RESULT)
        return full_path.string() + "/test/qtumStateTests/ResultExecution/result" + nameMethod + ".json";
    return full_path.string() + "/test/qtumStateTests/qtumState" + nameMethod + ".json";
}

void InitQtumState(BaseState _bs = BaseState::PreExisting){
    dev::eth::Ethash::init();
    boost::filesystem::path full_path(boost::filesystem::current_path());
    std::string path = full_path.string() + "/test/qtumStateTests/state/";
    const dev::u256 accountStartNonce(0); 
    const dev::h256 hashBlock(dev::sha3(dev::rlp("")));
    StateTest = new dev::eth::QtumState(accountStartNonce,
                                dev::eth::State::openDB(path, hashBlock, dev::WithExisting::Trust),
                                path, hashBlock, _bs);
    StateTest->setRootUTXO(hashBlock);
}

void DelQtumState(){
    delete StateTest;
    StateTest = NULL;
}

QtumTransaction CreateTransaction(size_t version, std::vector<unsigned char> code, h256 hashTX, u256 value, u256 gasLimit = 500000, Address to = Address()){
    QtumTransaction ethTx;
    if(to == Address()){
        ethTx = QtumTransaction(u256(0), u256(1), u256(gasLimit * 1), code, dev::u256(0));
    } else {
        ethTx = QtumTransaction(value, u256(1), u256(gasLimit * 1), to, code, dev::u256(0));
    }
    ethTx.forceSender(Address("d799ea13055403da20eaf210fc5c30909889e8fa"));
    ethTx.setVoutNumber(0);
    ethTx.setHashWith(hashTX);
    ethTx.setVersion(version);
    return ethTx;
}

std::vector<QtumTransaction> ReadInfoForTest(std::string file){
    ifstream is(file);
    json_spirit::Value value;
    read_stream(is, value);
    is.close();

    json_spirit::Array& transactionsJSON = value.get_array();
    std::vector<QtumTransaction> transactions;

    for(size_t j = 0; j < transactionsJSON.size(); ++j){
        json_spirit::Object obj = transactionsJSON[j].get_obj();
        InfoForTx info{1, 0, 0, ParseHex("00"), Address(), Address(), h256()};
        info.version = obj[0].value_.get_int64();
        info.value = u256(obj[1].value_.get_int64());

        info.gasLimit = u256(obj[2].value_.get_int64());

        info.code = ParseHex(obj[3].value_.get_str());
        info.receiveAddress = obj[4].value_.get_str().size() != 0 ? Address(ParseHex(obj[4].value_.get_str())) : Address();
        info.sender = Address(ParseHex(obj[5].value_.get_str()));
        info.hash = uintToh256(uint256(ParseHex(obj[6].value_.get_str())));
        transactions.push_back(CreateTransaction(info.version, info.code, info.hash, info.value, info.gasLimit, info.receiveAddress));
    }
    return transactions;
}

std::vector<ResultAccountInfo> ReadResultExecution(std::string file, std::vector<uint256>& hashes){
    ifstream is(file);
    json_spirit::Value value;
    read_stream(is, value);
    is.close();

    std::vector<ResultAccountInfo> RAI;

    json_spirit::Object object = value.get_obj();

    json_spirit::Array vinsAcc;
    json_spirit::Array& accounts = object[0].value_.get_array();

    for(size_t i = 0; i < accounts.size(); ++i){
        ResultAccountInfo raii;
        json_spirit::Object obj = accounts[i].get_obj();
        raii.accountAddress = Address(obj[0].value_.get_str());
        raii.balance = u256(uintToh256(uint256(ParseHex(obj[1].value_.get_str()))));

        vinsAcc = obj[2].value_.get_array();
        VinsInfo vins;
        for(size_t j = 0; j < vinsAcc.size(); j++){
            json_spirit::Object resAccInfo = vinsAcc[j].get_obj();
            vinInfo vin;
            vin.first.hash = uint256(ParseHex(resAccInfo[0].value_.get_str()));
            vin.first.n = uint32_t(resAccInfo[1].value_.get_int64());
            vin.second = resAccInfo[2].value_.get_int64();
            vins.push_back(vin);
        }
        raii.vins = vins;
        RAI.push_back(raii);
    }

    json_spirit::Array& txHashes = object[1].value_.get_array();
    for(size_t i = 0; i < txHashes.size(); i++){
        hashes.push_back(uint256(ParseHex(txHashes[i].get_str())));
    }
    return RAI;
}

ResultExecute Execute(QtumTransaction ethTx){
    EnvInfo envInfo;
    envInfo.setAuthor(Address("2ce42a7c257411ad96b77e271fa93c6d95b8ae22"));
    envInfo.setGasLimit(1 << 31);
    unique_ptr<dev::eth::SealEngineFace> se(ChainParams(genesisInfo(Network::HomesteadTest)).createSealEngine());
    using OnOpFunc = std::function<void(uint64_t /*steps*/, uint64_t /* PC */, dev::eth::Instruction /*instr*/, dev::bigint /*newMemSize*/, dev::bigint /*gasCost*/, dev::bigint /*gas*/, dev::eth::VM*, dev::eth::ExtVMFace const*)>;
    return StateTest->execute(envInfo, se.get(), ethTx, Permanence::Committed, OnOpFunc());
}

std::vector<ResultExecute> ExecuteQtumTest(std::vector<QtumTransaction> txs){
    std::vector<ResultExecute> res;
    for(size_t i = 0; i < txs.size(); i++){
        res.push_back(Execute(txs[i]));
    }
    StateTest->db().commit();
    StateTest->dbUTXO().commit();
    return res;
}

void CheckResultExecution(std::vector<ResultAccountInfo>& accounts, std::vector<uint256>& hashes, std::vector<ResultExecute>& res){
    std::unordered_map<Address, u256> addresses(StateTest->addresses());
    for(size_t i = 0; i < accounts.size(); i++){
        BOOST_CHECK(addresses.count(accounts[i].accountAddress));
        BOOST_CHECK(StateTest->getVins(accounts[i].accountAddress) == accounts[i].vins);
        BOOST_CHECK(StateTest->balance(accounts[i].accountAddress) == accounts[i].balance);
    }
    for(size_t i = 0; i < res.back().txs.size(); i++){
        BOOST_CHECK(res.back().txs[i].GetHash() == hashes[i]);
    }
}

void RunTest(std::string nameTest){
    InitQtumState(dev::eth::BaseState::Empty);
    std::vector<QtumTransaction> txs(ReadInfoForTest(GetPathTestFile(nameTest)));
    std::vector<ResultExecute> res = ExecuteQtumTest(txs);

    std::vector<uint256> hashes;
    std::vector<ResultAccountInfo> accountInfo = ReadResultExecution(GetPathTestFile(nameTest, RESULT), hashes);
    CheckResultExecution(accountInfo, hashes, res);
    DelQtumState();
}

void SuicideTest(){
    InitQtumState(dev::eth::BaseState::Empty);
    std::vector<QtumTransaction> txs(ReadInfoForTest(GetPathTestFile(__func__)));
    std::vector<ResultExecute> res = ExecuteQtumTest(txs);

    Address receiver("91f3d660fdff65857426038bb8de7bca6255fc01");    
    BOOST_CHECK(StateTest->addresses().count(receiver) == 0);
    std::vector<uint256> hashes;
    std::vector<ResultAccountInfo> accountInfo = ReadResultExecution(GetPathTestFile(__func__, RESULT), hashes);
    CheckResultExecution(accountInfo, hashes, res);
    DelQtumState();
}

void CallMethodTest(){
    InitQtumState(dev::eth::BaseState::Empty);
    std::vector<QtumTransaction> txs(ReadInfoForTest(GetPathTestFile(__func__)));
    std::vector<ResultExecute> res = ExecuteQtumTest(txs);

    BOOST_CHECK(HexStr(res.back().execRes.output) == std::string("000000000000000000000000000000000000000000000000000000000000001a"));
    std::vector<uint256> hashes;
    std::vector<ResultAccountInfo> accountInfo = ReadResultExecution(GetPathTestFile(__func__, RESULT), hashes);
    CheckResultExecution(accountInfo, hashes, res);
    DelQtumState();
}

void TransferAmountAndCallMethodTest(){
    InitQtumState(dev::eth::BaseState::Empty);
    std::vector<QtumTransaction> txs(ReadInfoForTest(GetPathTestFile(__func__)));
    std::vector<ResultExecute> res = ExecuteQtumTest(txs);

    BOOST_CHECK(HexStr(res.back().execRes.output) == std::string("000000000000000000000000000000000000000000000000000000000000001a"));
    std::vector<uint256> hashes;
    std::vector<ResultAccountInfo> accountInfo = ReadResultExecution(GetPathTestFile(__func__, RESULT), hashes);
    CheckResultExecution(accountInfo, hashes, res);
    DelQtumState();
}

void AccCallSuicideAccTest(){
    InitQtumState(dev::eth::BaseState::Empty);
    std::vector<QtumTransaction> txs(ReadInfoForTest(GetPathTestFile(__func__)));
    std::vector<ResultExecute> res = ExecuteQtumTest(txs);

    BOOST_CHECK(!StateTest->addresses().count(Address("a17f9c425d1f462da07341ddff103b088dc7a0fb")));
    std::vector<uint256> hashes;
    std::vector<ResultAccountInfo> accountInfo = ReadResultExecution(GetPathTestFile(__func__, RESULT), hashes);
    CheckResultExecution(accountInfo, hashes, res);
    DelQtumState();
}

void WriteAndReadDBTest(){
    InitQtumState(dev::eth::BaseState::Empty);
    std::vector<QtumTransaction> txs(ReadInfoForTest(GetPathTestFile(__func__)));
    std::vector<ResultExecute> res = ExecuteQtumTest(txs);

    StateTest->db().commit();
    StateTest->dbUTXO().commit();
    h256 rootHashState = StateTest->rootHash();
    h256 rootHashUTXO = StateTest->rootHashUTXO();

    std::vector<std::pair<Address, VinsInfo>> accountsVinsBefore;
    std::unordered_map<Address, u256> accountsBefore = StateTest->addresses();
    for(std::pair<Address, u256> acc : accountsBefore){
        accountsVinsBefore.push_back(make_pair(acc.first, StateTest->getVins(acc.first)));
    }
    DelQtumState();

    InitQtumState();
    StateTest->setRoot(rootHashState);
    StateTest->setRootUTXO(rootHashUTXO);

    std::unordered_map<Address, u256> accountsAfter = StateTest->addresses();
    for(std::pair<Address, VinsInfo> acc : accountsVinsBefore){
        BOOST_CHECK(acc.second == StateTest->getVins(acc.first));
    }
    BOOST_CHECK(accountsBefore == accountsAfter);
    DelQtumState();
}

BOOST_AUTO_TEST_SUITE(QtumStateTest)

BOOST_AUTO_TEST_CASE(qtumStateCreateAccountTest){
    RunTest(std::string("CreateAccountTest"));
}

BOOST_AUTO_TEST_CASE(qtumStateTransferAmountUTXOToAccTest){
    RunTest(std::string("TransferAmountUTXOToAccTest"));
}

BOOST_AUTO_TEST_CASE(qtumStateTransferAmountAccToAccTest){
    RunTest(std::string("TransferAmountAccToAccTest"));
}

BOOST_AUTO_TEST_CASE(qtumStateTransferAmountAccToAccAndUTXOTest){
    RunTest(std::string("TransferAmountAccToAccAndUTXOTest"));
}

BOOST_AUTO_TEST_CASE(qtumStateTransferAmountAccToAccManyVoutTest){
    RunTest(std::string("TransferAmountAccToAccManyVoutTest"));
}

BOOST_AUTO_TEST_CASE(qtumStateOutOfGasNoValueTest){
    RunTest(std::string("OutOfGasNoValueTest"));
}

BOOST_AUTO_TEST_CASE(qtumStateOutOfGasValueTest){
    RunTest(std::string("OutOfGasValueTest"));
}

BOOST_AUTO_TEST_CASE(qtumStateOutOfGasBaseNoValueTest){
    RunTest(std::string("OutOfGasBaseNoValueTest"));
}

BOOST_AUTO_TEST_CASE(qtumStateOutOfGasBaseValueTest){
    RunTest(std::string("OutOfGasBaseValueTest"));
}

BOOST_AUTO_TEST_CASE(qtumStateSuicideTest){
    SuicideTest(); 
}

BOOST_AUTO_TEST_CASE(qtumStateCallMethodTest){
    CallMethodTest(); 
}

BOOST_AUTO_TEST_CASE(qtumStateTransferAmountAndCallMethodTest){
    TransferAmountAndCallMethodTest(); 
}

BOOST_AUTO_TEST_CASE(qtumStateAccCallSuicideAccTest){
    AccCallSuicideAccTest(); 
}

BOOST_AUTO_TEST_CASE(qtumStateWriteAndReadDBTest){
    WriteAndReadDBTest(); 
}

BOOST_AUTO_TEST_SUITE_END()

// void WriteAccount(std::string file){
//     std::vector<ResultAccountInfo> RAI;
//     std::unordered_map<Address, u256> addresses(StateTest->addresses());
//     for(std::pair<Address, u256> acc : addresses){ 
//         RAI.push_back({acc.first, StateTest->balance(acc.first), StateTest->getVins(acc.first)});
//     }


//     json_spirit::Array accounts;
//     json_spirit::Object resAccInfo;
//     json_spirit::Array vinsAcc;
//     json_spirit::Object vinAcc;

//     for(size_t l = 0; l < RAI.size(); l++){

//         resAccInfo.clear();
//         resAccInfo.push_back(json_spirit::Pair("accountAddress", RAI[l].accountAddress.hex()));
//         uint256 balance = h256Touint(h256(RAI[l].balance));
//         std::reverse(balance.begin(), balance.end());
//         resAccInfo.push_back(json_spirit::Pair("balance", balance.GetHex()));

//         vinsAcc.clear();
//         for(size_t i = 0; i < RAI[l].vins.size(); i++){
//             vinAcc.clear();
//             std::reverse(RAI[l].vins[i].first.hash.begin(), RAI[l].vins[i].first.hash.end());
//             std::string hash(RAI[l].vins[i].first.hash.ToString());
//             vinAcc.push_back(json_spirit::Pair("hash", hash));
//             vinAcc.push_back(json_spirit::Pair("nVout", uint64_t(RAI[l].vins[i].first.n)));
//             vinAcc.push_back(json_spirit::Pair("amount", RAI[l].vins[i].second));
//             vinsAcc.push_back(vinAcc);
//         }
//         resAccInfo.push_back(json_spirit::Pair("vins", vinsAcc));
//         accounts.push_back(resAccInfo);
//     }

//     ofstream os;
//     os.open(file);
//     json_spirit::write_stream(json_spirit::Value(accounts), os, true);
//     os.close();
// }
