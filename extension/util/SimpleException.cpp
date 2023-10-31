/*
 * Exception.cpp
 *
 *  Created on: Feb 24, 2017
 */

#include "SimpleException.h"

#include <utlstring.h>

SimpleException::SimpleException(const CUtlString& msg) :
		msg(new CUtlString(msg)) {
}

SimpleException::SimpleException(const char* msg) :
		msg(new CUtlString(msg)) {

}

SimpleException::~SimpleException() {
	delete msg;
}

const char* SimpleException::what() const {
	return msg->Get();
}
