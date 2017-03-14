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
/** @file SealEngine.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * Determines the PoW algorithm.
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <libdevcore/Guards.h>
#include <libdevcore/RLP.h>
// #include "BlockHeader.h"
#include "Common.h"
// #include "libevmcore/EVMSchedule.h"
#include "ChainOperationParams.h"
#include "libevm/ExtVMFace.h"

#include "uint256.h"
#include "crypto/ripemd160.h"
#include "crypto/sha256.h"
#define UNUSED(x) (void)(x);

namespace dev
{
namespace eth
{

// class BlockHeader;
struct ChainOperationParams;
// class Interface;
// class PrecompiledFace;
class TransactionBase;
// class EnvInfo;

/////////////////////////////////////////////// // TODO temp dataToTx
struct TxDataToGenerate
{
	Address sender;
	Address receiver;
	u256 value;

	size_t delVinSender;
};
///////////////////////////////////////////////

class SealEngineFace
{
public:

	virtual std::string name() const = 0;
	// virtual unsigned revision() const { return 0; }
	virtual unsigned sealFields() const { return 0; }
	virtual bytes sealRLP() const { return bytes(); }
	// virtual StringHashMap jsInfo(BlockHeader const&) const { return StringHashMap(); }

	// /// Don't forget to call Super::verify when subclassing & overriding.
	// virtual void verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent = BlockHeader(), bytesConstRef _block = bytesConstRef()) const;
	// /// Additional verification for transactions in blocks.
	// virtual void verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t, BlockHeader const& _bi) const;
	// /// Don't forget to call Super::populateFromParent when subclassing & overriding.
	// virtual void populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const;

	// bytes option(std::string const& _name) const { Guard l(x_options); return m_options.count(_name) ? m_options.at(_name) : bytes(); }
	// bool setOption(std::string const& _name, bytes const& _value) { Guard l(x_options); try { if (onOptionChanging(_name, _value)) { m_options[_name] = _value; return true; } } catch (...) {} return false; }

	// virtual strings sealers() const { return { "default" }; }
	// virtual std::string sealer() const { return "default"; }
	// virtual void setSealer(std::string const&) {}

	// virtual bool shouldSeal(Interface*) { return true; }
	// virtual void generateSeal(BlockHeader const& _bi) = 0;
	// virtual void onSealGenerated(std::function<void(bytes const& s)> const& _f) = 0;
	// virtual void cancelGeneration() {}

	ChainOperationParams const& chainParams() const { return m_params; }
	void setChainParams(ChainOperationParams const& _params) { m_params = _params; }
	SealEngineFace* withChainParams(ChainOperationParams const& _params) { setChainParams(_params); return this; }

	bool isPrecompiled(Address const& _a) const { return m_params.precompiled.count(_a); }
	bigint costOfPrecompiled(Address const& _a, bytesConstRef _in) const { return m_params.precompiled.at(_a).cost(_in); }
	void executePrecompiled(Address const& _a, bytesConstRef _in, bytesRef _out) const { return m_params.precompiled.at(_a).execute(_in, _out); }
	virtual EVMSchedule const& evmSchedule(EnvInfo const&) const { return m_params.evmSchedule; }

/////////////////////////////////////////////// // TODO temp dataToTx
	std::vector<TxDataToGenerate> txData;
	std::vector<Address> delAddresses;
	Address qtumAddress;
	void createQtumAddress(h256 hashTx, unsigned char voutNumber){
		unsigned char version = 9;
		UNUSED(version);
		uint256 hashTXid(h256Touint(hashTx));
		std::vector<unsigned char> txIdAndVout(hashTXid.begin(), hashTXid.end());
		txIdAndVout.push_back(voutNumber);
		
		std::vector<unsigned char> SHA256TxVout(32);
		CSHA256().Write(begin_ptr(txIdAndVout), txIdAndVout.size()).Finalize(begin_ptr(SHA256TxVout));

		std::vector<unsigned char> hashTxIdAndVout(20);
		CRIPEMD160().Write(begin_ptr(SHA256TxVout), SHA256TxVout.size()).Finalize(begin_ptr(hashTxIdAndVout));
		
		qtumAddress = h160(hashTxIdAndVout);

		/*hashTxIdAndVout.insert(hashTxIdAndVout.begin(), version);

		vector<unsigned char> checksumTemp(32);
		vector<unsigned char> checksum(32);
		CSHA256().Write(begin_ptr(hashTxIdAndVout), hashTxIdAndVout.size()).Finalize(begin_ptr(checksumTemp));
		CSHA256().Write(begin_ptr(checksumTemp), checksumTemp.size()).Finalize(begin_ptr(checksum));

		hashTxIdAndVout.insert(hashTxIdAndVout.end(), checksum[0]);
		hashTxIdAndVout.insert(hashTxIdAndVout.end(), checksum[1]);
		hashTxIdAndVout.insert(hashTxIdAndVout.end(), checksum[2]);
		hashTxIdAndVout.insert(hashTxIdAndVout.end(), checksum[3]);*/
	}
///////////////////////////////////////////////

// protected:
// 	virtual bool onOptionChanging(std::string const&, bytes const&) { return true; }
// 	void injectOption(std::string const& _name, bytes const& _value) { Guard l(x_options); m_options[_name] = _value; }

private:
	// mutable Mutex x_options;
	// std::unordered_map<std::string, bytes> m_options;

	ChainOperationParams m_params;
};

class SealEngineBase: public SealEngineFace
{
public:
	// void generateSeal(BlockHeader const& _bi) override
	// {
	// 	RLPStream ret;
	// 	_bi.streamRLP(ret);
	// 	if (m_onSealGenerated)
	// 		m_onSealGenerated(ret.out());
	// }
	// void onSealGenerated(std::function<void(bytes const&)> const& _f) override { m_onSealGenerated = _f; }

private:
	std::function<void(bytes const& s)> m_onSealGenerated;
};

using SealEngineFactory = std::function<SealEngineFace*()>;

class SealEngineRegistrar
{
public:
	/// Creates the seal engine and uses it to "polish" the params (i.e. fill in implicit values) as necessary. Use this rather than the other two
	/// unless you *know* that the params contain all information regarding the seal on the Genesis block.
	static SealEngineFace* create(ChainOperationParams const& _params);
	static SealEngineFace* create(std::string const& _name) { if (!get()->m_sealEngines.count(_name)) return nullptr; return get()->m_sealEngines[_name](); }

	template <class SealEngine> static SealEngineFactory registerSealEngine(std::string const& _name) { return (get()->m_sealEngines[_name] = [](){return new SealEngine;}); }
	// static void unregisterSealEngine(std::string const& _name) { get()->m_sealEngines.erase(_name); }

private:
	static SealEngineRegistrar* get() { if (!s_this) s_this = new SealEngineRegistrar; return s_this; }

	std::unordered_map<std::string, SealEngineFactory> m_sealEngines;
	static SealEngineRegistrar* s_this;
};

#define ETH_REGISTER_SEAL_ENGINE(Name) static SealEngineFactory __eth_registerSealEngineFactory ## Name = SealEngineRegistrar::registerSealEngine<Name>(#Name)

class NoProof: public eth::SealEngineBase
{
public:
	std::string name() const override { return "NoProof"; }
	static void init();
};

class Ethash: public SealEngineFace
{
public:
	// Ethash();

	std::string name() const override { return "Ethash"; }
	// unsigned revision() const override { return 1; }
	unsigned sealFields() const override { return 2; }
	bytes sealRLP() const override { return rlp(h256()) + rlp(Nonce()); }

	// StringHashMap jsInfo(BlockHeader const& _bi) const override;
	// void verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const override;
	// void verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t, BlockHeader const& _bi) const override;
	// void populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const override;

	// strings sealers() const override;
	// std::string sealer() const override { return m_sealer; }
	// void setSealer(std::string const& _sealer) override { m_sealer = _sealer; }
	// void cancelGeneration() override { m_farm.stop(); }
	// void generateSeal(BlockHeader const& _bi) override;
	// void onSealGenerated(std::function<void(bytes const&)> const& _f) override;
	// bool shouldSeal(Interface* _i) override;

	// eth::GenericFarm<EthashProofOfWork>& farm() { return m_farm; }

	// enum { MixHashField = 0, NonceField = 1 };
	// static h256 seedHash(BlockHeader const& _bi);
	// static Nonce nonce(BlockHeader const& _bi) { return _bi.seal<Nonce>(NonceField); }
	// static h256 mixHash(BlockHeader const& _bi) { return _bi.seal<h256>(MixHashField); }
	// static h256 boundary(BlockHeader const& _bi) { auto d = _bi.difficulty(); return d ? (h256)u256((bigint(1) << 256) / d) : h256(); }
	// static BlockHeader& setNonce(BlockHeader& _bi, Nonce _v) { _bi.setSeal(NonceField, _v); return _bi; }
	// static BlockHeader& setMixHash(BlockHeader& _bi, h256 const& _v) { _bi.setSeal(MixHashField, _v); return _bi; }

	// u256 calculateDifficulty(BlockHeader const& _bi, BlockHeader const& _parent) const;
	// u256 childGasLimit(BlockHeader const& _bi, u256 const& _gasFloorTarget = Invalid256) const;

	virtual EVMSchedule const& evmSchedule(EnvInfo const&) const override;

	// void manuallySetWork(BlockHeader const& _work) { m_sealing = _work; }
	// void manuallySubmitWork(h256 const& _mixHash, Nonce _nonce);

	// static void ensurePrecomputed(unsigned _number);
	static void init();

private:
	// bool verifySeal(BlockHeader const& _bi) const;
	// bool quickVerifySeal(BlockHeader const& _bi) const;

	// eth::GenericFarm<EthashProofOfWork> m_farm;
	// std::string m_sealer = "cpu";
	// BlockHeader m_sealing;
	std::function<void(bytes const&)> m_onSealGenerated;
};

}
}
