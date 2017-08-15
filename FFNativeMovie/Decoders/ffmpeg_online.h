#pragma once

#include "./ffmpeg_common.h"
#include "ffmpeg.h"
#include <Shlobj.h> //<! ���ڻ�ȡTemp·��������ɾ�������ļ�����
#include <vector>


typedef struct CacheQueueItem
{
	TCHAR url[MAX_PATH];
	int32_t size;
	int64_t storageSize;
	double dts;
	int32_t streamIndex;
	int32_t isLastItem;  //<! 0:�������һ���� 1�������һ�� !>
	CacheQueueItem* Next;
}CacheQueueItem;


typedef struct CacheQueue
{
	CacheQueueItem *FirstItem;//����ͷ
	CacheQueueItem *LastItem;//����λ
	int Length;//���г���
	SDL_mutex *cacheMutex;//����ͬ�������߳�ͬʱ�������е� ������
	SDL_cond *cacheCond;//�����߳�

}CacheQueue;



//<! ���粥������ �����ڱ��ز����������ļ�����
class FfmpegOnline : public Ffmpeg
{

public:
	FfmpegOnline();
	~FfmpegOnline();
	virtual int32_t Open(std::string fileName);
	virtual int32_t Stop();


	virtual int32_t GetCachedPosition(uint64_t * position) ; //<!��ȡ��ǰ���ڲ��ŵ�ʱ��
	virtual int32_t GetIsBuffering(); //<!��ȡ��ǰ�Ƿ��ڻ���

private:
	//static int AudioThread(void* ptr);
	//static int VideoThread(void* ptr);
	static int ReadThread(void* ptr);
	int32_t SetLastCacheItem();
	static int CacheReadThread(void* ptr);

	virtual int32_t InitDecoder();  //<! ��д������麯�� !>
	virtual int32_t ReleaseDecoder(); //<! ��д������麯�� !>
	virtual int32_t InitThread();


	int32_t SeekWithCache();  //<! �л���ʱseek��Ϊ ���ѻ��������seek �� ���ѻ����������seek !>
	void SeekInCacheArea(CacheQueue* cq, double seekTime); //<! �ڻ���������ڽ�����ת !>
	uint32_t CreateUsableCacheFileName(TCHAR* entirePath);  //<! ���������ļ��� !>
	int64_t WriteCacheFileFromPacket(AVPacket* packet, LPCTSTR cacheFoldName);  //<! ��AVPacketд�������� !>
	int64_t ReadPacketFromCacheFile(AVPacket* packet, LPCTSTR cacheFoldName);   //<! �ӱ��ش����ж�ȡAVPacket !>
	uint64_t GetFreeSpaceOfPath(TCHAR* in_file_path, uint32_t path_length);     //<! ��ȡ����·����ʣ��ռ� !>
	int32_t CreateTempFoldFromSystemTempPath(TCHAR* tempPath, uint32_t length); //<! ��ϵͳ��ʱ�ļ�Ŀ¼�´���һ���洢�����ļ���Ŀ¼ !>
	double CalcCacheItemDts(AVPacket* packet);   //<! ����Dts !>
	int32_t AddCachedPtsToThis(CacheQueueItem* cache_item);
	int32_t AddCacheFileStorageSizeToThis(int64_t size);
	int32_t RemoveCacheFileStorageSizeFromThis(int64_t size);
	uint64_t CalculateStorageSpaceInKB(uint64_t sizeInBytes);  //<! �����ļ�����С�ռ� !>
	int32_t InitCacheQueue(CacheQueue** cq);
	void ClearCacheQueue(CacheQueue* cq);
	void ClearCacheQueueWithoutDeleteFile(CacheQueue* cq);  //<! ��ջ�����е���ɾ���ļ� !>
	void ReleaseCacheQueue(CacheQueue** cq);
	int32_t PutItemIntoCacheQueue(CacheQueue* cq, CacheQueueItem* item);
	int32_t GetItemFromCacheQueue(CacheQueue* cq, CacheQueueItem** item);
	int32_t CreateCacheItemFromPacket(AVPacket* packet);
	bool DeleteFold(const TCHAR* lpszPath);  //<! ɾ��ĳ���ļ����Լ��ļ����е����� !>

private:
	SDL_Thread* m_pCacheThread; //<! ���粥�����Ļ����߳�
	TCHAR m_lSystemTempPath[MAX_PATH];
	CacheQueue* m_CacheQueue;
	uint64_t m_ui64CacheSpaceInKB;
	uint64_t m_ui64CurrentCacheSpaceInKB;
};