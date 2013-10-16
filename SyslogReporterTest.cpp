/* 
 * File:   SyslogReporterTest.cpp
 * Author: John Gress
 * 
 */
#include <iostream>
#include "SendDpiMsgLRZMQ.h"
#include "SyslogReporterTest.h"
#include "MockSyslogReporter.h"
//#include "MockConfSlave.h"
//#include "DpiMsgLR.h"
//#include "luajit-2.0/lua.hpp"

void ZeroCopyDelete(void*, void* data) {
   std::string* theString = reinterpret_cast<std::string*> (data);
   delete theString;
}

void SyslogReporterTest::ShootZeroCopySyslogThread(int numberOfMessages,
        std::string& location, std::string& exampleSyslog) {
   Rifle rifle(location);
   rifle.SetHighWater(500);
   rifle.SetIOThreads(1);
   rifle.SetOwnSocket(false);
   rifle.Aim();
   std::string* syslogMsg = new std::string(exampleSyslog);
   for (int i = 0; i < numberOfMessages && !zctx_interrupted; i++) {
      std::string* syslogMsg = new std::string(exampleSyslog);
      ASSERT_TRUE(rifle.FireZeroCopy(syslogMsg, syslogMsg->size(), ZeroCopyDelete, 10000));
   }
   while (!zctx_interrupted) {
      boost::this_thread::sleep(boost::posix_time::milliseconds(10));
   }
}


TEST_F(SyslogReporterTest, InstantiateSyslogReporter) {
   MockSyslogReporter syslogReporter(mConfSlave, syslogName, syslogOption, 
         syslogFacility, syslogPriority);
   EXPECT_EQ(syslogName, syslogOpenIdent);
   EXPECT_EQ(syslogOption, syslogOpenOption);
   EXPECT_EQ(syslogFacility, syslogOpenFacility);
}

TEST_F(SyslogReporterTest, NewDeleteSyslogReporter) {
   MockSyslogReporter* psr = new MockSyslogReporter(mConfSlave, syslogName,
         syslogOption, syslogFacility, syslogPriority);
   delete psr;
}

TEST_F(SyslogReporterTest, StartStopSyslogReporter) {
   MockSyslogReporter syslogReporter(mConfSlave, syslogName, syslogOption, 
         syslogFacility, syslogPriority);
   boost::thread* srThread = syslogReporter.Start(); 
   boost::this_thread::sleep(boost::posix_time::milliseconds(100));
   EXPECT_NE(syslogReporter.GetThreadId(), 0);
   syslogReporter.Join();
}

TEST_F(SyslogReporterTest, SendMsgToSyslogReporter) {
   MockSyslogReporter syslogReporter(mConfSlave, syslogName, syslogOption, 
         syslogFacility, syslogPriority);
   boost::thread* srThread = syslogReporter.Start(); 
   boost::this_thread::sleep(boost::posix_time::milliseconds(500));

   std::string queueName = mConfSlave.GetConf().getSyslogQueue();

   int numSyslogSenders = 4;
   int numSyslogMsgs = 10;
   std::vector<boost::thread*> theSyslogSenders;
   std::string syslogMsg(" EVT:001 49f7c7f9-73c1-4c6f-9585-fca792087297:00 10.128.20.60,10.128.64.251,58177,53,f0:1f:af:0b:dc:2d,f0:f7:55:dc:a8:7f,17,793,98/98,172/172,1/1,1381436188,1381436247,59/59,process=dns");

   for (int i = 0; i < numSyslogSenders && !zctx_interrupted; i++) {
      std::stringstream ssSyslog;
      ssSyslog << i << syslogMsg;
      boost::thread* aSender = new boost::thread(
              &SyslogReporterTest::ShootZeroCopySyslogThread, this, 
              numSyslogMsgs, queueName, ssSyslog.str());
      theSyslogSenders.push_back(aSender);
   }

   boost::this_thread::sleep(boost::posix_time::seconds(1));

   for (auto it = theSyslogSenders.begin();
           it != theSyslogSenders.end() && !zctx_interrupted; it++) {
      (*it)->interrupt();
      (*it)->join();
      delete *it;
   }

   EXPECT_EQ(numSyslogSenders*numSyslogMsgs, syslogOutput.size());
   for ( auto syslogIt : syslogOutput ) {
      EXPECT_NE(std::string::npos, syslogIt.find(syslogMsg));
   }
   syslogReporter.Join();
}

TEST_F(SyslogReporterTest, SendMsgToSyslogReporterSyslogDisabled) {
   MockSyslogReporter syslogReporter(mConfSlave, syslogName, syslogOption, 
         syslogFacility, syslogPriority);
   syslogReporter.SetSyslogEnabled(false);
   boost::thread* srThread = syslogReporter.Start(); 
   boost::this_thread::sleep(boost::posix_time::milliseconds(500));
   EXPECT_NE(syslogReporter.GetThreadId(), 0);

   std::string queueName = mConfSlave.GetConf().getSyslogQueue();

   int numSyslogSenders = 1;
   int numSyslogMsgs = 10;
   std::vector<boost::thread*> theSyslogSenders;
   std::string syslogMsg(" EVT:001 49f7c7f9-73c1-4c6f-9585-fca792087297:00 10.128.20.60,10.128.64.251,58177,53,f0:1f:af:0b:dc:2d,f0:f7:55:dc:a8:7f,17,793,98/98,172/172,1/1,1381436188,1381436247,59/59,process=dns");

   for (int i = 0; i < numSyslogSenders && !zctx_interrupted; i++) {
      std::stringstream ssSyslog;
      ssSyslog << i << syslogMsg;
      boost::thread* aSender = new boost::thread(
              &SyslogReporterTest::ShootZeroCopySyslogThread, this, 
              numSyslogMsgs, queueName, ssSyslog.str());
      theSyslogSenders.push_back(aSender);
   }

   boost::this_thread::sleep(boost::posix_time::seconds(1));

   for (auto it = theSyslogSenders.begin();
           it != theSyslogSenders.end() && !zctx_interrupted; it++) {
      (*it)->interrupt();
      (*it)->join();
      delete *it;
   }

   EXPECT_EQ(0, syslogOutput.size());
   syslogReporter.Join();
}

TEST_F(SyslogReporterTest, SyslogSendStat) {
   MockSyslogReporter syslogReporter(mConfSlave, syslogName, syslogOption, 
         syslogFacility, syslogPriority);
   syslogReporter.SetMockTime( 10, 0 );
   boost::thread* srThread = syslogReporter.Start(); 
   boost::this_thread::sleep(boost::posix_time::milliseconds(500));
   EXPECT_NE(syslogReporter.GetThreadId(), 0);

   std::string queueName = mConfSlave.GetConf().getSyslogQueue();
   int numSyslogMsgs = 10;

   Rifle rifle(queueName);
   rifle.SetHighWater(500);
   rifle.SetIOThreads(1);
   rifle.SetOwnSocket(false);
   rifle.Aim();

   std::string* pMsg = NULL;
   std::string syslogMsg(" EVT:001 49f7c7f9-73c1-4c6f-9585-fca792087297:00 10.128.20.60,10.128.64.251,58177,53,f0:1f:af:0b:dc:2d,f0:f7:55:dc:a8:7f,17,793,98/98,172/172,1/1,1381436188,1381436247,59/59,process=dns");
   for (int i = 0; i < numSyslogMsgs && !zctx_interrupted; i++) {
      pMsg = new std::string(syslogMsg);
      ASSERT_TRUE(rifle.FireZeroCopy(pMsg, pMsg->size(), ZeroCopyDelete, 10));
   }
   boost::this_thread::sleep(boost::posix_time::milliseconds(500));
   EXPECT_EQ(0, syslogReporter.GetStatCount());
   syslogReporter.SetMockTime( 15, 0 );
   boost::this_thread::sleep(boost::posix_time::milliseconds(600));
   EXPECT_EQ(numSyslogMsgs, syslogReporter.GetStatCount());
   
   syslogReporter.SetMockTime( 16, 0 );
   boost::this_thread::sleep(boost::posix_time::milliseconds(600));
   EXPECT_EQ(0, syslogReporter.GetStatCount());

   numSyslogMsgs = 5;
   for (int i = 0; i < numSyslogMsgs && !zctx_interrupted; i++) {
      pMsg = new std::string(syslogMsg);
      ASSERT_TRUE(rifle.FireZeroCopy(pMsg, pMsg->size(), ZeroCopyDelete, 10));
   }
   boost::this_thread::sleep(boost::posix_time::milliseconds(500));
   EXPECT_EQ(0, syslogReporter.GetStatCount());
   syslogReporter.SetMockTime( 17, 0 );
   boost::this_thread::sleep(boost::posix_time::milliseconds(600));
   EXPECT_EQ(numSyslogMsgs, syslogReporter.GetStatCount());

   syslogReporter.Join();
}

TEST_F(SyslogReporterTest, SendStatTime) {
   MockSyslogReporter syslogReporter(mConfSlave, syslogName, syslogOption, 
         syslogFacility, syslogPriority);

   syslogReporter.SetMockTime( 1, 0 );
   EXPECT_TRUE(syslogReporter.IsSendStatTime());
   EXPECT_FALSE(syslogReporter.IsSendStatTime());
   syslogReporter.SetMockTime( 1, 999999 );
   EXPECT_FALSE(syslogReporter.IsSendStatTime());
   syslogReporter.SetMockTime( 2, 0 );
   EXPECT_TRUE(syslogReporter.IsSendStatTime());

}
