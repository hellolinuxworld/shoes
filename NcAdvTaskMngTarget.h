// Copyright (c) Naiky Company. All rights reserved.
//
// 高级加工任务管理，用于鞋模机
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "NcTarget.h"

#ifndef _NCUIBASE_EXP
#ifdef _NCUIBASE
#define _NCUIBASE_EXP __declspec(dllexport)
#else
#define _NCUIBASE_EXP __declspec(dllimport)
#endif
#endif

const int c_nDEFFILES_NUM = 20;
const int c_nDEFNAME_LENGTH = 30;
const int c_nTECHNAME_LENGTH = 60;
const int c_nFILENAME_LENGTH = 60;

const int c_nNOTDEFFILE = -1;

struct DefFileInfo
{
	TCHAR szTech[c_nDEFNAME_LENGTH];
	TCHAR szFile[c_nDEFNAME_LENGTH];
	workcoor_t nCoorNo;
	double nWC;
};

struct AdvFileNode
{
	// 加工文件信息
	bool bIsFolder;							// 是否是目录
	int nImageNum;							// 文件图标号
	DWORD nFileLength;						// 文件大小
	CTime nLastTime;						// 修改时间

	// 加工信息
	bool bEnable;							// 是否加工
	int nDefFileID;							// 默认工艺ID，与m_DefFileInfo索引一致，不可变更
	double nWC;								// 加工前轴运动(工件坐标)
	workcoor_t nCoorNo;						// 坐标系
	TCHAR szFileName[_MAX_PATH];			// 文件名
	TCHAR szTechName[c_nTECHNAME_LENGTH];	// 工艺名
	AdvFileNode(){ReSet();}
	void ReSet()
	{
		ZeroMemory(this, sizeof(AdvFileNode));
		nDefFileID = c_nNOTDEFFILE;
	}
};

struct AdvTaskNode
{
	TCHAR szFoldPath[_MAX_PATH];			// 文件目录全路径
	std::vector<AdvFileNode> m_vecFiles;

	AdvTaskNode(){ReSet();}
	void ReSet()
	{
		ZeroMemory(szFoldPath, sizeof(szFoldPath));
		m_vecFiles.clear();
	}
};

class _NCUIBASE_EXP CNcAdvTaskMngTarget : public CNcTarget
{
	DECLARE_DYNAMIC(CNcAdvTaskMngTarget)

public:
	CNcAdvTaskMngTarget(void);
	~CNcAdvTaskMngTarget(void);

public:
	AdvTaskNode* GetAdvTaskNode(){return &m_AdvTask;}
	bool SaveAdvTaskList(LPCTSTR strPath_);
	bool LoadAdvTaskList(LPCTSTR strPath_);
	bool IsAdvTaskValid();
	int GetAdvTaskCount();
	void SetCurAdvTask();
	bool SaveDefFileInfo();
	bool LoadDefFileInfo();

protected:
	DECLARE_MESSAGE_MAP()

private:
	LPCTSTR GetTaskInfoFileName(LPCTSTR strPath_) const;
	LPCTSTR GetTaskDefFileInfo() const;

private:
	AdvTaskNode m_AdvTask;
	// 任务文件加工时间间隔，单位s
	double m_nInterval;

private:
// Static Members
	static CNcAdvTaskMngTarget* m_pTarget;
	static DefFileInfo m_DefFileInfo[c_nDEFFILES_NUM];

// Friends
	friend _NCUIBASE_EXP CNcAdvTaskMngTarget* GetNcAdvTaskMngTarget();
	friend _NCUIBASE_EXP DefFileInfo* GetDefFileInfo();
};

_NCUIBASE_EXP CNcAdvTaskMngTarget* GetNcAdvTaskMngTarget();
_NCUIBASE_EXP DefFileInfo* GetDefFileInfo();
