#pragma once

#include "Exception.h"

class CUtlString;

class SimpleException: public Exception {
public:
	SimpleException(const CUtlString& msg);

	SimpleException(const char* msg);

	virtual ~SimpleException();

	const char* what() const;

private:
	CUtlString* msg;
};
