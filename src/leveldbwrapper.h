// Copyright (c) 2012-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LEVELDBWRAPPER_H
#define BITCOIN_LEVELDBWRAPPER_H

#include "clientversion.h"
#include "serialize.h"
#include "streams.h"
#include "util.h"
#include "version.h"

#include <boost/filesystem/path.hpp>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

static const size_t DBWRAPPER_PREALLOC_KEY_SIZE = 64;

class leveldb_error : public std::runtime_error
{
public:
    leveldb_error(const std::string& msg) : std::runtime_error(msg) {}
};

class CLevelDBWrapper;

/** These should be considered an implementation detail of the specific database.
 */
namespace dbwrapper_private {

void HandleError(const leveldb::Status& status);

/** Work around circular dependency, as well as for testing in dbwrapper_tests.
 * Database obfuscation should be considered an implementation detail of the
 * specific database.
 */
const std::vector<unsigned char>& GetObfuscateKey(const CLevelDBWrapper &w);

}

/** Batch of changes queued to be written to a CLevelDBWrapper */
class CLevelDBBatch
{
    friend class CLevelDBWrapper;

private:
    const CLevelDBWrapper &parent;
    leveldb::WriteBatch batch;
    size_t size_estimate;

public:
    explicit CLevelDBBatch(const CLevelDBWrapper &_parent) : parent(_parent), size_estimate(0) { }

    void Clear()
    {
        batch.Clear();
        size_estimate = 0;
    }

    template <typename K, typename V>
    void Write(const K& key, const V& value)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;
        leveldb::Slice slKey(&ssKey[0], ssKey.size());

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(ssValue.GetSerializeSize(value));
        ssValue << value;
        ssValue.Xor(dbwrapper_private::GetObfuscateKey(parent));
        leveldb::Slice slValue(&ssValue[0], ssValue.size());

        batch.Put(slKey, slValue);

        // LevelDB serializes writes as:
        // - byte: header
        // - varint: key length (1 byte up to 127B, 2 bytes up to 16383B, ...)
        // - byte[]: key
        // - varint: value length
        // - byte[]: value
        // The formula below assumes the key and value are both less than 16k.
        size_estimate += 3 + (slKey.size() > 127) + slKey.size() + (slValue.size() > 127) + slValue.size();
        ssKey.clear();
        ssValue.clear();
    }

    template <typename K>
    void Erase(const K& key)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;
        leveldb::Slice slKey(&ssKey[0], ssKey.size());

        batch.Delete(slKey);
    }

    void Erase(const leveldb::Slice& slKey)
    {
        batch.Delete(slKey);
    }

    size_t SizeEstimate() const { return size_estimate; }
};

class CLevelDBIterator
{
private:
    const CLevelDBWrapper &parent;
    std::unique_ptr<leveldb::Iterator> piter;

public:
    /**
     * @param[in] _parent          Parent CLevelDBIterator instance.
     * @param[in] _piter           The original leveldb iterator.
     */
    CLevelDBIterator(const CLevelDBWrapper &_parent, leveldb::Iterator *_piter) :
        parent(_parent), piter(_piter) { }
    ~CLevelDBIterator() = default;

    bool Valid() const { return piter->Valid(); }

    void SeekToFirst() { piter->SeekToFirst(); }

    template<typename K> void Seek(const K& key) {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey(ssKey.data(), ssKey.size());
        piter->Seek(slKey);
    }

    void Next() { piter->Next(); }

    leveldb::Slice GetKey() {
        return piter->key();
    }

    bool GetKey(leveldb::Slice& key, bool fThrow = false) {
        key = GetKey();
        return true;
    }

    template<typename K> bool GetKey(K& key, bool fThrow = false) {
        leveldb::Slice slKey = piter->key();
        try {
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);
            ssKey >> key;
        } catch (const std::exception&) {
            if (fThrow)
                throw;

            return false;
        }
        return true;
    }

    leveldb::Slice GetValue() {
        return piter->value();
    }

    bool GetValue(leveldb::Slice& value, bool fThrow = false) {
        value = GetValue();
        return true;
    }

    template<typename V> bool GetValue(V& value, bool fThrow = false) {
        leveldb::Slice slValue = piter->value();
        try {
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue.Xor(dbwrapper_private::GetObfuscateKey(parent));
            ssValue >> value;
        } catch (const std::exception&) {
            if (fThrow)
                throw;

            return false;
        }
        return true;
    }

    unsigned int GetValueSize() {
        return piter->value().size();
    }
};

class CLevelDBWrapper
{
private:
    //! custom environment this database is using (may be NULL in case of default environment)
    leveldb::Env* penv;

    //! database options used
    leveldb::Options options;

    //! options used when reading from the database
    leveldb::ReadOptions readoptions;

    //! options used when iterating over values of the database
    leveldb::ReadOptions iteroptions;

    //! options used when writing to the database
    leveldb::WriteOptions writeoptions;

    //! options used when sync writing to the database
    leveldb::WriteOptions syncoptions;

    //! the database itself
    leveldb::DB* pdb;

   //! a key used for optional XOR-obfuscation of the database
   std::vector<unsigned char> obfuscate_key;

   //! the key under which the obfuscation key is stored
   static const std::string OBFUSCATE_KEY_KEY;

   //! the length of the obfuscate key in number of bytes
   static const unsigned int OBFUSCATE_KEY_NUM_BYTES;

   std::vector<unsigned char> CreateObfuscateKey() const;

public:
    CLevelDBWrapper(const boost::filesystem::path& path, size_t nCacheSize, bool fMemory = false, bool fWipe = false, bool fObfuscate = false);
    ~CLevelDBWrapper();

    template <typename K, typename V>
    bool Read(const K& key, V& value) const
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;
        leveldb::Slice slKey(&ssKey[0], ssKey.size());

        std::string strValue;
        leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
        if (!status.ok()) {
            if (status.IsNotFound())
                return false;
            LogPrintf("LevelDB read failure: %s\n", status.ToString());
            dbwrapper_private::HandleError(status);
        }
        try {
            CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue.Xor(obfuscate_key);
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    template <typename K, typename V>
    bool Write(const K& key, const V& value, bool fSync = false)
    {
        CLevelDBBatch batch(*this);
        batch.Write(key, value);
        return WriteBatch(batch, fSync);
    }

    template <typename K>
    bool Exists(const K& key) const
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;
        leveldb::Slice slKey(&ssKey[0], ssKey.size());

        std::string strValue;
        leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
        if (!status.ok()) {
            if (status.IsNotFound())
                return false;
            LogPrintf("LevelDB read failure: %s\n", status.ToString());
            dbwrapper_private::HandleError(status);
        }
        return true;
    }

    template <typename K>
    bool Erase(const K& key, bool fSync = false)
    {
        CLevelDBBatch batch(*this);
        batch.Erase(key);
        return WriteBatch(batch, fSync);
    }

    bool WriteBatch(CLevelDBBatch& batch, bool fSync = false);

    // not available for LevelDB; provide for compatibility with BDB
    bool Flush()
    {
        return true;
    }

    bool Sync()
    {
        CLevelDBBatch batch(*this);
        return WriteBatch(batch, true);
    }

    // not exactly clean encapsulation, but it's easiest for now
    std::unique_ptr<CLevelDBIterator> NewIterator() const
    {
        return std::unique_ptr<CLevelDBIterator>(new CLevelDBIterator(*this, pdb->NewIterator(iteroptions)));
    }

    /**
    * Return true if the database managed by this class contains no entries.
    */
    bool IsEmpty();

   template <typename K, typename V>
   void forEach(std::function<bool(const K& key,const V& value)> _f) const
   {
      auto itr = this->NewIterator();
      if (!itr)
         return;

      auto keepIterating = true;
      for (itr->SeekToFirst(); keepIterating && itr->Valid(); itr->Next())
      {
         K key;
         V value;
         itr->GetKey(key, true);
         itr->GetValue(value, true);
         keepIterating = _f(key, value);
      }
   }

   const std::vector<unsigned char>& GetObfuscateKey() const
   {
      return obfuscate_key;
   }

    /**
      * Compact a certain range of keys in the database.
      */
    template<typename K>
    void CompactRange(const K& key_begin, const K& key_end) const
    {
        CDataStream ssKey1(SER_DISK, CLIENT_VERSION), ssKey2(SER_DISK, CLIENT_VERSION);
        ssKey1.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey2.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey1 << key_begin;
        ssKey2 << key_end;
        leveldb::Slice slKey1(ssKey1.data(), ssKey1.size());
        leveldb::Slice slKey2(ssKey2.data(), ssKey2.size());
        pdb->CompactRange(&slKey1, &slKey2);
    }
};

#endif // BITCOIN_LEVELDBWRAPPER_H
