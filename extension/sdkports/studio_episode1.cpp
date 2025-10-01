#include NAVBOT_PCH_FILE
#include <extension.h>
#include <studio.h>

#if SOURCE_ENGINE == SE_EPISODEONE || SOURCE_ENGINE == SE_DARKMESSIAH

////////////////////////////////////////////////////////////////////////
const studiohdr_t* studiohdr_t::FindModel(void** cache, char const* modelname) const
{
	return modelinfo->FindModel(this, cache, modelname);
}

virtualmodel_t* studiohdr_t::GetVirtualModel(void) const
{
	if (numincludemodels == 0)
		return NULL;
	return modelinfo->GetVirtualModel(this);
}

const studiohdr_t* virtualgroup_t::GetStudioHdr() const
{
	return modelinfo->FindModel(this->cache);
}


byte* studiohdr_t::GetAnimBlock(int iBlock) const
{
	return modelinfo->GetAnimBlock(this, iBlock);
}

int	studiohdr_t::GetAutoplayList(unsigned short** pOut) const
{
	return modelinfo->GetAutoplayList(this, pOut);
}

CStudioHdr::CStudioHdr(void)
{
	// set pointer to bogus value
	m_nFrameUnlockCounter = 0;
	m_pFrameUnlockCounter = &m_nFrameUnlockCounter;
	Init(nullptr);
}

CStudioHdr::CStudioHdr(IMDLCache* modelcache)
{
	SetModelCache(modelcache);
	Init(nullptr);
}

CStudioHdr::CStudioHdr(const studiohdr_t* pStudioHdr, IMDLCache* modelcache)
{
	SetModelCache(modelcache);
	Init(pStudioHdr);
}

void CStudioHdr::SetModelCache(IMDLCache* modelcache)
{
	m_pFrameUnlockCounter = modelcache->GetFrameUnlockCounterPtr(MDLCACHE_STUDIOHDR);
}

void CStudioHdr::Init(const studiohdr_t* pStudioHdr)
{
	m_pStudioHdr = pStudioHdr;

	m_pVModel = nullptr;
	m_pStudioHdrCache.RemoveAll();

	if (m_pStudioHdr == nullptr)
	{
		// make sure the tag says it's not ready
		m_nFrameUnlockCounter = *m_pFrameUnlockCounter - 1;
		return;
	}

	m_nFrameUnlockCounter = *m_pFrameUnlockCounter;

	if (m_pStudioHdr->numincludemodels == 0)
	{
		return;
	}

	ResetVModel(m_pStudioHdr->GetVirtualModel());

	return;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

bool CStudioHdr::SequencesAvailable() const
{
	if (m_pStudioHdr->numincludemodels == 0)
	{
		return true;
	}

	if (m_pVModel == NULL)
	{
		// repoll m_pVModel
		ResetVModel(m_pStudioHdr->GetVirtualModel());
	}

	return (m_pVModel != NULL);
}

void CStudioHdr::ResetVModel(const virtualmodel_t* pVModel) const
{
	m_pVModel = (virtualmodel_t*)pVModel;

	if (m_pVModel != NULL)
	{
		m_pStudioHdrCache.SetCount(m_pVModel->m_group.Count());

		int i;
		for (i = 0; i < m_pStudioHdrCache.Count(); i++)
		{
			m_pStudioHdrCache[i] = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::GetNumSeq(void) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->numlocalseq;
	}

	return m_pVModel->m_seq.Count();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

mstudioseqdesc_t& CStudioHdr::pSeqdesc(int i)
{
	Assert(i >= 0 && i < GetNumSeq());
	if (i < 0 || i >= GetNumSeq())
	{
		// Avoid reading random memory.
		i = 0;
	}

	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalSeqdesc(i);
	}

	const studiohdr_t* pStudioHdr = GroupStudioHdr(m_pVModel->m_seq[i].group);

	return *pStudioHdr->pLocalSeqdesc(m_pVModel->m_seq[i].index);
}

const studiohdr_t* CStudioHdr::GroupStudioHdr(int i)
{
#if 0
	if (!this)
	{
		ExecuteNTimes(5, Warning("Call to NULL CStudioHdr::GroupStudioHdr()\n"));
	}
#endif // 0

	if (m_nFrameUnlockCounter != *m_pFrameUnlockCounter)
	{
		m_FrameUnlockCounterMutex.Lock();
		if (*m_pFrameUnlockCounter != m_nFrameUnlockCounter) // i.e., this thread got the mutex
		{
			memset(m_pStudioHdrCache.Base(), 0, m_pStudioHdrCache.Count() * sizeof(studiohdr_t*));
			m_nFrameUnlockCounter = *m_pFrameUnlockCounter;
		}
		m_FrameUnlockCounterMutex.Unlock();
	}

	if (!m_pStudioHdrCache.IsValidIndex(i))
	{
		const char* pszName = (m_pStudioHdr) ? m_pStudioHdr->pszName() : "<<null>>";
		ExecuteNTimes(5, Warning("Invalid index passed to CStudioHdr(%s)::GroupStudioHdr(): %d [%d]\n", pszName, i, m_pStudioHdrCache.Count()));
		DebuggerBreakIfDebugging();
		return m_pStudioHdr; // return something known to probably exist, certainly things will be messed up, but hopefully not crash before the warning is noticed
	}

	const studiohdr_t* pStudioHdr = m_pStudioHdrCache[i];

	if (pStudioHdr == NULL)
	{
#if !defined( POSIX )
		Assert(!m_pVModel->m_Lock.GetOwnerId());
#endif
		virtualgroup_t* pGroup = &m_pVModel->m_group[i];
		pStudioHdr = pGroup->GetStudioHdr();
		m_pStudioHdrCache[i] = pStudioHdr;
	}

	Assert(pStudioHdr);
	return pStudioHdr;
}

#endif // SOURCE_ENGINE == SE_EPISODEONE