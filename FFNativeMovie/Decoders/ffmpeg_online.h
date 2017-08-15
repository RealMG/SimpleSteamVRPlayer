#pragma once

#include "./ffmpeg_common.h"
#include "ffmpeg.h"
#include <Shlobj.h> //<! 用于获取Temp路径，创建删除缓存文件操作
#include <vector>


typedef struct CacheQueueItem
{
	TCHAR url[MAX_PATH];
	int32_t size;
	int64_t storageSize;
	double dts;
	int32_t streamIndex;
	int32_t isLastItem;  //<! 0:不是最后一个； 1：是最后一个 !>
	CacheQueueItem* Next;
}CacheQueueItem;


typedef struct CacheQueue
{
	CacheQueueItem *FirstItem;//队列头
	CacheQueueItem *LastItem;//队列位
	int Length;//队列长度
	SDL_mutex *cacheMutex;//用于同步两个线程同时操作队列的 互斥量
	SDL_cond *cacheCond;//唤醒线程

}CacheQueue;



//<! 网络播放器， 区别于本地播放器：带文件缓冲
class FfmpegOnline : public Ffmpeg
{

public:
	FfmpegOnline();
	~FfmpegOnline();
	virtual int32_t Open(std::string fileName);
	virtual int32_t Stop();


	virtual int32_t GetCachedPosition(uint64_t * position) ; //<!获取当前正在播放的时间
	virtual int32_t GetIsBuffering(); //<!获取当前是否在缓冲

private:
	//static int AudioThread(void* ptr);
	//static int VideoThread(void* ptr);
	static int ReadThread(void* ptr);
	int32_t SetLastCacheItem();
	static int CacheReadThread(void* ptr);

	virtual int32_t InitDecoder();  //<! 重写父类的虚函数 !>
	virtual int32_t ReleaseDecoder(); //<! 重写父类的虚函数 !>
	virtual int32_t InitThread();


	int32_t SeekWithCache();  //<! 有缓存时seek分为 在已缓存的区域被seek 和 在已缓存的区域外seek !>
	void SeekInCacheArea(CacheQueue* cq, double seekTime); //<! 在缓存的区域内进行跳转 !>
	uint32_t CreateUsableCacheFileName(TCHAR* entirePath);  //<! 创建缓存文件名 !>
	int64_t WriteCacheFileFromPacket(AVPacket* packet, LPCTSTR cacheFoldName);  //<! 将AVPacket写到磁盘中 !>
	int64_t ReadPacketFromCacheFile(AVPacket* packet, LPCTSTR cacheFoldName);   //<! 从本地磁盘中读取AVPacket !>
	uint64_t GetFreeSpaceOfPath(TCHAR* in_file_path, uint32_t path_length);     //<! 获取给定路径下剩余空间 !>
	int32_t CreateTempFoldFromSystemTempPath(TCHAR* tempPath, uint32_t length); //<! 在系统临时文件目录下创建一个存储缓存文件的目录 !>
	double CalcCacheItemDts(AVPacket* packet);   //<! 计算Dts !>
	int32_t AddCachedPtsToThis(CacheQueueItem* cache_item);
	int32_t AddCacheFileStorageSizeToThis(int64_t size);
	int32_t RemoveCacheFileStorageSizeFromThis(int64_t size);
	uint64_t CalculateStorageSpaceInKB(uint64_t sizeInBytes);  //<! 计算文件的最小空间 !>
	int32_t InitCacheQueue(CacheQueue** cq);
	void ClearCacheQueue(CacheQueue* cq);
	void ClearCacheQueueWithoutDeleteFile(CacheQueue* cq);  //<! 清空缓存队列但不删除文件 !>
	void ReleaseCacheQueue(CacheQueue** cq);
	int32_t PutItemIntoCacheQueue(CacheQueue* cq, CacheQueueItem* item);
	int32_t GetItemFromCacheQueue(CacheQueue* cq, CacheQueueItem** item);
	int32_t CreateCacheItemFromPacket(AVPacket* packet);
	bool DeleteFold(const TCHAR* lpszPath);  //<! 删除某个文件夹以及文件夹中的内容 !>

private:
	SDL_Thread* m_pCacheThread; //<! 网络播放器的缓存线程
	TCHAR m_lSystemTempPath[MAX_PATH];
	CacheQueue* m_CacheQueue;
	uint64_t m_ui64CacheSpaceInKB;
	uint64_t m_ui64CurrentCacheSpaceInKB;
};