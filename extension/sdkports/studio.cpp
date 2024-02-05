#include <ifaces_extern.h>
#include <studio.h>

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