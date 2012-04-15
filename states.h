/*
 * states.h
 *
 *  Created on: Apr 1, 2012
 *      Author: John Tiernan
 */

#ifndef STATES_H_
#define STATES_H_


struct setting{
	int blindPos;
	int lightLev;

};


struct setting settingsMAX;
struct setting settingsMIN;

struct setting dayNorm;
struct setting film;
struct setting nightNorm;
struct setting closed;
struct setting open;


#endif /* STATES_H_ */
