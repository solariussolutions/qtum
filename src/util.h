// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Server/client environment: argument handling, config file parsing,
 * logging, thread wrappers
 */
#ifndef QUANTUM_UTIL_H
#define QUANTUM_UTIL_H

#if defined(HAVE_CONFIG_H)
#include "config/quantum-config.h"
#endif

#include "compat.h"
#include "tinyformat.h"
#include "utiltime.h"

#include <atomic>
#include <exception>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/thread/exceptions.hpp>

#ifndef WIN32
#include <signal.h>
//begin modif qtum
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
//end modif qtum
#endif
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp> // for startswith() and endswith()
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options/detail/config_file.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/thread.hpp>
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/conf.h>

static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS        = false;
static const bool DEFAULT_LOGTIMESTAMPS = true;

/** Signals for translation. */
class CTranslationInterface
{
public:
    /** Translate a message to the native language of the user. */
    boost::signals2::signal<std::string (const char* psz)> Translate;
};

extern std::map<std::string, std::string> mapArgs;
extern std::map<std::string, std::vector<std::string> > mapMultiArgs;
extern bool fDebug;
extern bool fPrintToConsole;
extern bool fPrintToDebugLog;
extern bool fServer;
extern std::string strMiscWarning;
extern bool fLogTimestamps;
extern bool fLogTimeMicros;
extern bool fLogIPs;
extern std::atomic<bool> fReopenDebugLog;
extern CTranslationInterface translationInterface;

extern const char * const QUANTUM_CONF_FILENAME;
extern const char * const QUANTUM_PID_FILENAME;

////////////////////////////////////////////////////////////////////////////////////// // TODO temp qtumLog
enum LogType{ DEBUG_LOG, VM_LOG };
//////////////////////////////////////////////////////////////////////////////////////

/**
 * Translation function: Call Translate signal on UI interface, which returns a boost::optional result.
 * If no translation slot is registered, nothing is returned, and simply return the input.
 */
inline std::string _(const char* psz)
{
    boost::optional<std::string> rv = translationInterface.Translate(psz);
    return rv ? (*rv) : psz;
}

void SetupEnvironment();
bool SetupNetworking();

/** Return true if log accepts specified category */
bool LogAcceptCategory(const char* category);
/** Send a string to the log output */
// int LogPrintStr(const std::string &str);
int LogPrintStr(const std::string &str, LogType type = DEBUG_LOG); // TODO temp qtumLog

#define LogPrintf(...) LogPrint(NULL, __VA_ARGS__)
#define LogPrintfVM(...) LogPrintVM(NULL, __VA_ARGS__) // TODO temp qtumLog

template<typename T1, typename... Args>
static inline int LogPrint(const char* category, const char* fmt, const T1& v1, const Args&... args)
{
    if(!LogAcceptCategory(category)) return 0;                            \
    return LogPrintStr(tfm::format(fmt, v1, args...));
}

template<typename T1, typename... Args>
bool error(const char* fmt, const T1& v1, const Args&... args)
{
    LogPrintStr("ERROR: " + tfm::format(fmt, v1, args...) + "\n");
    return false;
}

/**
 * Zero-arg versions of logging and error, these are not covered by
 * the variadic templates above (and don't take format arguments but
 * bare strings).
 */
static inline int LogPrint(const char* category, const char* s)
{
    if(!LogAcceptCategory(category)) return 0;
    return LogPrintStr(s);
}

////////////////////////////////////////////////////////////////////////////////////// // TODO temp qtumLog
template<typename T1, typename... Args>
static inline int LogPrintVM(const char* category, const char* fmt, const T1& v1, const Args&... args)
{
    if(!LogAcceptCategory(category)) return 0;                            \
    return LogPrintStr(tfm::format(fmt, v1, args...), LogType::VM_LOG);
}

static inline int LogPrintVM(const char* category, const char* s)
{
    if(!LogAcceptCategory(category)) return 0;
    return LogPrintStr(s, LogType::VM_LOG);
}
//////////////////////////////////////////////////////////////////////////////////////

static inline bool error(const char* s)
{
    LogPrintStr(std::string("ERROR: ") + s + "\n");
    return false;
}

void PrintExceptionContinue(const std::exception *pex, const char* pszThread);
void ParseParameters(int argc, const char*const argv[]);
void FileCommit(FILE *fileout);
bool TruncateFile(FILE *file, unsigned int length);
int RaiseFileDescriptorLimit(int nMinFD);
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length);
bool RenameOver(boost::filesystem::path src, boost::filesystem::path dest);
bool TryCreateDirectory(const boost::filesystem::path& p);
boost::filesystem::path GetDefaultDataDir();
const boost::filesystem::path &GetDataDir(bool fNetSpecific = true);
void ClearDatadirCache();
boost::filesystem::path GetConfigFile();
#ifndef WIN32
boost::filesystem::path GetPidFile();
void CreatePidFile(const boost::filesystem::path &path, pid_t pid);
#endif
void ReadConfigFile(std::map<std::string, std::string>& mapSettingsRet, std::map<std::string, std::vector<std::string> >& mapMultiSettingsRet);
#ifdef WIN32
boost::filesystem::path GetSpecialFolderPath(int nFolder, bool fCreate = true);
#endif
void OpenDebugLog();
void ShrinkDebugFile();
void runCommand(const std::string& strCommand);

inline bool IsSwitchChar(char c)
{
#ifdef WIN32
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}

/**
 * Return string argument or default value
 *
 * @param strArg Argument to get (e.g. "-foo")
 * @param default (e.g. "1")
 * @return command-line argument or default value
 */
std::string GetArg(const std::string& strArg, const std::string& strDefault);

/**
 * Return integer argument or default value
 *
 * @param strArg Argument to get (e.g. "-foo")
 * @param default (e.g. 1)
 * @return command-line argument (0 if invalid number) or default value
 */
int64_t GetArg(const std::string& strArg, int64_t nDefault);

/**
 * Return boolean argument or default value
 *
 * @param strArg Argument to get (e.g. "-foo")
 * @param default (true or false)
 * @return command-line argument or default value
 */
bool GetBoolArg(const std::string& strArg, bool fDefault);

/**
 * Set an argument if it doesn't already have a value
 *
 * @param strArg Argument to set (e.g. "-foo")
 * @param strValue Value (e.g. "1")
 * @return true if argument gets set, false if it already had a value
 */
bool SoftSetArg(const std::string& strArg, const std::string& strValue);

/**
 * Set a boolean argument if it doesn't already have a value
 *
 * @param strArg Argument to set (e.g. "-foo")
 * @param fValue Value (e.g. false)
 * @return true if argument gets set, false if it already had a value
 */
bool SoftSetBoolArg(const std::string& strArg, bool fValue);

/**
 * Format a string to be used as group of options in help messages
 *
 * @param message Group name (e.g. "RPC server options:")
 * @return the formatted string
 */
std::string HelpMessageGroup(const std::string& message);

/**
 * Format a string to be used as option description in help messages
 *
 * @param option Option message (e.g. "-rpcuser=<user>")
 * @param message Option description (e.g. "Username for JSON-RPC connections")
 * @return the formatted string
 */
std::string HelpMessageOpt(const std::string& option, const std::string& message);

/**
 * Return the number of physical cores available on the current system.
 * @note This does not count virtual cores, such as those provided by HyperThreading
 * when boost is newer than 1.56.
 */
int GetNumCores();

//begin modif qtum
#ifdef WIN32
inline void SetThreadPriority(int nPriority)
{
    SetThreadPriority(GetCurrentThread(), nPriority);
}
#else

#define THREAD_PRIORITY_LOWEST          PRIO_MAX
#define THREAD_PRIORITY_BELOW_NORMAL    2
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_ABOVE_NORMAL    0

inline void SetThreadPriority(int nPriority)
{
    // It's unclear if it's even possible to change thread priorities on Linux,
    // but we really and truly need it for the generation threads.
#ifdef PRIO_THREAD
    setpriority(PRIO_THREAD, 0, nPriority);
#else
    setpriority(PRIO_PROCESS, 0, nPriority);
#endif
}
#endif
//end modif qtum

void RenameThread(const char* name);

/**
 * .. and a wrapper that just calls func once
 */
template <typename Callable> void TraceThread(const char* name,  Callable func)
{
    std::string s = strprintf("quantum-%s", name);
    RenameThread(s.c_str());
    try
    {
        LogPrintf("%s thread start\n", name);
        func();
        LogPrintf("%s thread exit\n", name);
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("%s thread interrupt\n", name);
        throw;
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, name);
        throw;
    }
    catch (...) {
        PrintExceptionContinue(NULL, name);
        throw;
    }
}

std::string CopyrightHolders(const std::string& strPrefix);

#endif // QUANTUM_UTIL_H
