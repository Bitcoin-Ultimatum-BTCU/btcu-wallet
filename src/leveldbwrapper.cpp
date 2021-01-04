// Copyright (c) 2012-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "leveldbwrapper.h"

#include "util.h"
#include "random.h"

#include <boost/filesystem.hpp>

#include <leveldb/cache.h>
#include <leveldb/env.h>
#include <leveldb/filter_policy.h>
#include <memenv.h>
#include "utilstrencodings.h"

namespace dbwrapper_private {

void HandleError(const leveldb::Status& status)
{
    if (status.ok())
        return;
    LogPrintf("%s\n", status.ToString());
    if (status.IsCorruption())
        throw leveldb_error("Database corrupted");
    if (status.IsIOError())
        throw leveldb_error("Database I/O error");
    if (status.IsNotFound())
        throw leveldb_error("Database entry missing");
    throw leveldb_error("Unknown database error");
}

const std::vector<unsigned char>& GetObfuscateKey(const CLevelDBWrapper &w)
{
    return w.GetObfuscateKey();
}

}

static leveldb::Options GetOptions(size_t nCacheSize)
{
    leveldb::Options options;
    options.block_cache = leveldb::NewLRUCache(nCacheSize / 2);
    options.write_buffer_size = nCacheSize / 4; // up to two write buffers may be held in memory simultaneously
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.compression = leveldb::kNoCompression;
    options.max_open_files = 64;
    if (leveldb::kMajorVersion > 1 || (leveldb::kMajorVersion == 1 && leveldb::kMinorVersion >= 16)) {
        // LevelDB versions before 1.16 consider short writes to be corruption. Only trigger error
        // on corruption in later versions.
        options.paranoid_checks = true;
    }
    return options;
}

// Prefixed with null character to avoid collisions with other keys
//
// We must use a string constructor which specifies length so that we copy
// past the null-terminator.
const std::string CLevelDBWrapper::OBFUSCATE_KEY_KEY("\000obfuscate_key", 14);

const unsigned int CLevelDBWrapper::OBFUSCATE_KEY_NUM_BYTES = 8;

/**
 * Returns a string (consisting of 8 random bytes) suitable for use as an
 * obfuscating XOR key.
 */
std::vector<unsigned char> CLevelDBWrapper::CreateObfuscateKey() const
{
   unsigned char buff[OBFUSCATE_KEY_NUM_BYTES];
   GetRandBytes(buff, OBFUSCATE_KEY_NUM_BYTES);
   return std::vector<unsigned char>(&buff[0], &buff[OBFUSCATE_KEY_NUM_BYTES]);

}
bool CLevelDBWrapper::IsEmpty()
{
   auto it = NewIterator();
   it->SeekToFirst();
   return !(it->Valid());
}

CLevelDBWrapper::CLevelDBWrapper(const boost::filesystem::path& path, size_t nCacheSize, bool fMemory, bool fWipe, bool fObfuscate)
{
    penv = NULL;
    readoptions.verify_checksums = true;
    iteroptions.verify_checksums = true;
    iteroptions.fill_cache = false;
    syncoptions.sync = true;
    options = GetOptions(nCacheSize);
    options.create_if_missing = true;

    if (fMemory) {
        penv = leveldb::NewMemEnv(leveldb::Env::Default());
        options.env = penv;
    } else {
        if (fWipe) {
            LogPrintf("Wiping LevelDB in %s\n", path.string());
            leveldb::DestroyDB(path.string(), options);
        }
        TryCreateDirectory(path);
        LogPrintf("Opening LevelDB in %s\n", path.string());
    }
    leveldb::Status status = leveldb::DB::Open(options, path.string(), &pdb);
    dbwrapper_private::HandleError(status);
    LogPrintf("Opened LevelDB successfully\n");

   // In the original bitcoin there is initializing with '\000' the OBFUSCATE_KEY_NUM_BYTES times
   //   when in BTCU we don't use any obfuscate_key
   // obfuscate_key = std::vector<unsigned char>(OBFUSCATE_KEY_NUM_BYTES, '\000');

   bool key_exists = Read(OBFUSCATE_KEY_KEY, obfuscate_key);

   if (!key_exists && fObfuscate && IsEmpty()) {
      // Initialize non-degenerate obfuscation if it won't upset
      // existing, non-obfuscated data.
      std::vector<unsigned char> new_key = CreateObfuscateKey();

      // Write `new_key` so we don't obfuscate the key with itself
      Write(OBFUSCATE_KEY_KEY, new_key);
      obfuscate_key = new_key;

      LogPrintf("Wrote new obfuscate key for %s: %s\n", path.string(), HexStr(obfuscate_key));
   }

   if (!obfuscate_key.empty()) {
      LogPrintf("Using obfuscation key for %s: %s\n", path.string(), HexStr(obfuscate_key));
   } else {
      LogPrintf("No obfuscation key for %s\n", path.string());
   }
}

CLevelDBWrapper::~CLevelDBWrapper()
{
    delete pdb;
    pdb = NULL;
    delete options.filter_policy;
    options.filter_policy = NULL;
    delete options.block_cache;
    options.block_cache = NULL;
    delete penv;
    options.env = NULL;
}

bool CLevelDBWrapper::WriteBatch(CLevelDBBatch& batch, bool fSync)
{
    leveldb::Status status = pdb->Write(fSync ? syncoptions : writeoptions, &batch.batch);
    dbwrapper_private::HandleError(status);
    return true;
}
