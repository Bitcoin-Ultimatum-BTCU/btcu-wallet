#ifndef QTUMTRANSACTION_H
#define QTUMTRANSACTION_H

#include <libethereum/Transaction.h>

#        if defined(_MSC_VER)
/*           __pragma() is specified starting from Visual Studio 2008*/
#            if (_MSC_VER < 1500)
#                error "Unsupport Visual C compiler version. Minimum version is Visual Studio 2008."
#            endif
#            define ATTRIBUTE_PACKED
/*           Enable packing and supress warning C4103: Packing was changed after the inclusion of the header, probably missing #pragma pop */
#            define BEGIN_ATTRIBUTE_PACKED __pragma(pack(push,1)) \
                                           __pragma(warning(disable : 4103))
/*           Disable packing and enable warning C4103 back */
#            define END_ATTRIBUTE_PACKED   __pragma(pack(pop)) \
                                          __pragma(warning(default : 4103))
#            define ATTRIBUTE_SECTION_GCC(x)
#        endif /* defined(_MSC_VER) */

#ifdef _MSC_VER
BEGIN_ATTRIBUTE_PACKED

struct VersionVM ATTRIBUTE_PACKED {
#else
struct __attribute__((may_alias)) VersionVM {
#endif
    //this should be portable, see https://stackoverflow.com/questions/31726191/is-there-a-portable-alternative-to-c-bitfields
# if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t format : 2;
    uint8_t rootVM : 6;
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8_t rootVM : 6;
    uint8_t format : 2;
#endif
    uint8_t vmVersion;
    uint16_t flagOptions;
    // CONSENSUS CRITICAL!
    // Do not add any other fields to this struct

    uint32_t toRaw(){
        return *(uint32_t*)this;
    }
    static VersionVM fromRaw(uint32_t val){
        VersionVM x = *(VersionVM*)&val;
        return x;
    }
    static VersionVM GetNoExec(){
        VersionVM x;
        x.flagOptions=0;
        x.rootVM=0;
        x.format=0;
        x.vmVersion=0;
        return x;
    }
    static VersionVM GetEVMDefault(){
        VersionVM x;
        x.flagOptions=0;
        x.rootVM=1;
        x.format=0;
        x.vmVersion=0;
        return x;
    }
#ifdef _MSC_VER
} END_ATTRIBUTE_PACKED;
#else
} __attribute__((__packed__));
#endif

class QtumTransaction : public dev::eth::Transaction{

public:

    QtumTransaction() : nVout(0), hasRefundSender(false) {}

    QtumTransaction(dev::u256 const& _value, dev::u256 const& _gasPrice, dev::u256 const& _gas, dev::bytes const& _data, dev::u256 const& _nonce = dev::Invalid256):
        dev::eth::Transaction(_value, _gasPrice, _gas, _data, _nonce) , nVout(0), hasRefundSender(false) {}

    QtumTransaction(dev::u256 const& _value, dev::u256 const& _gasPrice, dev::u256 const& _gas, dev::Address const& _dest, dev::bytes const& _data, dev::u256 const& _nonce = dev::Invalid256):
        dev::eth::Transaction(_value, _gasPrice, _gas, _dest, _data, _nonce) , nVout(0), hasRefundSender(false) {}

    void setHashWith(const dev::h256 hash) { m_hashWith = hash; }

    dev::h256 getHashWith() const { return m_hashWith; }

    void setNVout(uint32_t vout) { nVout = vout; }

    uint32_t getNVout() const { return nVout; }

    void setVersion(VersionVM v){
        version=v;
    }
    VersionVM getVersion() const{
        return version;
    }

    void setRefundSender(const dev::Address _refundSender) { refundSender = _refundSender; hasRefundSender = true;}

    dev::Address getRefundSender() const { return hasRefundSender ? refundSender : sender();}

private:

    uint32_t nVout;
    VersionVM version;
    dev::Address refundSender;
    bool hasRefundSender;
};
#endif
