#include <extension.h>
#include <studio.h>

#if SOURCE_ENGINE >= SE_ALIENSWARM

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
	Init(NULL);
}

CStudioHdr::CStudioHdr(const studiohdr_t* pStudioHdr, IMDLCache* mdlcache)
{
	// preset pointer to bogus value (it may be overwritten with legitimate data later)
	m_nFrameUnlockCounter = 0;
	m_pFrameUnlockCounter = &m_nFrameUnlockCounter;
	Init(pStudioHdr, mdlcache);
}


// extern IDataCache *g_pDataCache;

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

	if (m_pStudioHdr->numincludemodels != 0)
	{
		ResetVModel(m_pStudioHdr->GetVirtualModel());
	}

	m_boneFlags.EnsureCount(numbones());
	m_boneParent.EnsureCount(numbones());
	for (int i = 0; i < numbones(); i++)
	{
		m_boneFlags[i] = pBone(i)->flags;
		m_boneParent[i] = pBone(i)->parent;
	}

	m_pActivityToSequence = NULL;
}

void CStudioHdr::Term()
{
	// CActivityToSequenceMapping::ReleaseMapping(m_pActivityToSequence);
	m_pActivityToSequence = NULL;
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
#if !defined( POSIX )
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

const studiohdr_t* CStudioHdr::GroupStudioHdr(int i)
{
	if (!this)
	{
		ExecuteNTimes(5, Warning("Call to NULL CStudioHdr::GroupStudioHdr()\n"));
	}

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


const studiohdr_t* CStudioHdr::pSeqStudioHdr(int sequence)
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr;
	}

	const studiohdr_t* pStudioHdr = GroupStudioHdr(m_pVModel->m_seq[sequence].group);

	return pStudioHdr;
}


const studiohdr_t* CStudioHdr::pAnimStudioHdr(int animation)
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr;
	}

	const studiohdr_t* pStudioHdr = GroupStudioHdr(m_pVModel->m_anim[animation].group);

	return pStudioHdr;
}



mstudioanimdesc_t& CStudioHdr::pAnimdesc(int i)
{
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalAnimdesc(i);
	}

	const studiohdr_t* pStudioHdr = GroupStudioHdr(m_pVModel->m_anim[i].group);

	return *pStudioHdr->pLocalAnimdesc(m_pVModel->m_anim[i].index);
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

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::iRelativeAnim(int baseseq, int relanim) const
{
	if (m_pVModel == NULL)
	{
		return relanim;
	}

	virtualgroup_t* pGroup = &m_pVModel->m_group[m_pVModel->m_seq[baseseq].group];

	return pGroup->masterAnim[relanim];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::iRelativeSeq(int baseseq, int relseq) const
{
	if (m_pVModel == NULL)
	{
		return relseq;
	}

	Assert(m_pVModel);

	virtualgroup_t* pGroup = &m_pVModel->m_group[m_pVModel->m_seq[baseseq].group];

	return pGroup->masterSeq[relseq];
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetNumPoseParameters(void) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->numlocalposeparameters;
	}

	Assert(m_pVModel);

	return m_pVModel->m_pose.Count();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

const mstudioposeparamdesc_t& CStudioHdr::pPoseParameter(int i)
{
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalPoseParameter(i);
	}

	if (m_pVModel->m_pose[i].group == 0)
		return *m_pStudioHdr->pLocalPoseParameter(m_pVModel->m_pose[i].index);

	const studiohdr_t* pStudioHdr = GroupStudioHdr(m_pVModel->m_pose[i].group);

	return *pStudioHdr->pLocalPoseParameter(m_pVModel->m_pose[i].index);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::GetSharedPoseParameter(int iSequence, int iLocalPose) const
{
	if (m_pVModel == NULL)
	{
		return iLocalPose;
	}

	if (iLocalPose == -1)
		return iLocalPose;

	Assert(m_pVModel);

	virtualgroup_t* pGroup = &m_pVModel->m_group[m_pVModel->m_seq[iSequence].group];

	return pGroup->masterPose[iLocalPose];
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::EntryNode(int iSequence)
{
	mstudioseqdesc_t& seqdesc = pSeqdesc(iSequence);

	if (m_pVModel == NULL || seqdesc.localentrynode == 0)
	{
		return seqdesc.localentrynode;
	}

	Assert(m_pVModel);

	virtualgroup_t* pGroup = &m_pVModel->m_group[m_pVModel->m_seq[iSequence].group];

	return pGroup->masterNode[seqdesc.localentrynode - 1] + 1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


int CStudioHdr::ExitNode(int iSequence)
{
	mstudioseqdesc_t& seqdesc = pSeqdesc(iSequence);

	if (m_pVModel == NULL || seqdesc.localexitnode == 0)
	{
		return seqdesc.localexitnode;
	}

	Assert(m_pVModel);

	virtualgroup_t* pGroup = &m_pVModel->m_group[m_pVModel->m_seq[iSequence].group];

	return pGroup->masterNode[seqdesc.localexitnode - 1] + 1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetNumAttachments(void) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->numlocalattachments;
	}

	Assert(m_pVModel);

	return m_pVModel->m_attachment.Count();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

const mstudioattachment_t& CStudioHdr::pAttachment(int i)
{
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalAttachment(i);
	}

	Assert(m_pVModel);

	const studiohdr_t* pStudioHdr = GroupStudioHdr(m_pVModel->m_attachment[i].group);

	return *pStudioHdr->pLocalAttachment(m_pVModel->m_attachment[i].index);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetAttachmentBone(int i)
{
	if (m_pVModel == 0)
	{
		return m_pStudioHdr->pLocalAttachment(i)->localbone;
	}

	virtualgroup_t* pGroup = &m_pVModel->m_group[m_pVModel->m_attachment[i].group];
	const mstudioattachment_t& attachment = pAttachment(i);
	int iBone = pGroup->masterBone[attachment.localbone];
	if (iBone == -1)
		return 0;
	return iBone;
}

#endif // SOURCE_ENGINE >= SE_ALIENSWARM
