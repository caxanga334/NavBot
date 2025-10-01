#include NAVBOT_PCH_FILE
#include <extension.h>
#include <studio.h>

#if SOURCE_ENGINE > SE_DARKMESSIAH && SOURCE_ENGINE < SE_ALIENSWARM

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

CStudioHdr::CStudioHdr(const studiohdr_t* pStudioHdr, IMDLCache* mdlcache)
{
	// preset pointer to bogus value (it may be overwritten with legitimate data later)
	m_nFrameUnlockCounter = 0;
	m_pFrameUnlockCounter = &m_nFrameUnlockCounter;
	Init(pStudioHdr, mdlcache);
}

void CStudioHdr::Init(const studiohdr_t* pStudioHdr, IMDLCache* mdlcache)
{
	m_pStudioHdr = pStudioHdr;

	m_pVModel = NULL;
	m_pStudioHdrCache.RemoveAll();

	if (m_pStudioHdr == NULL)
	{
		return;
	}

	if (mdlcache)
	{
		m_pFrameUnlockCounter = mdlcache->GetFrameUnlockCounterPtr(MDLCACHE_STUDIOHDR);
		m_nFrameUnlockCounter = *m_pFrameUnlockCounter - 1;
	}

	if (m_pStudioHdr->numincludemodels == 0)
	{
#if STUDIO_SEQUENCE_ACTIVITY_LAZY_INITIALIZE || SOURCE_ENGINE == SE_CSGO
#else
		m_ActivityToSequence.Initialize(this);
#endif
	}
	else
	{
		ResetVModel(m_pStudioHdr->GetVirtualModel());
#if STUDIO_SEQUENCE_ACTIVITY_LAZY_INITIALIZE || SOURCE_ENGINE == SE_CSGO
#else
		m_ActivityToSequence.Initialize(this);
#endif
	}

	m_boneFlags.EnsureCount(numbones());
	m_boneParent.EnsureCount(numbones());
	for (int i = 0; i < numbones(); i++)
	{
		m_boneFlags[i] = pBone(i)->flags;
		m_boneParent[i] = pBone(i)->parent;
	}
}

void CStudioHdr::Term()
{
}

bool CStudioHdr::SequencesAvailable() const
{
	if (m_pStudioHdr->numincludemodels == 0)
	{
		return true;
	}

	if (m_pVModel == NULL)
	{
		// repoll m_pVModel
		return (ResetVModel(m_pStudioHdr->GetVirtualModel()) != NULL);
	}
	else
		return true;
}


const virtualmodel_t* CStudioHdr::ResetVModel(const virtualmodel_t* pVModel) const
{
	if (pVModel != NULL)
	{
		m_pVModel = (virtualmodel_t*)pVModel;
#if defined(_WIN32) && !defined(THREAD_PROFILER)
		Assert(!pVModel->m_Lock.GetOwnerId());
#endif
		m_pStudioHdrCache.SetCount(m_pVModel->m_group.Count());

		int i;
		for (i = 0; i < m_pStudioHdrCache.Count(); i++)
		{
			m_pStudioHdrCache[i] = NULL;
		}

		return const_cast<virtualmodel_t*>(pVModel);
	}
	else
	{
		m_pVModel = NULL;
		return NULL;
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

#endif