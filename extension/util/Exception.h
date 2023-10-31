/*
 * Exception.h
 *
 *  Created on: Nov 20, 2016
 *      Author: ttwang
 */

#ifndef SRC_EXCEPTION_H_
#define SRC_EXCEPTION_H_

class Exception {
public:
	virtual const char *what() const = 0;
};

#endif /* SRC_EXCEPTION_H_ */
