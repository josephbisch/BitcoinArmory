#include <limits.h>
#include <iostream>
#include <stdlib.h>
#include "gtest.h"

#include "../log.h"
#include "../BinaryData.h"
#include "../BtcUtils.h"
#include "../BlockObj.h"
#include "../StoredBlockObj.h"
#include "../lmdb_wrapper.h"
#include "../BlockUtils.h"
#include "../ScrAddrObj.h"
#include "../BtcWallet.h"
#include "../BlockDataViewer.h"
#include "../reorgTest/blkdata.h"

#ifdef _MSC_VER
   #ifdef mlock
      #undef mlock
      #undef munlock
   #endif
   #include "win32_posix.h"
    #undef close

   #ifdef _DEBUG
      //#define _CRTDBG_MAP_ALLOC
      #include <stdlib.h>
      #include <crtdbg.h>
      
      #ifndef DBG_NEW
         #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
         #define new DBG_NEW
      #endif
   #endif
#endif

#define TheBDM (*theBDM)

//#define READHEX BinaryData::createFromHex

#if ! defined(_MSC_VER) && ! defined(__MINGW32__)
/////////////////////////////////////////////////////////////////////////////
static void rmdir(string src)
{
   char* syscmd = new char[4096];
   sprintf(syscmd, "rm -rf %s", src.c_str());
   system(syscmd);
   delete[] syscmd;
}

///////////////////////////////////////////////////////////////////////////////
static void mkdir(string newdir)
{
   char* syscmd = new char[4096];
   sprintf(syscmd, "mkdir -p %s", newdir.c_str());
   system(syscmd);
   delete[] syscmd;
}
#endif

static void concatFile(const string &from, const string &to)
{
   std::ifstream i(from, ios::binary);
   std::ofstream o(to, ios::app | ios::binary);
   o << i.rdbuf();
}

static void setBlocks(const std::vector<std::string> &files, const std::string &to)
{
   std::ofstream o(to, ios::trunc | ios::binary);
   o.close();
   for (const std::string &f : files)
      concatFile("../reorgTest/blk_" + f + ".dat", to);
}
static void nullProgress(unsigned, double, unsigned, unsigned)
{

}


class NamecoinTest : public ::testing::Test
{
protected:
    virtual void SetUp(void)
    {
        rawBlock_ = READHEX(""
           // Block 19200 from https://en.bitcoin.it/wiki/Merged_mining_specification
           // Header (80 bytes in 6 fields)
           "01010100"
           "36909ac07a1673daf65fa7d828882e66c9e89f8546cdd50a9fb1000000000000"
           "0f5c6549bcd608ab7c4eac593e5bd5a73b2d432eb63518708f778fc7dcdfaf88"
           "8d1a904e"
           "69b2001b"
           "00000000"
           // Parent block coinbase tx
           "01000000"
           "01"
           "000000000000000000000000000000000000"
           "0000000000000000000000000000ffffffff"
           "35"
           "045dee091a014d522cfabe6d6dd8a7c3e01e1e95bcee015e6fcc7583a2ca60b7"
           "9e5a3aa0a171eddd344ada903d0100000000000000"
           "ffffffff"
           "01"
           "60a0102a01000000"
           "43"
           "4104f8bbe97ed2acbc5bba11c68f6f1a0313f918f3d3c0e8475055e351e3bf44"
           "2f8c8dcee682d2457bdc5351b70dd9e34026766eba18b06eaee2e102efd1ab63"
           "4667ac"
           "00000000"
           // Coinbase link
           "a903ef9de1918e4b44f6176a30c0e7c7e3439c96fb597327473d000000000000"
           "05"
           "050ac4a1a1e1bce0c48e555b1a9f935281968c72d6379b24729ca0425a3fc3cb"
           "433cd348b35ea22806cf21c7b146489aef6989551eb5ad2373ab6121060f3034"
           "1d648757c0217d43e66c57eaed64fc1820ec65d157f33b741965183a5e0c8506"
           "ac2602dfe2f547012d1cc75004d48f97aba46bd9930ff285c9f276f5bd09f356"
           "df19724579d65ec7cb62bf97946dfc6fb0e3b2839b7fdab37cdb60e55122d35b"
           "00000000"
           // Aux blockchain link
           "00"
           "00000000"
           // Parent block header
           "01000000"
           "08be13295c03e67cb70d00dae81ea06e78b9014e5ceb7d9ba504000000000000"
           "e0fd42db8ef6d783f079d126bea12e2d10c104c0927cd68f954d856f9e8111e5"
           "9a23904e"
           "5dee091a"
           "1c655086"
           // Transactions
           "01"
           "01000000"
           "01"
           "000000000000000000000000000000000000"
           "0000000000000000000000000000ffffffff"
           "08"
           "0469b2001b010152"
           "ffffffff"
           "01"
           "00f2052a01000000"
           "43"
           "410489fe91e62847575c98deeab020f65fdff17a3a870ebb05820b414f3d8097"
           "218ec9a65f1e0ae0ac35af7247bd79ed1f2a24675fffb5aa6f9620e1920ad4bf"
           "5aa6ac"
           "00000000");
    }

    BinaryData rawBlock_;
};

////////////////////////////////////////////////////////////////////////////////
TEST_F(NamecoinTest, FullBlockUnserialize)
{
    BinaryRefReader brr(rawBlock_);
    StoredHeader sbh;
    BlockHeader bh;
    sbh.unserializeFullBlock(brr, false, false);
    bh = sbh.getBlockHeaderCopy();
    
    EXPECT_EQ(65793, bh.getVersion());
    EXPECT_EQ(READHEX("3d90da4a34dded71a1a03a5a9eb760caa28375cc6f5e01eebc951e1ee0c3a7d8"), bh.getThisHash());
    EXPECT_EQ(READHEX("000000000000b19f0ad5cd46859fe8c9662e8828d8a75ff6da73167ac09a9036").swapEndian(0, 32), bh.getPrevHash());
    EXPECT_EQ(READHEX("88afdfdcc78f778f701835b62e432d3ba7d55b3e59ac4e7cab08d6bc49655c0f").swapEndian(0, 32), bh.getMerkleRoot());
    EXPECT_EQ(READHEX("1b00b269").swapEndian(0, 4), bh.getDiffBits());
    EXPECT_EQ(1318066829, bh.getTimestamp());
    EXPECT_EQ(0, bh.getNonce());
    EXPECT_EQ(1, sbh.numTx_);
    
    for(uint32_t tx=0; tx<sbh.numTx_; tx++)
    {
        StoredTx stx = sbh.stxMap_[tx];
        EXPECT_EQ(1, stx.version_);
    }
    // If the number of transactions and the tx version are both correct, we
    // probably read the NMC block correctly.
    // Though, we could test more tx data in the future.
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// THESE ARE ARMORY_DB_BARE tests.
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class BlockUtilsBare : public ::testing::Test
{
protected:
   BlockDataManager_LevelDB *theBDM;
   BlockDataViewer *theBDV;
   
   // Run this before registering a BDM.
   void regWallet(const vector<BinaryData>& scrAddrs, const string& wltName,
                  BlockDataViewer*& inBDV, BtcWallet** inWlt)
   {
      // Register the standalone address wallet. (All registrations should be
      // done before initializing the BDM. This is critical!)
      *inWlt = inBDV->registerWallet(scrAddrs, wltName, false);
   }
   
   
   /////////////////////////////////////////////////////////////////////////////
   virtual void SetUp()
   {
      LOGDISABLESTDOUT();
      magic_ = READHEX(NAMECOIN_MAGIC_BYTES);
      ghash_ = READHEX(MAINNET_GENESIS_HASH_HEX);
      gentx_ = READHEX(MAINNET_GENESIS_TX_HASH_HEX);
      zeros_ = READHEX("00000000");
      
      blkdir_ = string("./blkfiletest");
      homedir_ = string("./fakehomedir");
      ldbdir_ = string("./ldbtestdir");
      
      
      mkdir(blkdir_);
      mkdir(homedir_);
      
      // Put the first 5 blocks into the blkdir
      blk0dat_ = BtcUtils::getBlkFilename(blkdir_, 0);
      setBlocks({"0", "1", "2", "3", "4", "5"}, blk0dat_);
      
      config.armoryDbType = ARMORY_DB_BARE;
      config.pruneType = DB_PRUNE_NONE;
      config.blkFileLocation = blkdir_;
      config.levelDBLocation = ldbdir_;
      
      config.genesisBlockHash = ghash_;
      config.genesisTxHash = gentx_;
      config.magicBytes = magic_;
      
      theBDM = new BlockDataManager_LevelDB(config);
      theBDM->openDatabase();
      iface_ = theBDM->getIFace();
      
      theBDV = new BlockDataViewer(theBDM);
   }
   

   /////////////////////////////////////////////////////////////////////////////
   virtual void TearDown(void)
   {
      delete theBDM;
      delete theBDV;
      
      theBDM = nullptr;
      theBDV = nullptr;
      
      rmdir(blkdir_);
      rmdir(homedir_);
      
      #ifdef _MSC_VER
         rmdir("./ldbtestdir");
         mkdir("./ldbtestdir");
      #else
         string delstr = ldbdir_ + "/*";
         rmdir(delstr);
      #endif
      LOGENABLESTDOUT();
      CLEANUP_ALL_TIMERS();
   }
   
   BlockDataManagerConfig config;
   
   LMDBBlockDatabase* iface_;
   BinaryData magic_;
   BinaryData ghash_;
   BinaryData gentx_;
   BinaryData zeros_;
   
   string blkdir_;
   string homedir_;
   string ldbdir_;
   string blk0dat_;
};

TEST_F(BlockUtilsBare, Load5Blocks)
{
   vector<BinaryData> scrAddrVec;
   scrAddrVec.push_back(TestChain::scrAddrA);
   scrAddrVec.push_back(TestChain::scrAddrB);
   scrAddrVec.push_back(TestChain::scrAddrC);
   scrAddrVec.push_back(TestChain::scrAddrD);
   scrAddrVec.push_back(TestChain::scrAddrE);
   scrAddrVec.push_back(TestChain::scrAddrF);
   BtcWallet* wlt;
   regWallet(scrAddrVec, "wallet1", theBDV, &wlt);

   TheBDM.doInitialSyncOnLoad(nullProgress);
   theBDV->scanWallets();

   const ScrAddrObj* scrObj;
   scrObj = wlt->getScrAddrObjByKey(TestChain::scrAddrA);
   EXPECT_EQ(scrObj->getFullBalance(), 50*COIN);
   scrObj = wlt->getScrAddrObjByKey(TestChain::scrAddrB);
   EXPECT_EQ(scrObj->getFullBalance(), 70*COIN);
   scrObj = wlt->getScrAddrObjByKey(TestChain::scrAddrC);
   EXPECT_EQ(scrObj->getFullBalance(), 20*COIN);
   scrObj = wlt->getScrAddrObjByKey(TestChain::scrAddrD);
   EXPECT_EQ(scrObj->getFullBalance(), 65*COIN);
   scrObj = wlt->getScrAddrObjByKey(TestChain::scrAddrE);
   EXPECT_EQ(scrObj->getFullBalance(), 30*COIN);
   scrObj = wlt->getScrAddrObjByKey(TestChain::scrAddrF);
   EXPECT_EQ(scrObj->getFullBalance(), 5*COIN);
 
   EXPECT_EQ(wlt->getFullBalance(), 240*COIN);
}


////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//// Now actually execute all the tests
//////////////////////////////////////////////////////////////////////////////////
GTEST_API_ int main(int argc, char **argv)
{
   #ifdef _MSC_VER
      _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
   #endif

   std::cout << "Running main() from gtest_main.cc\n";

   // Setup the log file 
   STARTLOGGING("namecoinTestsLog.txt", LogLvlDebug2);
   //LOGDISABLESTDOUT();

   testing::InitGoogleTest(&argc, argv);
   int exitCode = RUN_ALL_TESTS();

   FLUSHLOG();
   CLEANUPLOG();

   return exitCode;
}                                      
