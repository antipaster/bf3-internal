#ifndef VMTHOOK_H
#define VMTHOOK_H

class CVMTHookManager
{
public:
	CVMTHookManager(void)
	{
		memset(this, 0, sizeof(CVMTHookManager));
	}

	CVMTHookManager(PDWORD* ppdwClassBase)
	{
		bInitialize(ppdwClassBase);
	}

	~CVMTHookManager(void)
	{
		UnHook();
	}
	bool bInitialize(PDWORD* ppdwClassBase)
	{
		m_ppdwClassBase = ppdwClassBase;
		m_pdwOldVMT = *ppdwClassBase;
		m_dwVMTSize = dwGetVMTCount(*ppdwClassBase);
		m_pdwNewVMT = new DWORD[m_dwVMTSize];
		memcpy(m_pdwNewVMT, m_pdwOldVMT, sizeof(DWORD) * m_dwVMTSize);
		*ppdwClassBase = m_pdwNewVMT;
		return true;
	}
	bool bInitialize(PDWORD** pppdwClassBase) // fix for pp
	{
		return bInitialize(*pppdwClassBase);
	}

	void UnHook(void)
	{
		dwHookMethod(oFunction, unhookindex);
	}

	void ReHook(void)
	{
		if (m_ppdwClassBase)
		{
			*m_ppdwClassBase = m_pdwNewVMT;
		}
	}

	int iGetFuncCount(void)
	{
		return (int)m_dwVMTSize;
	}

	DWORD dwGetMethodAddress(int Index)
	{
		if (Index >= 0 && Index <= (int)m_dwVMTSize && m_pdwOldVMT != NULL)
		{
			unhookindex = Index;
			oFunction = m_pdwOldVMT[Index];
			return m_pdwOldVMT[Index];
		}
		return NULL;
	}

	PDWORD pdwGetOldVMT(void)
	{
		return m_pdwOldVMT;
	}

	DWORD dwHookMethod(DWORD dwNewFunc, unsigned int iIndex)
	{
		if (m_pdwNewVMT && m_pdwOldVMT && iIndex <= m_dwVMTSize && iIndex >= 0)
		{
			m_pdwNewVMT[iIndex] = dwNewFunc;
			return m_pdwOldVMT[iIndex];
		}

		return NULL;
	}

private:
	DWORD dwGetVMTCount(PDWORD pdwVMT)
	{
		DWORD dwIndex = 0;

		for (dwIndex = 0; pdwVMT[dwIndex]; dwIndex++)
		{
			if (IsBadCodePtr((FARPROC)pdwVMT[dwIndex]))
			{
				break;
			}
		}
		return dwIndex;
	}
	PDWORD*	m_ppdwClassBase;
	PDWORD	m_pdwNewVMT, m_pdwOldVMT;
	DWORD	m_dwVMTSize, oFunction;
	int unhookindex;
};

static PBYTE HookVtblFunction(PDWORD* dwVTable, PBYTE dwHook, INT Index)
{
	DWORD dwOld = 0;
	VirtualProtect((void*)((*dwVTable) + Index), 4, PAGE_EXECUTE_READWRITE, &dwOld);

	PBYTE pOrig = ((PBYTE)(*dwVTable)[Index]);
	(*dwVTable)[Index] = (DWORD)dwHook;

	VirtualProtect((void*)((*dwVTable) + Index), 4, dwOld, &dwOld);

	return pOrig;
}

#endif // VMTHOOK_H