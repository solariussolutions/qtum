DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS)
AM_CXXFLAGS = $(HARDENED_CXXFLAGS)
AM_CPPFLAGS = $(HARDENED_CPPFLAGS)
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la
BOOST_LIBS += -lboost_random
CRYPTOPP_LIBS = -lcryptopp

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

QUANTUM_CONFIG_INCLUDES=-I$(builddir)/config
QUANTUM_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)

QUANTUM_INCLUDES += -I$(srcdir)/secp256k1/include
QUANTUM_INCLUDES += $(UNIVALUE_CFLAGS)
QUANTUM_INCLUDES += -I$(srcdir)/evm

LIBQUANTUM_SERVER=libquantum_server.a
LIBQUANTUM_COMMON=libquantum_common.a
LIBQUANTUM_CONSENSUS=libquantum_consensus.a
LIBQUANTUM_CLI=libquantum_cli.a
LIBQUANTUM_UTIL=libquantum_util.a
LIBQUANTUM_CRYPTO=crypto/libquantum_crypto.a
LIBQUANTUMQT=qt/libquantumqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la

if ENABLE_ZMQ
LIBQUANTUM_ZMQ=libquantum_zmq.a
endif
if BUILD_QUANTUM_LIBS
LIBQUANTUMCONSENSUS=libquantumconsensus.la
endif
if ENABLE_WALLET
LIBQUANTUM_WALLET=libquantum_wallet.a
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
  
$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBQUANTUM_CRYPTO) \
  $(LIBQUANTUM_UTIL) \
  $(LIBQUANTUM_COMMON) \
  $(LIBQUANTUM_CONSENSUS) \
  $(LIBQUANTUM_SERVER) \
  $(LIBQUANTUM_CLI) \
  $(LIBQUANTUM_WALLET) \
  $(LIBQUANTUM_ZMQ)

lib_LTLIBRARIES = $(LIBQUANTUMCONSENSUS)

bin_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_QUANTUMD
  bin_PROGRAMS += quantumd
endif

if BUILD_QUANTUM_UTILS
  bin_PROGRAMS += quantum-cli quantum-tx
endif

.PHONY: FORCE check-symbols check-security
# quantum core #
QUANTUM_CORE_H = \
  addrman.h \
  amount.h \
  arith_uint256.h \
  base58.h \
  bloom.h \
  blockencodings.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  coins.h \
  compat.h \
  compat/byteswap.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/merkle.h \
  consensus/params.h \
  consensus/validation.h \
  core_io.h \
  core_memusage.h \
  hash.h \
  httprpc.h \
  httpserver.h \
  indirectmap.h \
  init.h \
  key.h \
  keystore.h \
  dbwrapper.h \
  limitedmap.h \
  main.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  net.h \
  netbase.h \
  noui.h \
  policy/fees.h \
  policy/policy.h \
  policy/rbf.h \
  pow.h \
  pos.h \
  prevector.h \
  primitives/block.h \
  primitives/transaction.h \
  protocol.h \
  pubkey.h \
  random.h \
  reverselock.h \
  rpcclient.h \
  rpcprotocol.h \
  rpcserver.h \
  register.h \
  scheduler.h \
  script/interpreter.h \
  script/script.h \
  script/script_error.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  script/ismine.h \
  serialize.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/pagelocker.h \
  sync.h \
  threadsafety.h \
  timedata.h \
  tinyformat.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  uint256.h \
  undo.h \
  util.h \
  utilmoneystr.h \
  utilstrencodings.h \
  utiltime.h \
  validationinterface.h \
  version.h \
  versionbits.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/rpcwallet.h \
  wallet/wallet.h \
  wallet/wallet_ismine.h \
  wallet/walletdb.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqconfig.h\
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  evm/libdevcore/Assertions.h \
  evm/libdevcore/Base58.h \
  evm/libdevcore/Base64.h \
  evm/libdevcore/Common.h \
  evm/libdevcore/CommonData.h \
  evm/libdevcore/CommonIO.h \
  evm/libdevcore/CommonJS.h \
  evm/libdevcore/Diff.h \
  evm/libdevcore/Exceptions.h \
  evm/libdevcore/FileSystem.h \
  evm/libdevcore/FixedHash.h \
  evm/libdevcore/Guards.h \
  evm/libdevcore/Hash.h \
  evm/libdevcore/Log.h \
  evm/libdevcore/MemoryDB.h \
  evm/libdevcore/OverlayDB.h \
  evm/libdevcore/RLP.h \
  evm/libdevcore/RangeMask.h \
  evm/libdevcore/SHA3.h \
  evm/libdevcore/Terminal.h \
  evm/libdevcore/TransientDirectory.h \
  evm/libdevcore/TrieCommon.h \
  evm/libdevcore/UndefMacros.h \
  evm/libdevcore/Worker.h \
  evm/libdevcore/concurrent_queue.h \
  evm/libdevcore/db.h \
  evm/libdevcore/debugbreak.h \
  evm/libdevcore/picosha2.h \
  evm/libdevcore/vector_ref.h \
  evm/libqtumevm/QuantumEVM.h \
  evm/libqtumevm/QuantumExtVM.h \
  evm/libethereum/Account.h \
  evm/libethereum/AccountDiff.h \
  evm/libethereum/CachedAddressState.h \
  evm/libethereum/Defaults.h \
  evm/libethereum/Executive.h \
  evm/libethereum/GasPricer.h \
  evm/libethereum/State.h \
  evm/libethcore/ABI.h \
  evm/libethcore/ChainOperationParams.h \
  evm/libethcore/Common.h \
  evm/libethcore/ICAP.h \
  evm/libethcore/Precompiled.h \
  evm/libethcore/Exeptions.h \
  evm/libdevcore/TrieDB.h \
  evm/libdevcrypto/Common.h \
  evm/libdevcrypto/libscrypt.h \
  evm/libdevcrypto/CryptoPP.h \
  evm/libdevcrypto/AES.h \
  evm/libdevcrypto/ECDHE.h \
  evm/libethcore/Transaction.h \
  evm/libethereum/Executive.h \
  evm/libethereum/Transaction.h \
  evm/libethereum/TransactionReceipt.h \
  evm/libethcore/SealEngine.h \
  evm/libethashseal/GenesisInfo.h \
  evm/libethereum/ChainParams.h
  
  
  

obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
libquantum_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between quantumd and quantum-qt
libquantum_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(QUANTUM_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libquantum_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libquantum_server_a_SOURCES = \
  addrman.cpp \
  bloom.cpp \
  blockencodings.cpp \
  chain.cpp \
  checkpoints.cpp \
  httprpc.cpp \
  httpserver.cpp \
  init.cpp \
  dbwrapper.cpp \
  main.cpp \
  merkleblock.cpp \
  miner.cpp \
  net.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/policy.cpp \
  pow.cpp \
  pos.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  script/sigcache.cpp \
  script/ismine.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  ui_interface.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  $(BITCOIN_CORE_H)

if ENABLE_ZMQ
libquantum_zmq_a_CPPFLAGS = $(QUANTUM_INCLUDES) $(ZMQ_CFLAGS)
libquantum_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libquantum_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp
endif


# wallet: shared between quantumd and quantum-qt, but only linked
# when wallet enabled
libquantum_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(QUANTUM_INCLUDES)
libquantum_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libquantum_wallet_a_SOURCES = \
  crypto/aes.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  policy/rbf.cpp \
  $(QUANTUM_CORE_H)

# crypto primitives library
crypto_libquantum_crypto_a_CPPFLAGS = $(AM_CPPFLAGS) $(QUANTUM_CONFIG_INCLUDES) -fPIC
crypto_libquantum_crypto_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libquantum_crypto_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/common.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha512.cpp \
  crypto/sha512.h \
  crypto/libscrypt/b64.c \
  crypto/libscrypt/b64.h \
  crypto/libscrypt/crypto-mcf.c \
  crypto/libscrypt/crypto_scrypt-check.c \
  crypto/libscrypt/crypto_scrypt-hash.c \
  crypto/libscrypt/crypto_scrypt-hexconvert.c \
  crypto/libscrypt/crypto_scrypt-hexconvert.h \
  crypto/libscrypt/crypto_scrypt-nosse.c \
  crypto/libscrypt/crypto-scrypt-saltgen.c \
  crypto/libscrypt/libscrypt.h \
  crypto/libscrypt/sha256.c \
  crypto/libscrypt/sha256.h \
  crypto/libscrypt/slowequals.c \
  crypto/libscrypt/slowequals.h \
  crypto/libscrypt/sysendian.h
  
  

# consensus: shared between all executables that validate any consensus rules.
libquantum_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(QUANTUM_INCLUDES)
libquantum_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libquantum_consensus_a_SOURCES = \
  amount.h \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  prevector.h \
  primitives/block.cpp \
  primitives/block.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/quantumconsensus.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
  script/standard.cpp \
  evm/libdevcore/Base58.cpp \
  evm/libdevcore/Base64.cpp \
  evm/libdevcore/Common.cpp \
  evm/libdevcore/CommonData.cpp \
  evm/libdevcore/CommonIO.cpp \
  evm/libdevcore/CommonJS.cpp \
  evm/libdevcore/FileSystem.cpp \
  evm/libdevcore/FixedHash.cpp \
  evm/libdevcore/Guards.cpp \
  evm/libdevcore/Hash.cpp \
  evm/libdevcore/Log.cpp \
  evm/libdevcore/MemoryDB.cpp \
  evm/libdevcore/OverlayDB.cpp \
  evm/libdevcore/RLP.cpp \
  evm/libdevcore/RangeMask.cpp \
  evm/libdevcore/SHA3.cpp \
  evm/libdevcore/TransientDirectory.cpp \
  evm/libdevcore/TrieCommon.cpp \
  evm/libdevcore/Worker.cpp \
  evm/libevm/ExtVMFace.cpp \
  evm/libevm/VM.cpp \
  evm/libevm/VMFactory.cpp \
  evm/libevm/VMNoInline.cpp \
  evm/libevmcore/EVMSchedule.cpp \
  evm/libevmcore/Instruction.cpp \
  evm/libqtumevm/QuantumExtVM.cpp \
  evm/libethereum/Account.cpp \
  evm/libethereum/AccountDiff.cpp \
  evm/libethereum/CachedAddressState.cpp \
  evm/libethereum/Defaults.cpp \
  evm/libethereum/Executive.cpp \
  evm/libethereum/GasPricer.cpp \
  evm/libethereum/State.cpp \
  evm/libethcore/ABI.cpp \
  evm/libethcore/ChainOperationParams.cpp \
  evm/libethcore/Common.cpp \
  evm/libethcore/ICAP.cpp \
  evm/libethcore/Precompiled.cpp \
  evm/libdevcore/TrieDB.cpp \
  evm/libdevcrypto/Common.cpp \
  evm/libdevcrypto/CryptoPP.cpp \
  evm/libdevcrypto/AES.cpp \
  evm/libdevcrypto/ECDHE.cpp \
  evm/libethcore/Transaction.cpp \
  evm/libethereum/Transaction.cpp \
  evm/libethereum/TransactionReceipt.cpp \
  evm/libethashseal/GenesisInfo.cpp \
  evm/libethereum/ChainParams.cpp \
  evm/libethcore/SealEngine.cpp \
  serialize.h \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  utilstrencodings.cpp \
  utilstrencodings.h \
  version.h

# common: shared between quantumd, and quantum-qt and non-server tools
libquantum_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(QUANTUM_INCLUDES)
libquantum_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libquantum_common_a_SOURCES = \
  amount.cpp \
  arith_uint256.cpp \
  base58.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  consensus/merkle.cpp \
  core_read.cpp \
  core_write.cpp \
  hash.cpp \
  key.cpp \
  keystore.cpp \
  netbase.cpp \
  primitives/block.cpp \
  primitives/transaction.cpp \
  protocol.cpp \
  pubkey.cpp \
  scheduler.cpp \
  script/interpreter.cpp \
  script/script.cpp \
  script/script_error.cpp \
  script/sign.cpp \
  script/standard.cpp \
  evm/libdevcore/Base58.cpp \
  evm/libdevcore/Base64.cpp \
  evm/libdevcore/Common.cpp \
  evm/libdevcore/CommonData.cpp \
  evm/libdevcore/CommonIO.cpp \
  evm/libdevcore/CommonJS.cpp \
  evm/libdevcore/FileSystem.cpp \
  evm/libdevcore/FixedHash.cpp \
  evm/libdevcore/Guards.cpp \
  evm/libdevcore/Hash.cpp \
  evm/libdevcore/Log.cpp \
  evm/libdevcore/MemoryDB.cpp \
  evm/libdevcore/OverlayDB.cpp \
  evm/libdevcore/RLP.cpp \
  evm/libdevcore/RangeMask.cpp \
  evm/libdevcore/SHA3.cpp \
  evm/libdevcore/TransientDirectory.cpp \
  evm/libdevcore/TrieCommon.cpp \
  evm/libdevcore/Worker.cpp \
  evm/libevm/ExtVMFace.cpp \
  evm/libevm/VM.cpp \
  evm/libevm/VMFactory.cpp \
  evm/libevm/VMNoInline.cpp \
  evm/libevmcore/EVMSchedule.cpp \
  evm/libevmcore/Instruction.cpp \
  evm/libqtumevm/QuantumExtVM.cpp \
  evm/libethereum/Account.cpp \
  evm/libethereum/AccountDiff.cpp \
  evm/libethereum/CachedAddressState.cpp \
  evm/libethereum/Defaults.cpp \
  evm/libethereum/Executive.cpp \
  evm/libethereum/GasPricer.cpp \
  evm/libethereum/State.cpp \
  evm/libethcore/ABI.cpp \
  evm/libethcore/ChainOperationParams.cpp \
  evm/libethcore/Common.cpp \
  evm/libethcore/ICAP.cpp \
  evm/libethcore/Precompiled.cpp \
  evm/libdevcore/TrieDB.cpp \
  evm/libdevcrypto/Common.cpp \
  evm/libdevcrypto/CryptoPP.cpp \
  evm/libdevcrypto/AES.cpp \
  evm/libdevcrypto/ECDHE.cpp \
  evm/libethcore/Transaction.cpp \
  evm/libethereum/Transaction.cpp \
  evm/libethereum/TransactionReceipt.cpp \
  evm/libethashseal/GenesisInfo.cpp \
  evm/libethereum/ChainParams.cpp \
  evm/libethcore/SealEngine.cpp \
  $(QUANTUM_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libquantum_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(QUANTUM_INCLUDES)
libquantum_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libquantum_util_a_SOURCES = \
  crypto/sha512.cpp \
  support/pagelocker.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  random.cpp \
  rpc/protocol.cpp \
  rpc/client.cpp \
  support/cleanse.cpp \
  sync.cpp \
  uint256.cpp \
  util.cpp \
  utilmoneystr.cpp \
  utilstrencodings.cpp \
  utiltime.cpp \
  $(QUANTUM_CORE_H)

if GLIBC_BACK_COMPAT
libquantum_util_a_SOURCES += compat/glibc_compat.cpp
endif

# cli: shared between quantum-cli and quantum-qt
libquantum_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(QUANTUM_INCLUDES)
libquantum_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libquantum_cli_a_SOURCES = \
  rpc/client.cpp \
  $(QUANTUM_CORE_H)

nodist_libquantum_util_a_SOURCES = $(srcdir)/obj/build.h
#

# quantumd binary #
quantumd_SOURCES = quantumd.cpp
quantumd_CPPFLAGS = $(AM_CPPFLAGS) $(QUANTUM_INCLUDES)
quantumd_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
quantumd_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
quantumd_SOURCES += quantumd-res.rc
endif

quantumd_LDADD = \
  $(LIBQUANTUM_SERVER) \
  $(LIBQUANTUM_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBQUANTUM_UTIL) \
  $(LIBQUANTUM_WALLET) \
  $(LIBQUANTUM_ZMQ) \
  $(LIBQUANTUM_CONSENSUS) \
  $(LIBQUANTUM_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

quantumd_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(CRYPTOPP_LIBS) $(ZMQ_LIBS)

# quantum-cli binary #
quantum_cli_SOURCES = quantum-cli.cpp
quantum_cli_CPPFLAGS = $(AM_CPPFLAGS) $(QUANTUM_INCLUDES) $(EVENT_CFLAGS)
quantum_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
quantum_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
quantum_cli_SOURCES += quantum-cli-res.rc
endif

quantum_cli_LDADD = \
  $(LIBQUANTUM_CLI) \
  $(LIBUNIVALUE) \
  $(LIBQUANTUM_UTIL) \
  $(LIBQUANTUM_CRYPTO)

quantum_cli_LDADD += $(BOOST_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS)
#

# quantum-tx binary #
quantum_tx_SOURCES = quantum-tx.cpp
quantum_tx_CPPFLAGS = $(AM_CPPFLAGS) $(QUANTUM_INCLUDES)
quantum_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
quantum_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
quantum_tx_SOURCES += quantum-tx-res.rc
endif

quantum_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBQUANTUM_COMMON) \
  $(LIBQUANTUM_UTIL) \
  $(LIBQUANTUM_CONSENSUS) \
  $(LIBQUANTUM_CRYPTO) \
  $(LIBSECP256K1) \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(CRYPTOPP_LIBS)

quantum_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# quantumconsensus library #
if BUILD_QUANTUM_LIBS
include_HEADERS = script/quantumconsensus.h
libquantumconsensus_la_SOURCES = $(crypto_libquantum_crypto_a_SOURCES) $(libquantum_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  libquantumconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libquantumconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libquantumconsensus_la_LIBADD = $(LIBSECP256K1) $(BOOST_LIBS) $(LIBLEVELDB) $(CRYPTOPP_LIBS)
libquantumconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_QUANTUM_INTERNAL -I$(srcdir)/evm $(LEVELDB_CPPFLAGS)
libquantumconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) $(BOOST_CPPFLAGS) -fpermissive

endif
#

CTAES_DIST =  crypto/ctaes/bench.c
CTAES_DIST += crypto/ctaes/ctaes.c
CTAES_DIST += crypto/ctaes/ctaes.h
CTAES_DIST += crypto/ctaes/README.md
CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = $(CTAES_DIST)

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

.mm.o:
	$(AM_V_CXX) $(OBJCXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  $(CPPFLAGS) $(AM_CXXFLAGS) $(QT_INCLUDES) $(AM_CXXFLAGS) $(PIE_FLAGS) $(CXXFLAGS) -c -o $@ $<

check-symbols: $(bin_PROGRAMS)
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(<D) $<

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
