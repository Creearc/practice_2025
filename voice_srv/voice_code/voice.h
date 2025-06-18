/*
For now, unless I will be able to find a way to include the lib in there directly,
to test and to use the class, you will have to link Vosk lib to the whole project by yourself
*/
//NOTE: commentary will be in english because stinky github doesn't understand russian font
#pragma once
#include <string>
#include "vosk_api.h" //the header file for the library
class voice
{
public:
	voice(); //constructor
	voice(short t); //constructor with parameters
	~voice(); //destructor
	std::string convertAudio(const char* fileLoc); //main function
private:
	VoskModel* model; //model that recognizes audio on specific language (depends on used model files)
	VoskRecognizer* recognizer; //recognizer object, recognizes text in audio
};