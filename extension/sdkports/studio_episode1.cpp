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

#endif // SOURCE_ENGINE == SE_EPISODEONE