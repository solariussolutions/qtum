/*Adapter classes for ETH constructs such as addresses and blocks */

#ifndef QUANTUMEVM_H
#define QUANTUMEVM_H

//block header
#include <algorithm>
#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/Exceptions.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/FileSystem.h>


namespace dev
{

using Secret = SecureFixedHash<32>;

/// A public key: 64 bytes.
/// @NOTE This is not endian-specific; it's just a bunch of bytes.
using Public = h512;

/// A signature: 65 bytes: r: [0, 32), s: [32, 64), v: 64.
/// @NOTE This is not endian-specific; it's just a bunch of bytes.
using Signature = h520;

/*struct SignatureStruct
{
	SignatureStruct() = default;
	SignatureStruct(Signature const& _s) { *(h520*)this = _s; }
	SignatureStruct(h256 const& _r, h256 const& _s, byte _v): r(_r), s(_s), v(_v) {}
	operator Signature() const { return *(h520 const*)this; }

	/// @returns true if r,s,v values are valid, otherwise false
	bool isValid() const noexcept;

	/// @returns the public part of the key that signed @a _hash to give this sig.
	Public recover(h256 const& _hash) const;

	h256 r;
	h256 s;
	byte v = 0;
};*/ 

/// An Ethereum address: 20 bytes.
/// @NOTE This is not endian-specific; it's just a bunch of bytes.
using Address = h160;

/// The zero address.
extern Address ZeroAddress;

/// A vector of Ethereum addresses.
using Addresses = h160s;

/// A hash set of Ethereum addresses.
using AddressHash = std::unordered_set<h160>;

/// A vector of secrets.
using Secrets = std::vector<Secret>;


/// Get information concerning the currency denominations.
std::vector<std::pair<u256, std::string>> const& units();

/// The log bloom's size (2048-bit).
using LogBloom = h2048;

/// Many log blooms.
using LogBlooms = std::vector<LogBloom>;

// The various denominations; here for ease of use where needed within code.
static const u256 ether = exp10<18>();
static const u256 finney = exp10<15>();
static const u256 szabo = exp10<12>();
static const u256 shannon = exp10<9>();
static const u256 wei = exp10<0>();

using Nonce = h64;

using BlockNumber = unsigned;

static const BlockNumber LatestBlock = (BlockNumber)-2;
static const BlockNumber PendingBlock = (BlockNumber)-1;
static const h256 LatestBlockHash = h256(2);
static const h256 EarliestBlockHash = h256(1);
static const h256 PendingBlockHash = h256(0);

static const u256 DefaultBlockGasLimit = 4712388;


namespace eth
{


//struct EVMSchedule
//{
//	EVMSchedule(): tierStepGas(std::array<unsigned, 8>{{0, 2, 3, 5, 8, 10, 20, 0}}) {}
//	EVMSchedule(bool _efcd, bool _hdc, unsigned const& _txCreateGas): exceptionalFailedCodeDeposit(_efcd), haveDelegateCall(_hdc), tierStepGas(std::array<unsigned, 8>{{0, 2, 3, 5, 8, 10, 20, 0}}), txCreateGas(_txCreateGas) {}
//	bool exceptionalFailedCodeDeposit = true;
//	bool haveDelegateCall = true;
//	unsigned stackLimit = 1024;
//	std::array<unsigned, 8> tierStepGas;
//	unsigned expGas = 10;
//	unsigned expByteGas = 10;
//	unsigned sha3Gas = 30;
//	unsigned sha3WordGas = 6;
//	unsigned sloadGas = 50;
//	unsigned sstoreSetGas = 20000;
//	unsigned sstoreResetGas = 5000;
//	unsigned sstoreRefundGas = 15000;
//	unsigned jumpdestGas = 1;
//	unsigned logGas = 375;
//	unsigned logDataGas = 8;
//	unsigned logTopicGas = 375;
//	unsigned createGas = 32000;
//	unsigned callGas = 40;
//	unsigned callStipend = 2300;
//	unsigned callValueTransferGas = 9000;
//	unsigned callNewAccountGas = 25000;
//	unsigned suicideRefundGas = 24000;
//	unsigned memoryGas = 3;
//	unsigned quadCoeffDiv = 512;
//	unsigned createDataGas = 200;
//	unsigned txGas = 21000;
//	unsigned txCreateGas = 53000;
//	unsigned txDataZeroGas = 4;
//	unsigned txDataNonZeroGas = 68;
//	unsigned copyGas = 3;
//};
//
//static const EVMSchedule DefaultSchedule = EVMSchedule();
//static const EVMSchedule FrontierSchedule = EVMSchedule(false, false, 21000);
//static const EVMSchedule HomesteadSchedule = EVMSchedule(true, true, 53000);


enum IncludeSeal
{
	WithoutSeal = 0,
	WithSeal = 1,
	OnlySeal = 2
};

enum Strictness
{
	CheckEverything,
	JustSeal,
	QuickNonce,
	IgnoreSeal,
	CheckNothingNew
};


enum BlockDataType
{
	HeaderData,
	BlockData
};

DEV_SIMPLE_EXCEPTION(NoHashRecorded);
DEV_SIMPLE_EXCEPTION(GenesisBlockCannotBeCalculated);

/** @brief Encapsulation of a block header.
 * Class to contain all of a block header's data. It is able to parse a block header and populate
 * from some given RLP block serialisation with the static fromHeader(), through the method
 * populate(). This will not conduct any verification above basic formating. In this case extra
 * verification can be performed through verify().
 *
 * The object may also be populated from an entire block through the explicit
 * constructor BlockHeader(bytesConstRef) and manually with the populate() method. These will
 * conduct verification of the header against the other information in the block.
 *
 * The object may be populated with a template given a parent BlockHeader object with the
 * populateFromParent() method. The genesis block info may be retrieved with genesis() and the
 * corresponding RLP block created with createGenesisBlock().
 *
 * To determine the header hash without the nonce (for sealing), the method hash(WithoutNonce) is
 * provided.
 *
 * The default constructor creates an empty object, which can be tested against with the boolean
 * conversion operator.
 */
class BlockHeader
{
public:
	static const unsigned BasicFields = 13;

	BlockHeader();
	explicit BlockHeader(bytesConstRef _data, BlockDataType _bdt = BlockData, h256 const& _hashWith = h256());
	explicit BlockHeader(bytes const& _data, BlockDataType _bdt = BlockData, h256 const& _hashWith = h256()): BlockHeader(&_data, _bdt, _hashWith) {}

	static h256 headerHashFromBlock(bytes const& _block) { return headerHashFromBlock(&_block); }
	static h256 headerHashFromBlock(bytesConstRef _block);
	static RLP extractHeader(bytesConstRef _block);

	explicit operator bool() const { return m_timestamp != Invalid256; }

	bool operator==(BlockHeader const& _cmp) const
	{
		return false;
		/*
		return m_parentHash == _cmp.parentHash() &&
			m_sha3Uncles == _cmp.sha3Uncles() &&
			m_author == _cmp.author() &&
			m_stateRoot == _cmp.stateRoot() &&
			m_transactionsRoot == _cmp.transactionsRoot() &&
			m_receiptsRoot == _cmp.receiptsRoot() &&
			m_logBloom == _cmp.logBloom() &&
			m_difficulty == _cmp.difficulty() &&
			m_number == _cmp.number() &&
			m_gasLimit == _cmp.gasLimit() &&
			m_gasUsed == _cmp.gasUsed() &&
			m_timestamp == _cmp.timestamp() &&
			m_extraData == _cmp.extraData(); */
	}
	bool operator!=(BlockHeader const& _cmp) const { return !operator==(_cmp); }

	void clear();
	void noteDirty() const { m_hashWithout = m_hash = h256(); }
	void populateFromParent(BlockHeader const& parent);

	// TODO: pull out into abstract class Verifier.
	void verify(Strictness _s = CheckEverything, BlockHeader const& _parent = BlockHeader(), bytesConstRef _block = bytesConstRef()) const;
	void verify(Strictness _s, bytesConstRef _block) const { verify(_s, BlockHeader(), _block); }

	h256 hash(IncludeSeal _i = WithSeal) const;
	void streamRLP(RLPStream& _s, IncludeSeal _i = WithSeal) const;

	void setParentHash(h256 const& _v) { m_parentHash = _v; noteDirty(); }
	void setSha3Uncles(h256 const& _v) { m_sha3Uncles = _v; noteDirty(); }
	void setTimestamp(u256 const& _v) { m_timestamp = _v; noteDirty(); }
	void setAuthor(Address const& _v) { m_author = _v; noteDirty(); }
	void setRoots(h256 const& _t, h256 const& _r, h256 const& _u, h256 const& _s) { m_transactionsRoot = _t; m_receiptsRoot = _r; m_stateRoot = _s; m_sha3Uncles = _u; noteDirty(); }
	void setGasUsed(u256 const& _v) { m_gasUsed = _v; noteDirty(); }
	void setNumber(u256 const& _v) { m_number = _v; noteDirty(); }
	void setGasLimit(u256 const& _v) { m_gasLimit = _v; noteDirty(); }
	void setExtraData(bytes const& _v) { m_extraData = _v; noteDirty(); }
	void setLogBloom(LogBloom const& _v) { m_logBloom = _v; noteDirty(); }
	void setDifficulty(u256 const& _v) { m_difficulty = _v; noteDirty(); }
	template <class T> void setSeal(unsigned _offset, T const& _value) { if (m_seal.size() <= _offset) m_seal.resize(_offset + 1); m_seal[_offset] = rlp(_value); noteDirty(); }
	template <class T> void setSeal(T const& _value) { setSeal(0, _value); }

	h256 const& parentHash() const { return m_parentHash; }
	h256 const& sha3Uncles() const { return m_sha3Uncles; }
	u256 const& timestamp() const { return m_timestamp; }
	Address const& author() const { return m_author; }
	h256 const& stateRoot() const { return m_stateRoot; }
	h256 const& transactionsRoot() const { return m_transactionsRoot; }
	h256 const& receiptsRoot() const { return m_receiptsRoot; }
	u256 const& gasUsed() const { return m_gasUsed; }
	u256 const& number() const { return m_number; }
	u256 const& gasLimit() const { return m_gasLimit; }
	bytes const& extraData() const { return m_extraData; }
	LogBloom const& logBloom() const { return m_logBloom; }
	u256 const& difficulty() const { return m_difficulty; }
	template <class T> T seal(unsigned _offset = 0) const { T ret; if (_offset < m_seal.size()) ret = RLP(m_seal[_offset]).convert<T>(RLP::VeryStrict); return ret; }

private:
	void populate(RLP const& _header);
	void streamRLPFields(RLPStream& _s) const;

	h256 m_parentHash;
	h256 m_sha3Uncles;
	h256 m_stateRoot;
	h256 m_transactionsRoot;
	h256 m_receiptsRoot;
	LogBloom m_logBloom;
	u256 m_number;
	u256 m_gasLimit;
	u256 m_gasUsed;
	bytes m_extraData;
	u256 m_timestamp = Invalid256;

	Address m_author;
	u256 m_difficulty;

	std::vector<bytes> m_seal;		///< Additional (RLP-encoded) header fields.

	mutable h256 m_hash;			///< (Memoised) SHA3 hash of the block header with seal.
	mutable h256 m_hashWithout;		///< (Memoised) SHA3 hash of the block header without seal.
};

inline std::ostream& operator<<(std::ostream& _out, BlockHeader const& _bi)
{
	_out << _bi.hash(WithoutSeal) << " " << _bi.parentHash() << " " << _bi.sha3Uncles() << " " << _bi.author() << " " << _bi.stateRoot() << " " << _bi.transactionsRoot() << " " <<
			_bi.receiptsRoot() << " " << _bi.logBloom() << " " << _bi.difficulty() << " " << _bi.number() << " " << _bi.gasLimit() << " " <<
			_bi.gasUsed() << " " << _bi.timestamp();
	return _out;
}



} //end namespaces
}

#endif
