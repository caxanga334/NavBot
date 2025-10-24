#include NAVBOT_PCH_FILE
#include "sdk_entityoutput.h"

void variant_t::SetString(const char* str)
{
	iszVal = MAKE_STRING(str);
	fieldType = FIELD_STRING;
}

void variant_t::SetEntity(CBaseEntity* val)
{
	eVal.Set(reinterpret_cast<IHandleEntity*>(val));
	fieldType = FIELD_EHANDLE;
}

const CHandle<CBaseEntity>& variant_t::Entity(void) const
{
	if (fieldType == FIELD_EHANDLE)
		return reinterpret_cast<const CHandle<CBaseEntity>&>(eVal);

	static CHandle<CBaseEntity> hNull;
	hNull.Set(nullptr);
	return hNull;
}

void variant_t::Vector3D(Vector& vec) const
{
	if ((fieldType == FIELD_VECTOR) || (fieldType == FIELD_POSITION_VECTOR))
	{
		vec[0] = vecVal[0];
		vec[1] = vecVal[1];
		vec[2] = vecVal[2];
	}
	else
	{
		vec = vec3_origin;
	}
}

const char* variant_t::ToString(void) const
{
	static char szBuf[512];

	switch (fieldType)
	{
	case FIELD_STRING:
	{
		return(STRING(iszVal));
	}

	case FIELD_BOOLEAN:
	{
		if (bVal == 0)
		{
			Q_strncpy(szBuf, "false", sizeof(szBuf));
		}
		else
		{
			Q_strncpy(szBuf, "true", sizeof(szBuf));
		}
		return(szBuf);
	}

	case FIELD_INTEGER:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "%i", iVal);
		return(szBuf);
	}

	case FIELD_FLOAT:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "%g", flVal);
		return(szBuf);
	}

	case FIELD_COLOR32:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "%d %d %d %d", (int)rgbaVal.r, (int)rgbaVal.g, (int)rgbaVal.b, (int)rgbaVal.a);
		return(szBuf);
	}

	case FIELD_VECTOR:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "[%g %g %g]", (double)vecVal[0], (double)vecVal[1], (double)vecVal[2]);
		return(szBuf);
	}

	case FIELD_VOID:
	{
		szBuf[0] = '\0';
		return(szBuf);
	}

	case FIELD_EHANDLE:
	{
		CBaseEntity* entity = Entity().Get();

		if (!entity)
		{
			ke::SafeSprintf(szBuf, sizeof(szBuf), "<<NULL ENTITY>>");
			return szBuf;
		}

		// TO-DO: SDK code actually returns the entity designer name, not class name.
		return gamehelpers->GetEntityClassname(entity);
	}
	}

	return("No conversion to string");
}
