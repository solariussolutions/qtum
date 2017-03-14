/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file State.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "State.h"

#include <ctime>
#include <boost/filesystem.hpp>
#include <boost/timer.hpp>
#include <libdevcore/CommonIO.h> //+
#include <libdevcore/Assertions.h> //+
#include <libdevcore/TrieHash.h> //+
#include <libevmcore/Instruction.h> //+
#include <libethcore/Exceptions.h> //+
#include <libevm/VMFactory.h> //+
// #include "BlockChain.h"
#include "Defaults.h" //+
// #include "QuantumExtVM.h" // ExtVM
#include <libqtumevm/QuantumExtVM.h> //+
#include <libethereum/Transaction.h>
#include "Executive.h"
#include "CachedAddressState.h" //-
// #include "BlockChain.h"
// #include "TransactionQueue.h"
#include <functional>

using namespace std;
using namespace dev;
using namespace dev::eth;
namespace fs = boost::filesystem;

using valtype = vector<unsigned char>;

#define ETH_TIMED_ENACTMENTS 0

const char* StateSafeExceptions::name() { return EthViolet "⚙" EthBlue " ℹ"; }
const char* StateDetail::name() { return EthViolet "⚙" EthWhite " ◌"; }
const char* StateTrace::name() { return EthViolet "⚙" EthGray " ◎"; }
const char* StateChat::name() { return EthViolet "⚙" EthWhite " ◌"; }

State::State(u256 const& _accountStartNonce, OverlayDB const& _db, BaseState _bs):
	m_db(_db),
	m_state(&m_db),
	m_accountStartNonce(_accountStartNonce)
{
	if (_bs != BaseState::PreExisting)
		// Initialise to the state entailed by the genesis block; this guarantees the trie is built correctly.
		m_state.init();
	paranoia("end of normal construction.", true);
}

State::State(State const& _s):
	m_db(_s.m_db),
	m_state(&m_db, _s.m_state.root(), Verification::Skip),
	m_cache(_s.m_cache),
	m_touched(_s.m_touched),
	m_accountStartNonce(_s.m_accountStartNonce)
{
	paranoia("after state cloning (copy cons).", true);
}

OverlayDB State::openDB(std::string const& _basePath, h256 const& _genesisHash, WithExisting _we)
{
	std::string path = _basePath.empty() ? Defaults::get()->m_dbPath : _basePath;

	if (_we == WithExisting::Kill)
	{
		cnote << "Killing state database (WithExisting::Kill).";
		boost::filesystem::remove_all(path + "/state");
	}

	path += "/" + toHex(_genesisHash.ref().cropped(0, 4)) + "/" + toString(c_databaseVersion);
	boost::filesystem::create_directories(path);
	DEV_IGNORE_EXCEPTIONS(fs::permissions(path, fs::owner_all));

	ldb::Options o;
	o.max_open_files = 256;
	o.create_if_missing = true;
	ldb::DB* db = nullptr;
	ldb::Status status = ldb::DB::Open(o, path + "/state", &db);
	if (!status.ok() || !db)
	{
		if (boost::filesystem::space(path + "/state").available < 1024)
		{
			cwarn << "Not enough available space found on hard drive. Please free some up and then re-run. Bailing.";
			BOOST_THROW_EXCEPTION(NotEnoughAvailableSpace());
		}
		else
		{
			cwarn << status.ToString();
			cwarn <<
				"Database " <<
				(path + "/state") <<
				"already open. You appear to have another instance of ethereum running. Bailing.";
			BOOST_THROW_EXCEPTION(DatabaseAlreadyOpen());
		}
	}

	LogPrintfVM("Opened state DB.\n"); // TODO temp qtumLog
	// ctrace << "Opened state DB.";
	return OverlayDB(db);
}

void State::populateFrom(AccountMap const& _map)
{
	eth::commit(_map, m_state);
	commit();
}

u256 const& State::requireAccountStartNonce() const
{
	if (m_accountStartNonce == Invalid256)
		BOOST_THROW_EXCEPTION(InvalidAccountStartNonceInState());
	return m_accountStartNonce;
}

void State::noteAccountStartNonce(u256 const& _actual)
{
	if (m_accountStartNonce == Invalid256)
		m_accountStartNonce = _actual;
	else if (m_accountStartNonce != _actual)
		BOOST_THROW_EXCEPTION(IncorrectAccountStartNonceInState());
}

void State::paranoia(std::string const& _when, bool _enforceRefs) const
{
#if ETH_PARANOIA && !ETH_FATDB
	// TODO: variable on context; just need to work out when there should be no leftovers
	// [in general this is hard since contract alteration will result in nodes in the DB that are no directly part of the state DB].
	if (!isTrieGood(_enforceRefs, false))
	{
		cwarn << "BAD TRIE" << _when;
		BOOST_THROW_EXCEPTION(InvalidTrie());
	}
#else
	(void)_when;
	(void)_enforceRefs;
#endif
}

State& State::operator=(State const& _s)
{
	if (&_s == this)
		return *this;

	m_db = _s.m_db;
	m_state.open(&m_db, _s.m_state.root(), Verification::Skip);
	m_cache = _s.m_cache;
	m_touched = _s.m_touched;
	m_accountStartNonce = _s.m_accountStartNonce;
	paranoia("after state cloning (assignment op)", true);
	return *this;
}

StateDiff State::diff(State const& _c, bool _quick) const
{
	StateDiff ret;

	std::unordered_set<Address> ads;
	std::unordered_set<Address> trieAds;
	std::unordered_set<Address> trieAdsD;

	auto trie = SecureTrieDB<Address, OverlayDB>(const_cast<OverlayDB*>(&m_db), rootHash());
	auto trieD = SecureTrieDB<Address, OverlayDB>(const_cast<OverlayDB*>(&_c.m_db), _c.rootHash());

	if (_quick)
	{
		trieAds = m_touched;
		trieAdsD = _c.m_touched;
		(ads += m_touched) += _c.m_touched;
	}
	else
	{
		for (auto const& i: trie)
			ads.insert(i.first), trieAds.insert(i.first);
		for (auto const& i: trieD)
			ads.insert(i.first), trieAdsD.insert(i.first);
	}

	for (auto const& i: m_cache)
		ads.insert(i.first);
	for (auto const& i: _c.m_cache)
		ads.insert(i.first);

	for (auto const& i: ads)
	{
		auto it = m_cache.find(i);
		auto itD = _c.m_cache.find(i);
		CachedAddressState source(trieAds.count(i) ? trie.at(i) : "", it != m_cache.end() ? &it->second : nullptr, &m_db);
		CachedAddressState dest(trieAdsD.count(i) ? trieD.at(i) : "", itD != _c.m_cache.end() ? &itD->second : nullptr, &_c.m_db);
		AccountDiff acd = source.diff(dest);
		if (acd.changed())
			ret.accounts[i] = acd;
	}

	return ret;
}

void State::ensureCached(Address const& _a, bool _requireCode, bool _forceCreate) const
{
	ensureCached(m_cache, _a, _requireCode, _forceCreate);
}


void State::ensureCached(std::unordered_map<Address, Account>& _cache, const Address& _a, bool _requireCode, bool _forceCreate) const
{
	auto it = _cache.find(_a);
	if (it == _cache.end())
	{
		// populate basic info.
		string stateBack = m_state.at(_a);
		if (stateBack.empty() && !_forceCreate)
			return;
		RLP state(stateBack);
		Account s;
		if (state.isNull())
			s = Account(requireAccountStartNonce(), 0, Account::NormalCreation);
		else
			s = Account(state[0].toInt<u256>(), state[1].toInt<u256>(), state[2].toHash<h256>(), state[3].toHash<h256>(), Account::Unchanged);
		bool ok;
		tie(it, ok) = _cache.insert(make_pair(_a, s));
	}
	if (_requireCode && it != _cache.end() && !it->second.isFreshCode() && !it->second.codeCacheValid())
		it->second.noteCode(it->second.codeHash() == EmptySHA3 ? bytesConstRef() : bytesConstRef(m_db.lookup(it->second.codeHash())));
}

void State::commit()
{
	m_touched += dev::eth::commit(m_cache, m_state);
	m_cache.clear();
}

unordered_map<Address, u256> State::addresses() const
{
#if ETH_FATDB
	unordered_map<Address, u256> ret;
	for (auto i: m_cache)
		if (i.second.isAlive())
			ret[i.first] = i.second.balance();
	for (auto const& i: m_state)
		if (m_cache.find(i.first) == m_cache.end())
			ret[i.first] = RLP(i.second)[1].toInt<u256>();
	return ret;
#else
	BOOST_THROW_EXCEPTION(InterfaceNotSupported("State::addresses()"));
#endif
}

void State::setRoot(h256 const& _r)
{
	m_cache.clear();
	// m_touched.clear();
	m_state.setRoot(_r);
	paranoia("begin setRoot", true);
}

bool State::addressInUse(Address const& _id) const
{
	ensureCached(_id, false, false);
	auto it = m_cache.find(_id);
	if (it == m_cache.end())
		return false;
	return true;
}

bool State::addressHasCode(Address const& _id) const
{
	ensureCached(_id, false, false);
	auto it = m_cache.find(_id);
	if (it == m_cache.end())
		return false;
	return it->second.isFreshCode() || it->second.codeHash() != EmptySHA3;
}

u256 State::balance(Address const& _id) const
{
	ensureCached(_id, false, false);
	auto it = m_cache.find(_id);
	if (it == m_cache.end())
		return 0;
	return it->second.balance();
}

void State::noteSending(Address const& _id)
{
	ensureCached(_id, false, false);
	auto it = m_cache.find(_id);
	if (asserts(it != m_cache.end()))
	{
		cwarn << "Sending from non-existant account. How did it pay!?!";
		// this is impossible. but we'll continue regardless...
		m_cache[_id] = Account(requireAccountStartNonce() + 1, 0);
	}
	else
		it->second.incNonce();
}

void State::addBalance(Address const& _id, u256 const& _amount)
{
	ensureCached(_id, false, false);
	auto it = m_cache.find(_id);
	if (it == m_cache.end())
		m_cache[_id] = Account(requireAccountStartNonce(), _amount, Account::NormalCreation);
	else
		it->second.addBalance(_amount);
}

void State::subBalance(Address const& _id, bigint const& _amount)
{
	ensureCached(_id, false, false);
	auto it = m_cache.find(_id);
	if (it == m_cache.end() || (bigint)it->second.balance() < _amount)
		BOOST_THROW_EXCEPTION(NotEnoughCash());
	else
		it->second.addBalance(-_amount);
}

Address State::newContract(u256 const& _balance, bytes const& _code)
{
	auto h = sha3(_code);
	m_db.insert(h, &_code);
	while (true)
	{
		Address ret = Address::random();
		ensureCached(ret, false, false);
		auto it = m_cache.find(ret);
		if (it == m_cache.end())
		{
			m_cache[ret] = Account(requireAccountStartNonce(), _balance, EmptyTrie, h, Account::Changed);
			return ret;
		}
	}
}

u256 State::transactionsFrom(Address const& _id) const
{
	ensureCached(_id, false, false);
	auto it = m_cache.find(_id);
	if (it == m_cache.end())
		return m_accountStartNonce;
	else
		return it->second.nonce();
}

u256 State::storage(Address const& _id, u256 const& _memory) const
{
	ensureCached(_id, false, false);
	auto it = m_cache.find(_id);

	// Account doesn't exist - exit now.
	if (it == m_cache.end())
		return 0;

	// See if it's in the account's storage cache.
	auto mit = it->second.storageOverlay().find(_memory);
	if (mit != it->second.storageOverlay().end())
		return mit->second;

	// Not in the storage cache - go to the DB.
	SecureTrieDB<h256, OverlayDB> memdb(const_cast<OverlayDB*>(&m_db), it->second.baseRoot());			// promise we won't change the overlay! :)
	string payload = memdb.at(_memory);
	u256 ret = payload.size() ? RLP(payload).toInt<u256>() : 0;
	it->second.setStorage(_memory, ret);
	return ret;
}

unordered_map<u256, u256> State::storage(Address const& _id) const
{
	unordered_map<u256, u256> ret;

	ensureCached(_id, false, false);
	auto it = m_cache.find(_id);
	if (it != m_cache.end())
	{
		// Pull out all values from trie storage.
		if (it->second.baseRoot())
		{
			SecureTrieDB<h256, OverlayDB> memdb(const_cast<OverlayDB*>(&m_db), it->second.baseRoot());		// promise we won't alter the overlay! :)
			for (auto const& i: memdb)
				ret[i.first] = RLP(i.second).toInt<u256>();
		}

		// Then merge cached storage over the top.
		for (auto const& i: it->second.storageOverlay())
			if (i.second)
				ret[i.first] = i.second;
			else
				ret.erase(i.first);
	}
	return ret;
}

h256 State::storageRoot(Address const& _id) const
{
	string s = m_state.at(_id);
	if (s.size())
	{
		RLP r(s);
		return r[2].toHash<h256>();
	}
	return EmptyTrie;
}

bytes const& State::code(Address const& _contract) const
{
	if (!addressHasCode(_contract))
		return NullBytes;
	ensureCached(_contract, true, false);
	return m_cache[_contract].code();
}

h256 State::codeHash(Address const& _contract) const
{
	if (!addressHasCode(_contract))
		return EmptySHA3;
	if (m_cache[_contract].isFreshCode())
		return sha3(code(_contract));
	return m_cache[_contract].codeHash();
}

bool State::isTrieGood(bool _enforceRefs, bool _requireNoLeftOvers) const
{
	for (int e = 0; e < (_enforceRefs ? 2 : 1); ++e)
		try
		{
			EnforceRefs r(m_db, !!e);
			auto lo = m_state.leftOvers();
			if (!lo.empty() && _requireNoLeftOvers)
			{
				cwarn << "LEFTOVERS" << (e ? "[enforced" : "[unenforced") << "refs]";
				cnote << "Left:" << lo;
				cnote << "Keys:" << m_db.keys();
				m_state.debugStructure(cerr);
				return false;
			}
			// TODO: Enable once fixed.
/*			for (auto const& i: m_state)
			{
				RLP r(i.second);
				SecureTrieDB<h256, OverlayDB> storageDB(const_cast<OverlayDB*>(&m_db), r[2].toHash<h256>());	// promise not to alter OverlayDB.
				for (auto const& j: storageDB) { (void)j; }
				if (!e && r[3].toHash<h256>() != EmptySHA3 && m_db.lookup(r[3].toHash<h256>()).empty())
					return false;
			}*/
		}
		catch (InvalidTrie const&)
		{
			cwarn << "BAD TRIE" << (e ? "[enforced" : "[unenforced") << "refs]";
			cnote << m_db.keys();
			m_state.debugStructure(cerr);
			return false;
		}
	return true;
}

std::pair<ExecutionResult, TransactionReceipt> State::execute(EnvInfo const& _envInfo, SealEngineFace* _sealEngine, Transaction const& _t, Permanence _p, OnOpFunc const& _onOp)
{
	auto onOp = _onOp;

	// Create and initialize the executive. This will throw fairly cheaply and quickly if the
	// transaction is bad in any way.
	Executive e(*this, _envInfo, _sealEngine);
	ExecutionResult res;
	e.setResultRecipient(res);
	e.initialize(_t);

	// OK - transaction looks valid - execute.
	u256 startGasUsed = _envInfo.gasUsed();

	if (!e.execute())
		e.go(onOp);

	e.finalize();

	if (_p == Permanence::Reverted)
		m_cache.clear();
	else
	{
		commit();
		// TODO: CHECK TRIE after level DB flush to make sure exactly the same.
	}

//	TODO: LOGS
	return make_pair(res, TransactionReceipt(rootHash(), startGasUsed + e.gasUsed(), e.logs()));
}

std::ostream& dev::eth::operator<<(std::ostream& _out, State const& _s)
{
	_out << "--- " << _s.rootHash() << std::endl;
	std::set<Address> d;
	std::set<Address> dtr;
	auto trie = SecureTrieDB<Address, OverlayDB>(const_cast<OverlayDB*>(&_s.m_db), _s.rootHash());
	for (auto i: trie)
		d.insert(i.first), dtr.insert(i.first);
	for (auto i: _s.m_cache)
		d.insert(i.first);

	for (auto i: d)
	{
		auto it = _s.m_cache.find(i);
		Account* cache = it != _s.m_cache.end() ? &it->second : nullptr;
		string rlpString = dtr.count(i) ? trie.at(i) : "";
		RLP r(rlpString);
		assert(cache || r);

		if (cache && !cache->isAlive())
			_out << "XXX  " << i << std::endl;
		else
		{
			string lead = (cache ? r ? " *   " : " +   " : "     ");
			if (cache && r && cache->nonce() == r[0].toInt<u256>() && cache->balance() == r[1].toInt<u256>())
				lead = " .   ";

			stringstream contout;

			if ((cache && cache->codeBearing()) || (!cache && r && (h256)r[3] != EmptySHA3))
			{
				std::map<u256, u256> mem;
				std::set<u256> back;
				std::set<u256> delta;
				std::set<u256> cached;
				if (r)
				{
					SecureTrieDB<h256, OverlayDB> memdb(const_cast<OverlayDB*>(&_s.m_db), r[2].toHash<h256>());		// promise we won't alter the overlay! :)
					for (auto const& j: memdb)
						mem[j.first] = RLP(j.second).toInt<u256>(), back.insert(j.first);
				}
				if (cache)
					for (auto const& j: cache->storageOverlay())
					{
						if ((!mem.count(j.first) && j.second) || (mem.count(j.first) && mem.at(j.first) != j.second))
							mem[j.first] = j.second, delta.insert(j.first);
						else if (j.second)
							cached.insert(j.first);
					}
				if (!delta.empty())
					lead = (lead == " .   ") ? "*.*  " : "***  ";

				contout << " @:";
				if (!delta.empty())
					contout << "???";
				else
					contout << r[2].toHash<h256>();
				if (cache && cache->isFreshCode())
					contout << " $" << toHex(cache->code());
				else
					contout << " $" << (cache ? cache->codeHash() : r[3].toHash<h256>());

				for (auto const& j: mem)
					if (j.second)
						contout << std::endl << (delta.count(j.first) ? back.count(j.first) ? " *     " : " +     " : cached.count(j.first) ? " .     " : "       ") << std::hex << nouppercase << std::setw(64) << j.first << ": " << std::setw(0) << j.second ;
					else
						contout << std::endl << "XXX    " << std::hex << nouppercase << std::setw(64) << j.first << "";
			}
			else
				contout << " [SIMPLE]";
			_out << lead << i << ": " << std::dec << (cache ? cache->nonce() : r[0].toInt<u256>()) << " #:" << (cache ? cache->balance() : r[1].toInt<u256>()) << contout.str() << std::endl;
		}
	}
	return _out;
}

#if ETH_FATDB
static std::string minHex(h256 const& _h)
{
	unsigned i = 0;
	for (; i < 31 && !_h[i]; ++i) {}
	return toHex(_h.ref().cropped(i));
}
#endif

void State::streamJSON(ostream& _f) const
{
	_f << "{" << endl;
#if ETH_FATDB
	int fi = 0;
	for (pair<Address, u256> const& i: addresses())
	{
		_f << (fi++ ? "," : "") << "\"" << i.first.hex() << "\": { ";
		_f << "\"balance\": \"" << toString(i.second) << "\", ";
		if (codeHash(i.first) != EmptySHA3)
		{
			_f << "\"codeHash\": \"" << codeHash(i.first).hex() << "\", ";
			_f << "\"storage\": {";
			int fj = 0;
			for (pair<u256, u256> const& j: storage(i.first))
				_f << (fj++ ? "," : "") << "\"" << minHex(j.first) << "\":\"" << minHex(j.second) << "\"";
			_f << "}, ";
		}
		_f << "\"nonce\": \"" << toString(transactionsFrom(i.first)) << "\"";
		_f << "}" << endl;	// end account
		if (!(fi % 100))
			_f << flush;
	}
#endif
	_f << "}";
}



//////////////////////////////////////////////////////////////////////////////////// // TODO temp OutPoints
void QtumState::ensureCachedUTXO(Address const& _a){
	ensureCachedUTXO(m_cache_utxo, _a);
}

void QtumState::ensureCachedUTXO(std::unordered_map<Address, VinsInfo>& _cache, const Address& _a){
	auto it = _cache.find(_a);
	if (it == _cache.end())
	{
		// populate basic info.
		string stateBack = m_state_utxo.at(_a);
		if (stateBack.empty())
			return;
		RLP state(stateBack);
		VinsInfo s;
		if (state.isNull())
			s = VinsInfo();
		else
			s = VinsInfoDeserialize(state[0].toVector<std::pair<std::pair<u256, bigint>, u256>>());
		bool ok;
		tie(it, ok) = _cache.insert(make_pair(_a, s));
	}
}

VinsInfoSerial QtumState::VinsInfoSerialization(VinsInfo vins){
	VinsInfoSerial res;
	for(size_t i = 0; i < vins.size(); i++){
		res.push_back(std::make_pair(std::make_pair(u256(uintToh256(vins[i].first.hash)), vins[i].first.n), vins[i].second));
	}
	return res;
}

VinsInfo QtumState::VinsInfoDeserialize(VinsInfoSerial vins){
	VinsInfo res;
	for(size_t i = 0; i < vins.size(); i++){
		res.push_back(std::make_pair(COutPoint(h256Touint(h256(vins[i].first.first)), uint32_t(vins[i].first.second)), CAmount(vins[i].second)));
	}
	return res;
}

// bool QtumState::changed(Address const& _id){
// 	ensureCached(_id, false, false);
// 	auto it = m_cache.find(_id);
// 	if (it == m_cache.end())
// 		return false;
// 	else {
// 		it->second.m_isUnchanged = true;;
// 		return true;
// 	}
// }

void QtumState::sortOutPoints(VinsInfo& vins){

		std::pair<COutPoint, CAmount> tempOutPoint;

		for(unsigned int i = 0; i < vins.size(); i++){
			for(unsigned int j = i + 1; j < vins.size(); j++){
						
				if(vins[i].second < vins[j].second){
					tempOutPoint = vins[j];
					vins[j] = vins[i];
					vins[i] = tempOutPoint;
				}

				if(vins[i].second == vins[j].second){

					dev::u256 hashAndVoutI(dev::u256(uintToh256(vins[i].first.hash)) + dev::u256(vins[i].first.n));
					dev::u256 hashAndVoutJ(dev::u256(uintToh256(vins[j].first.hash)) + dev::u256(vins[j].first.n));

					if(hashAndVoutI < hashAndVoutJ){
						tempOutPoint = vins[j];
						vins[j] = vins[i];
						vins[i] = tempOutPoint;
					}
				}
			}
		}
	}

void QtumState::initUTXODB(std::string const& _path, h256 const& _genesisHash, WithExisting _we){
	m_db_utxo = State::openDB(_path + "/qtumDB", _genesisHash, _we);
	m_state_utxo = SecureTrieDB<Address, OverlayDB>(&m_db_utxo);
}

ResultExecute QtumState::execute(EnvInfo const& _envInfo, SealEngineFace* _sealEngine, QtumTransaction const& _t, Permanence _p, OnOpFunc const& _onOp){ // TODO temp QtumTransaction
	if(processingVersionNullAndOne(_sealEngine, _t, _p))
 		return ResultExecute{ExecutionResult(), TransactionReceipt(rootHash(), u256(), LogEntries()), std::vector<CTransaction>()};
	
	auto onOp = _onOp;
	std::vector<CTransaction> transactions;
	addBalance(_t.sender(), (_t.gas() * _t.gasPrice()) + _t.endowment()); // TODO temp dataToTx
	_sealEngine->delAddresses.insert(_sealEngine->delAddresses.end(), {_t.sender(), _envInfo.author()});

	// Create and initialize the executive. This will throw fairly cheaply and quickly if the
	// transaction is bad in any way.
	Executive e(*this, _envInfo, _sealEngine);
	ExecutionResult res;
	e.setResultRecipient(res);

	try{
		e.initialize(_t);
	}catch(...){
		return exceptionHandling(_t, _envInfo);
    }

	if(_t.isCreation()){ // TODO temp dataToTx
		_sealEngine->createQtumAddress(_t.getHashWith(), _t.getVoutNumber());
		addVin(_sealEngine->qtumAddress, std::make_pair(COutPoint(h256Touint(_t.getHashWith()), uint32_t(_t.getVoutNumber())), CAmount(0)));
	}

	// OK - transaction looks valid - execute.
	u256 startGasUsed = _envInfo.gasUsed();
	if (!e.execute())
		e.go(onOp);
	e.finalize();

	for(Address addr : _sealEngine->delAddresses){
		m_cache.erase(addr);
		m_cache_utxo.erase(addr);
	}

	if(res.excepted != TransactionException::None){
		LogPrintfVM("VMException: %s\n", res.excepted);
		return exceptionHandling(_t, _envInfo);
	}
	
	for(size_t i = 0; i < _sealEngine->txData.size(); i++){
		if(_sealEngine->txData[i].value > 0 && !(_t.getVersion() == 1 && _t.value() > 0 && i == 0)){
			transactions.push_back(TxGeneration(_sealEngine->txData[i], _envInfo, _t.sender()));
			savedVinToAccount(transactions.back(), _sealEngine->txData[i]);
		}
	}

	if (_p == Permanence::Reverted){
		m_cache.clear();
		m_cache_utxo.clear();
	} else {
		QtumState::commit();
		dbUTXO().commit();
	}

	res.gasRefunded = e.gas();
	std::cout << "New account address: " << e.newAddress().hex() << std::endl << "Balance: " << balance(e.newAddress()) << std::endl << std::endl;
	return ResultExecute{res, TransactionReceipt(rootHash(), startGasUsed + e.gasUsed(), e.logs()), transactions};
}

AddressHash QtumState::commitUTXO(){
	AddressHash ret;
	for (auto const& i: m_cache_utxo){
		if (!addressInUse(i.first))
			m_state_utxo.remove(i.first);
		else {
			RLPStream s(1);
			s.append(VinsInfoSerialization(i.second));

			m_state_utxo.insert(i.first, &s.out());
		}
		ret.insert(i.first);
	}
	m_cache_utxo.clear();
	return ret;
}

bool QtumState::processingVersionNullAndOne(SealEngineFace* _sealEngine, QtumTransaction const& _t, Permanence _p){ // TODO temp QtumTransaction
	if(_t.getVersion() == 0){
		addVin(_t.receiveAddress(), std::make_pair(COutPoint(h256Touint(_t.getHashWith()), _t.getVoutNumber()), CAmount(_t.value())));
 		addBalance(_t.receiveAddress(), _t.value());
 		if (_p == Permanence::Committed) {
 			commit();
 			dbUTXO().commit();
 			db().commit();
 		} else {
 			m_cache.clear();
 			m_cache_utxo.clear();
 		}
 		return true;
 	}
 	if(_t.getVersion() == 1 && _t.value() > 0){
 		addVin(_t.receiveAddress(), std::make_pair(COutPoint(h256Touint(_t.getHashWith()), _t.getVoutNumber()), CAmount(_t.value())));
 	}
 	return false;
}

ResultExecute QtumState::exceptionHandling(QtumTransaction const& _t, EnvInfo const& _envInfo){ // TODO temp QtumTransaction
	m_cache.erase(_t.sender()); 
	m_cache.erase(_envInfo.author());

	m_cache_utxo.erase(_t.receiveAddress());

	std::vector<CTransaction> transactions;
	if(_t.value() != 0){
        CTransaction tx(createP2PHTx(_t.getHashWith(), _t.getVoutNumber(), _t.value(), _t.sender()));
        transactions.push_back(tx);
    }
	ExecutionResult res;
	res.gasRefunded = 0;
	return ResultExecute{res, TransactionReceipt(rootHash(), _t.gas(), LogEntries()), transactions};
}

void QtumState::setRootUTXO(h256 const& _r){
	m_cache_utxo.clear();
	// m_touched.clear();
	m_state_utxo.setRoot(_r);
	paranoia("begin setRootUTXO", true);
}

CTransaction QtumState::createP2PHTx(h256 hashWith, unsigned char voutNum, u256 value, Address sender){
	CMutableTransaction tx;
	tx.vin.push_back(CTxIn(h256Touint(hashWith), uint32_t(voutNum), CScript() << OP_TXHASH));
	CScript scriptPubKeyIn(CScript() << OP_DUP << OP_HASH160 << sender.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
	tx.vout.push_back(CTxOut(CAmount(value), scriptPubKeyIn));
	return CTransaction(tx);
}

VinsInfo QtumState::getVins(Address const& _id){
	ensureCachedUTXO(_id);
	auto it = m_cache_utxo.find(_id);
	if (it == m_cache_utxo.end())
		return VinsInfo();
	return it->second;
}

void QtumState::setVins(Address const& _id, VinsInfo _amount){
	ensureCachedUTXO(_id);
	auto it = m_cache_utxo.find(_id);
	if (it == m_cache_utxo.end())
		m_cache_utxo[_id] = _amount;
	else
		it->second = _amount;
}

void QtumState::addVin(Address const& _id, vinInfo _amount){
	ensureCachedUTXO(_id);
	auto it = m_cache_utxo.find(_id);
	if (it == m_cache_utxo.end())
		m_cache_utxo[_id].push_back(_amount);
	else {
		it->second.push_back(_amount);
	}
}

void QtumState::addVins(Address const& _id, VinsInfo _amount){
	ensureCachedUTXO(_id);
	auto it = m_cache_utxo.find(_id);
	if (it == m_cache_utxo.end()){
		m_cache_utxo[_id] = _amount;
	} else {
		it->second.insert(it->second.end(), _amount.begin(), _amount.end());
	}
}

void QtumState::subVins(Address const& _id, size_t _amount){
	ensureCachedUTXO(_id);
	auto it = m_cache_utxo.find(_id);
	if (it == m_cache_utxo.end() || (bigint)it->second.size() < _amount)
		BOOST_THROW_EXCEPTION(NotEnoughCash());
	else
		for(size_t i = 0; i < _amount; i++)
			it->second.erase(it->second.begin() + 1);
}

bool QtumState::vinsInUse(Address const& _id){
	ensureCachedUTXO(_id);
	auto it = m_cache_utxo.find(_id);
	if (it == m_cache_utxo.end())
		return false;
	return true;
}

VinsInfo QtumState::SelectOutputs(VinsInfo& vins, dev::Address const& _from, CAmount const& _value, CAmount& sum){
    VinsInfo res;
	for(unsigned int i = 1; i < vins.size(); i++){
        sum += vins[i].second;
        res.push_back(vins[i]);
        if(_value <= sum){
            break;
        }
    }
    return res;
}

vector<CTxIn> QtumState::CreateInputs(VinsInfo outs){

    vector<CTxIn> res;
    for(size_t i = 0; i < outs.size(); i++){
        res.push_back(CTxIn(outs[i].first.hash, outs[i].first.n, CScript() << OP_TXHASH));
    }
    return res;
}

vector<CTxOut> QtumState::CreateOutputs(dev::Address const& _from, dev::Address const& _to, CAmount const& _value, CAmount& sum, TransferType type){

    vector<CTxOut> res;
    static std::map<TransferType,std::function<CScript(valtype)>> scriptTypes
    {{ContractToContract, [] (valtype _to) { return CScript() << valtype{0} << valtype{0} << valtype{0}
                << valtype(1, 0) << _to << OP_EXEC_ASSIGN; }},
    {ContractToPubkeyhash, [] (valtype _to) { return CScript() << OP_DUP << OP_HASH160 << _to
                << OP_EQUALVERIFY << OP_CHECKSIG;}}};

    res.push_back(CTxOut(CAmount(_value), scriptTypes.at(type)(_to.asBytes())));
    if (_value < sum){
        res.push_back(CTxOut(CAmount(sum - _value), scriptTypes.at(ContractToContract)(_from.asBytes())));
    }
    return res;
}

void QtumState::RemoveVout(VinsInfo& outs, dev::Address const& _from, size_t size){
    for(size_t i = 0; i < size; i++){
		outs.erase(outs.begin() + 1);
    }
}

CTransaction QtumState::TxGeneration(TxDataToGenerate& txData, EnvInfo const& _envInfo, Address sender){
	CTransaction transactionUTXO;

    VinsInfo vinsReceiver(getVins(txData.receiver));
    VinsInfo vinsSender(getVins(txData.sender));

    CMutableTransaction res;
    CAmount sum = 0;
    vector<CTxIn> vinIn = CreateInputs(SelectOutputs(vinsSender ,txData.sender, CAmount(txData.value), sum));

    if (!vinIn.empty()){
        res.vin = vinIn;

		bool notSenderAndAuthor = txData.receiver != sender && txData.receiver != _envInfo.author();
        TransferType type = addressInUse(txData.receiver) && notSenderAndAuthor ? ContractToContract : ContractToPubkeyhash;                    
        res.vout = CreateOutputs(txData.sender, txData.receiver, CAmount(txData.value), sum, type);

		txData.delVinSender = vinIn.size();

        transactionUTXO = CTransaction(res);
    }
	return transactionUTXO;
}

void QtumState::savedVinToAccount(CTransaction& tx,TxDataToGenerate& txData){
	subVins(txData.sender, txData.delVinSender);
	if(!tx.vout[0].scriptPubKey.IsPayToPubkeyHash())
		addVin(txData.receiver, vinInfo{COutPoint(tx.GetHash(), 0), tx.vout[0].nValue});
	if(tx.vout.size() > 1)
		addVin(txData.sender, vinInfo{COutPoint(tx.GetHash(), 1), tx.vout[1].nValue});
}
