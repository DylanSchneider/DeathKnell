#pragma once
#include "DiskCleanup.h"
#include <sys/statvfs.h>
#include <MockDiskUsage.h>

class MockDiskCleanup : public DiskCleanup {
public:

   MockDiskCleanup(networkMonitor::ConfSlave& conf) : DiskCleanup(conf), mFailRemoveSearch(false),
   mFailFileSystemInfo(false), mFileSystemInfoCountdown(0), mSucceedRemoveSearch(false),
   mRealFilesSystemAccess(false) {
      mFleSystemInfo.f_bfree = 1;
      mFleSystemInfo.f_frsize = 1;
      mFleSystemInfo.f_blocks = 1;
      mFleSystemInfo.f_frsize = 1;
   }

   virtual ~MockDiskCleanup() {
   }

   bool IsShutdown() {
      return false;
   }
   size_t RemoveOldestPCapFiles(const size_t maxToRemove, ElasticSearch& es, size_t& filesRemoved, size_t& spaceRemoved) {
      return DiskCleanup::RemoveOldestPCapFiles(1, es, filesRemoved, spaceRemoved);
   }

   bool TooMuchPCap(std::atomic<size_t>& aDiskUsed, std::atomic<size_t>& aTotalFiles) {
      return DiskCleanup::TooMuchPCap(aDiskUsed, aTotalFiles);
   }

   void RecalculatePCapDiskUsed(std::atomic<size_t>& aDiskUsed, std::atomic<size_t>& aTotalFiles, DiskSpace& pcapDiskInGB) {
      DiskCleanup::RecalculatePCapDiskUsed(aDiskUsed, aTotalFiles, pcapDiskInGB);
   }

   void CleanupOldPcapFiles(bool canSendStats, PacketCaptureFilesystemDetails& previous, ElasticSearch& es, SendStats& sendQueue,
           std::time_t& currentTime, std::atomic<size_t>& aDiskUsed,
           std::atomic<size_t>& aTotalFiles,
        const DiskSpace& probeDiskInGB,
        DiskSpace& pcapDiskInGB) {
      DiskCleanup::CleanupOldPcapFiles(canSendStats, previous, es, sendQueue, currentTime, aDiskUsed, aTotalFiles, probeDiskInGB, pcapDiskInGB);
   }

   bool TooMuchSearch(const size_t& fsFreeGigs, const size_t& fsTotalGigs) {
      return DiskCleanup::TooMuchSearch(fsFreeGigs, fsTotalGigs);
   }

   void ResetConf() {
      DiskCleanup::ResetConf();
   }

   bool RemoveOldestSearchIndex(ElasticSearch& es) {
      if (mSucceedRemoveSearch) {
         return true;
      }
      if (!mFailRemoveSearch) {
         return DiskCleanup::RemoveOldestSearchIndex(es);
      }


      return false;
   }

   void GetPcapStoreUsage(DiskSpace& pcapDiskInGB, 
                          const DiskUsage::Size size) {
         if (mRealFilesSystemAccess) {
            DiskCleanup::GetPcapStoreUsage(pcapDiskInGB, size);
         }  else {
            struct statvfs mockStatvs;
            mockStatvs.f_bsize = mFleSystemInfo.f_bsize;
            mockStatvs.f_frsize = mFleSystemInfo.f_frsize;
            mockStatvs.f_blocks = mFleSystemInfo.f_blocks;
            mockStatvs.f_bfree = mFleSystemInfo.f_bfree;
            mockStatvs.f_bavail = 1;
            mockStatvs.f_files = 1;
            mockStatvs.f_ffree = 1;
            mockStatvs.f_favail = 1;
            MockDiskUsage disk(mockStatvs);

            disk.Update();
            pcapDiskInGB.Free = disk.DiskFree(size);
            pcapDiskInGB.Total = disk.DiskTotal(size); 
            pcapDiskInGB.Used = disk.DiskUsed(size); 
         }
   }

   void GetProbeFileSystemInfo(DiskSpace& probeDiskInGB, 
                               const DiskUsage::Size size) {
      if (!mFailFileSystemInfo) {
         if (mRealFilesSystemAccess) {
            DiskCleanup::GetProbeFileSystemInfo(probeDiskInGB,size);
         }  else {
            struct statvfs mockStatvs;
            mockStatvs.f_bsize = mFleSystemInfo.f_bsize;
            mockStatvs.f_frsize = mFleSystemInfo.f_frsize;
            mockStatvs.f_blocks = mFleSystemInfo.f_blocks;
            mockStatvs.f_bfree = mFleSystemInfo.f_bfree;
            mockStatvs.f_bavail = 1;
            mockStatvs.f_files = 1;
            mockStatvs.f_ffree = 1;
            mockStatvs.f_favail = 1;
            MockDiskUsage disk(mockStatvs);

            disk.Update();
            probeDiskInGB.Free = disk.DiskFree(size);
            probeDiskInGB.Total = disk.DiskTotal(size); 
            probeDiskInGB.Used = disk.DiskUsed(size); 
         }
      }
      if (mFileSystemInfoCountdown-- == 1) {
         probeDiskInGB.Free = probeDiskInGB.Total;
      }
      return;
   }

   void CleanupSearch(bool canSendStats, PacketCaptureFilesystemDetails& previous, 
           ElasticSearch& es, SendStats& sendQueue, DiskSpace& probeDiskUsage) {
      return DiskCleanup::CleanupSearch(canSendStats, previous, es, sendQueue, probeDiskUsage);
   }


   std::string GetOldestIndex(ElasticSearch& es) {
      return DiskCleanup::GetOldestIndex(es);
   }

   const Conf& GetConf() { return DiskCleanup::GetConf(); }


   bool mFailRemoveSearch;
   bool mFailFileSystemInfo;
   int mFileSystemInfoCountdown;
   bool mSucceedRemoveSearch;
   struct statvfs mFleSystemInfo;
   bool mRealFilesSystemAccess;
};
