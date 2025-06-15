#include "voice.h" 
#pragma warning(disable : 4996) //for fopen to work
voice::voice()
{
	//I don't know which language we will be using
	vosk_set_log_level(-1); //disable log messages, change from -1 to 0 to enable them again
	//model = vosk_model_new("../models/vosk-model-small-en-us-0.15");
	model = vosk_model_new("../models/vosk-model-small-ru-0.22"); //relative path to the model, location is subject to change, maybe
	recognizer = vosk_recognizer_new(model, 48000.0); //recognizer object, for now sample rate is default 48000hz
}
voice::~voice()
{
	vosk_recognizer_free(recognizer); //destructor text, free all the class' objects from the memory
	vosk_model_free(model);
}
std::string voice::convertAudio(const char* fileLoc)
{
	char buf[3200]; //buffer for reading
	std::string res = ""; //final result text, it's string, because in testing, using const char* resulted in weird bars
	FILE* wavin; //the file that will be opened
	int nread, final; //for reading from file
	if ((wavin = fopen(fileLoc, "rb")) != NULL) //if file at the specified path exists
	{
		fseek(wavin, 44, SEEK_SET); //set the starting positions - offset, probably specific to audio files
		while (!feof(wavin)) //read the file
		{
			nread = fread(buf, 1, sizeof(buf), wavin); //read a part
			final = vosk_recognizer_accept_waveform(recognizer, buf, nread); //convert that part
		}
		std::string resTemp = vosk_recognizer_final_result(recognizer); //get the final result - all things that got recognized by Vosk
		resTemp = resTemp.substr(14); //slicing because final result formatting is a bit specific, contains misc useless stuff
		resTemp = resTemp.substr(0, resTemp.length() - 3);
		res = resTemp;//.c_str(); //convert result to const char array
		fclose(wavin); //close the file
	}
	else //if file doesn't exist
	{
		res = "err -1"; //special value, means that audio file was not found
	}
	return res;
}