#include <iostream>
#include <fstream>
#include <string>
//#include <unistd.h>
using namespace std;

bool isLittleEndian()
//shamelessly pinched from StackOverflow
{
	short int number = 0x1;
	char *numPtr = (char*)&number;
	return (numPtr[0] == 1);
}

uint32_t fileIntPluck (ifstream& bif, uint32_t offset){
	bif.seekg(offset, bif.beg);
	char buffer[4]; //4 byte buffer
	bif.read(buffer, 0x4);
	//convert signed to unsigned
	unsigned char* ubuf = reinterpret_cast<unsigned char*>(buffer);
	uint32_t returnint = 0;
	if (isLittleEndian()){
		returnint = (ubuf[3] << 0) | (ubuf[2] << 8) | (ubuf[1] << 16) | (ubuf[0] << 24);
	} else {
		returnint = (ubuf[0] << 0) | (ubuf[1] << 8) | (ubuf[2] << 16) | (ubuf[3] << 24);
	}
	return returnint;
}

float fileFloatPluck (ifstream& bif, uint32_t offset){
	bif.seekg(offset, bif.beg);
	char buffer[4]; //4 byte buffer
	bif.read(buffer, 0x4);
	//convert signed to unsigned
	unsigned char* ubuf = reinterpret_cast<unsigned char*>(buffer);
	float returnfloat = 0;
	if (isLittleEndian()){
		returnfloat = (ubuf[3] << 0) | (ubuf[2] << 8) | (ubuf[1] << 16) | (ubuf[0] << 24);
	} else {
		returnfloat = (ubuf[0] << 0) | (ubuf[1] << 8) | (ubuf[2] << 16) | (ubuf[3] << 24);
	}
	return returnfloat;
}

void copyBytes(ifstream& bif, ofstream& bof, uint32_t offset, uint32_t length){
	char bytes[length];
	bif.seekg(offset, bif.beg);
	bif.read(bytes, length);
	bof.write(bytes, length);
}

void openFileAndCopyBytes(string filename, ifstream& bif, uint32_t offset, uint32_t length){
	ofstream bof(filename, ios::binary | ios::app);
	copyBytes(bif, bof, offset, length);
	bof.close();
}

void copyAllBytes(ifstream& bif, ofstream& bof){
	bif.seekg(0, bif.end);
	uint32_t length = bif.tellg();
	copyBytes(bif, bof, 0, length);
}

void saveIntToFileEnd(ofstream& bof, uint32_t newint){
	char buffer[4];
	char* initbuffer = reinterpret_cast<char*>(&newint);
		//assigns values wrt endianness
		if (isLittleEndian()){
			for (int i = 0; i < 4; i++){
				buffer[i] = initbuffer[3-i];
			}
		} else {
			for (int i = 0; i < 4; i++){
				buffer[i] = initbuffer[i];
			}
		}
		//write float
		bof.write(buffer, sizeof(uint32_t));
}

void saveFloatToFileEnd(ofstream& bof, float newfloat){
	char buffer[4];
	char* initbuffer = reinterpret_cast<char*>(&newfloat);
		//assigns values wrt endianness
		if (isLittleEndian()){
			for (int i = 0; i < 4; i++){
				buffer[i] = initbuffer[3-i];
			}
		} else {
			for (int i = 0; i < 4; i++){
				buffer[i] = initbuffer[i];
			}
		}
		//write float
		bof.write(buffer, sizeof(float));
}

void deleteOldFiles(string filename){
	remove((filename + "_bgheader").c_str());
	remove((filename + "_modellist").c_str());
	remove((filename + "_animheader").c_str());
	remove((filename + "_animframes").c_str());
	remove((filename + "_se1").c_str());
	remove((filename + "_se2").c_str());
	remove((filename + "_se3").c_str());
	remove((filename + "_seheader").c_str());
}

int getGameType(ifstream& file){
	if (fileIntPluck(file,0x04) > 0x10000){
		//SMB2 - this is a float, so will always be bigger that this
		return 2;
	} else {
		//SMB1 - this value is never this big
		return 1;
	}
}

uint32_t getBGListOffset(ifstream& file, int gametype){
	if (gametype == 1){
		return 0x68;
	} else { //gametype == 2
		return 0x58;
	}
}

uint32_t addToOffsetLength(ifstream& file, uint32_t offset){
	file.seekg(0, file.end);
	return offset + file.tellg();
}

uint32_t getModelNameLength(ifstream& bif, uint32_t modelnameoffset){
	bif.seekg(0, bif.end);
	uint32_t length = bif.tellg();
	bif.seekg(modelnameoffset, bif.beg);
	int modelnamelength = 0;
	bool endofmodelname = false;
	char endingbyte[1];
	while (endofmodelname == false){
		modelnamelength += 4;
		//in case it's beyond the length of the file
		if (modelnameoffset + modelnamelength > length){
			modelnamelength = length - modelnameoffset;
		}
		bif.seekg(modelnameoffset+modelnamelength-1, bif.beg);
		bif.read(endingbyte, 1);
		if (endingbyte[0] == 0){
			endofmodelname = true;
		}
	}
	return modelnamelength;
}

void helpText(){
	cout << "How to use bgtool:" << endl << "\"filename -e\" - extract background files from the file" << endl << "\"filename -r prefix\" - apply the background files with the given prefix to the file";
}

/*
Background extract functions
*/

void bgModelReadIn (string filename, ifstream& ef, uint32_t modelnameoffset){
	string modellistfilename = filename + "_modellist";
	ofstream bgmof(modellistfilename, ios::binary | ios::app);
	uint32_t modelnamelength = getModelNameLength(ef, modelnameoffset);
	bool endofmodelname = false;
	
	copyBytes(ef, bgmof, modelnameoffset, modelnamelength);
		if (modelnamelength % 4 != 0){
			char endingbyte[1];
			endingbyte[0] = 0x0;
			for (int j=0; j < 4-(modelnamelength%4); j++){
				bgmof << endingbyte[0];
			}
		}
	bgmof.close();
}

void bgAnimFramesReadIn(string filename, ifstream& ef, uint32_t animframesoffset){
	openFileAndCopyBytes(filename + "_animframes", ef, animframesoffset, 0x14);
}

void bgAnimHeaderReadIn (string filename, ifstream& ef, uint32_t animheaderoffset, int gametype){
	string animheaderfilename = filename + "_animheader";
	ofstream bgahof(animheaderfilename, ios::binary|ios::app);
	//attach initial bytes
	copyBytes(ef, bgahof, animheaderoffset, 4);
	//attach second set of bytes, convert smb1 format to smb2 format
	if (gametype == 1){
		//The second set of bytes are the animation loop time. This is an int in 1 but a float in 2.
		//This converts it to a float
		uint32_t looptimeint = fileIntPluck(ef, animheaderoffset+4);
		float looptimefloat = float(looptimeint);
		saveFloatToFileEnd(bgahof, looptimefloat);
	} else { //gametype == 2
		copyBytes(ef, bgahof, animheaderoffset+4, 4);
	}
	//attach remaining bytes
	copyBytes(ef, bgahof, animheaderoffset+8, 0x58);
	//check anim frames for all 11 flavours
	uint32_t framenumber = 0;
	uint32_t frameoffset = 0;
	for (int i = 0; i < 11; i++){
		framenumber = fileIntPluck(ef, animheaderoffset+0x8+0x8*i);
		if (framenumber != 0) {
			frameoffset = fileIntPluck(ef, animheaderoffset+0xC+0x8*i);
			for (int j = 0; j < framenumber; j++){
				bgAnimFramesReadIn(filename, ef, frameoffset+0x14*j);
			}
		}
	}
	bgahof.close();
}

void bgSpecialEffectType1ReadIn(string filename, ifstream& ef, uint32_t typeoneoffset){
	openFileAndCopyBytes(filename + "_se1", ef, typeoneoffset, 0x14);
}

void bgSpecialEffectType2ReadIn(string filename, ifstream& ef, uint32_t typetwooffset){
	openFileAndCopyBytes(filename + "_se2", ef, typetwooffset, 0x10);
}

void bgSpecialEffectType3ReadIn(string filename, ifstream& ef, uint32_t typethreeoffset){
	openFileAndCopyBytes(filename + "_se3", ef, typethreeoffset, 0x08);
}

void bgSpecialEffectHeaderReadIn(string filename, ifstream& ef, uint32_t seheaderoffset){
	//length 0x30
	string seheaderfilename = filename + "_seheader";
	ofstream bgsehof(seheaderfilename,ios::binary|ios::app);
	copyBytes(ef, bgsehof, seheaderoffset, 0x30);
	//0x00 and 0x04 - Length 0x14
	uint32_t typeonenumber = fileIntPluck(ef, seheaderoffset);
	if (typeonenumber != 0){
		uint32_t typeoneoffset = fileIntPluck(ef, seheaderoffset+0x4);
		for (int i = 0; i < typeonenumber; i++){
			bgSpecialEffectType1ReadIn(filename, ef, typeoneoffset+0x14*i);
		}
	}
	//0x08 and 0x0C - Length 0x10
	uint32_t typetwonumber = fileIntPluck(ef, seheaderoffset+0x8);
	if (typetwonumber != 0){
		uint32_t typetwooffset = fileIntPluck(ef, seheaderoffset+0xC);
		for (int i = 0; i < typetwonumber; i++){
			bgSpecialEffectType2ReadIn(filename, ef, typetwooffset+0x10*i);
		}
	}
	//0x10 - Length 0x08
	uint32_t typethreeoffset = fileIntPluck(ef, seheaderoffset+0x10);
	if (typethreeoffset != 0){
		bgSpecialEffectType3ReadIn(filename, ef, typethreeoffset);
	}
	//Other offsets apparently unused? Watch this space
	bgsehof.close();
}

void bgHeaderReadIn (string filename, ifstream& ef, uint32_t bgnumber, uint32_t bgoffset, int gametype){
	string bgheaderfilename = filename + "_bgheader";
	ofstream bghof(bgheaderfilename, ios::binary);
	uint32_t modelnameoffset = 0;
	uint32_t eolanimheaderoffset = 0;
	uint32_t standardanimheaderoffset = 0;
	uint32_t seheaderoffset = 0;
	for (int i = 0; i < bgnumber; i++){
		//Each background header is 38 bytes long.
		copyBytes(ef, bghof, bgoffset+0x38*i, 0x38);
		//check name
		modelnameoffset = fileIntPluck(ef, bgoffset+0x38*i+0x4);
		bgModelReadIn(filename, ef, modelnameoffset);
		//end of level animation
		eolanimheaderoffset = fileIntPluck(ef, bgoffset+0x38*i+0x2c);
		if (eolanimheaderoffset !=0){
			bgAnimHeaderReadIn (filename, ef, eolanimheaderoffset, gametype);
		}
		//standard animation
		standardanimheaderoffset = fileIntPluck(ef, bgoffset+0x38*i+0x30);
		if (standardanimheaderoffset != 0){
			bgAnimHeaderReadIn (filename, ef, standardanimheaderoffset, gametype);
		}
		//special effect headers
		seheaderoffset = fileIntPluck(ef, bgoffset+0x38*i+0x34);
		if (seheaderoffset != 0){
			bgSpecialEffectHeaderReadIn (filename, ef, seheaderoffset);
		}
	}
	bghof.close();
}

void bgInitReadIn (string filename, ifstream& ef, uint32_t initialoffset){
	openFileAndCopyBytes(filename + "_initvals", ef, initialoffset, 0x08);
}

int fileExtract (string filename){
	ifstream ef;
	/*
	if (access (filename.c_str(), F_OK) == -1) {
		cout << "Requested file does not exist.";
		return -1;
	}
	*/
	ef.open(filename, ios::binary);
	if (ef.good() == false){
		cout << "Requested file does not exist.";
		ef.close();
		return -1;
	}
	int gametype = getGameType(ef);
	uint32_t initialoffset = getBGListOffset(ef, gametype);
	/*
	uint32_t initialoffset = 0;
	int gametype = getGameType(ef);
	if (gametype == 1){
		initialoffset = 0x68;
	} else { //gametype == 2
		initialoffset = 0x58;
	}
	*/
	uint32_t bgnumber = fileIntPluck(ef, initialoffset); //no. of background objects
	if (bgnumber > 300) {
		//incredibly high numbers of bg objects do not exist in SMB1 or 2. Those with such objects will likely be malformed
		cout << "Very high number of background objects detected. (>300)" << endl << "Please check if your file has been decompressed correctly." << endl << "No files made.";
		ef.close();
		return -1;
	} else if (bgnumber == 0) {
		//no need to make files if there are no objects
		cout << "No background objects detected. No files made.";
		ef.close();
		return 0;
	}
	uint32_t bgoffset = fileIntPluck(ef, initialoffset+4);
	ef.seekg (0, ef.end);
	uint32_t eflength = ef.tellg();
	if (bgoffset > eflength) {
		cout << "Offset beyond file bounds." << endl << "Please check if your file has been decompressed correctly." << endl << "No files made.";
		ef.close();
		return -1;
	}
	cout << "Number of background objects: " << bgnumber;
	deleteOldFiles(filename);
	bgInitReadIn(filename, ef, initialoffset);
	bgHeaderReadIn(filename, ef, bgnumber, bgoffset, gametype);
	ef.close();
	return 0;
}

/*
Background reconstructor functions
*/

void addBytesModelList(ifstream& bgmif, ofstream& rf){
	copyAllBytes(bgmif, rf);
}

void addBytesAnimationFrames(ifstream& bgafif, ofstream& rf){
	copyAllBytes(bgafif, rf);
}

void addBytesAnimationHeader(ifstream& bgahif, ofstream& rf, uint32_t animframesoffset, int gametype){
	bgahif.seekg(0, bgahif.end);
	uint32_t framepointer = animframesoffset;
	uint32_t framenumber = 0;
	uint32_t animheaderlength = bgahif.tellg();
	uint32_t animheadernumber = animheaderlength / 0x60;
	float looptimefloat = 0;
	uint32_t looptimeint = 0;
	for (int i = 0; i < animheadernumber; i++){
		//write first bytes
		copyBytes(bgahif, rf, i*0x60, 0x4);
		//convert float to relevant type
		if (gametype == 1){
			looptimefloat = fileFloatPluck(bgahif, i*0x60+4);
			looptimeint = int(looptimefloat);
			saveIntToFileEnd(rf, looptimeint);
		} else {
			copyBytes(bgahif, rf, i*0x60+0x4, 0x4);
		}
		//write remaining bytes
		for (int j = 0; j < 11; j++){
			framenumber = fileIntPluck(bgahif, (i*0x60)+0x8+0x8*j);
			saveIntToFileEnd(rf, framenumber);
			if (framenumber != 0) {
				saveIntToFileEnd(rf, framepointer);
				framepointer += framenumber*0x14;
			} else {
				saveIntToFileEnd(rf, 0);
			}
		}
	}
}

void addBytesSEOne(ifstream& bgseoneif, ofstream& rf){
	copyAllBytes(bgseoneif, rf);
}
void addBytesSETwo(ifstream& bgsetwoif, ofstream& rf){
	copyAllBytes(bgsetwoif, rf);
}
void addBytesSEThree(ifstream& bgsethreeif, ofstream& rf){
	copyAllBytes(bgsethreeif, rf);
}

void addBytesSEHeader(ifstream& bgsehif, ofstream& rf, uint32_t typeoneoffset, uint32_t typetwooffset, uint32_t typethreeoffset){
	bgsehif.seekg(0, bgsehif.end);
	uint32_t seheaderlength = bgsehif.tellg();
	uint32_t seheadernumber = seheaderlength / 0x30;
	uint32_t typeonepointer = typeoneoffset;
	uint32_t typeonenumber = 0;
	uint32_t typetwopointer = typetwooffset;
	uint32_t typetwonumber = 0;
	uint32_t typethreepointer = typethreeoffset;
	uint32_t typethreeoldpointer = 0;
	for (int i = 0; i < seheadernumber; i++){
		typeonenumber = fileIntPluck(bgsehif, (i*0x30));
		saveIntToFileEnd(rf, typeonenumber);
		if (typeonenumber != 0) {
			saveIntToFileEnd(rf, typeonepointer);
			typeonepointer += typeonenumber*0x14;
		} else {
			saveIntToFileEnd(rf, 0);
		}
		typetwonumber = fileIntPluck(bgsehif, (i*0x30+0x8));
		saveIntToFileEnd(rf, typetwonumber);
		if (typetwonumber != 0) {
			saveIntToFileEnd(rf, typetwopointer);
			typetwopointer += typetwonumber*0x10;
		} else {
			saveIntToFileEnd(rf, 0);
		}
		typethreeoldpointer = fileIntPluck(bgsehif, (i*0x30+0x10));
		if (typethreeoldpointer != 0) {
			saveIntToFileEnd(rf, typethreepointer);
			typethreepointer += 0x08;
		} else {
			saveIntToFileEnd(rf, 0);
		}
		//remaining bytes
		copyBytes(bgsehif, rf, i*0x30+0x14, 0x1C);
	}
}

void addBytesBGHeader(ifstream& bghif, ofstream& rf, ifstream& bgmif, uint32_t modellistoffset, uint32_t animheaderoffset, uint32_t seheaderoffset){
	bghif.seekg(0, bghif.end);
	uint32_t bgheaderlength = bghif.tellg();
	uint32_t bgheadernumber = bgheaderlength / 0x38;
	uint32_t modelnamepointer = 0;
	uint32_t modelnamelength = 0;
	uint32_t animpointer = animheaderoffset;
	uint32_t animoldpointer = 0;
	uint32_t seheaderpointer = seheaderoffset;
	uint32_t seheaderoldpointer = 0;
	
	for (int i = 0; i < bgheadernumber; i++){
		//copies first byte
		copyBytes(bghif, rf, i*0x38, 0x4);
		
		//rewrites model offset
		saveIntToFileEnd(rf, modellistoffset + modelnamepointer);
		modelnamelength = getModelNameLength(bgmif, modelnamepointer);
		modelnamepointer += modelnamelength;
		//copies next 0x24 bytes
		copyBytes(bghif, rf, i*0x38+0x8, 0x24);
		//rewrites both animation offsets
		for (int j = 0; j < 2; j++){
			animoldpointer = fileIntPluck(bghif, (i*0x38+0x2C+j*0x4));
			if (animoldpointer != 0) {
				saveIntToFileEnd(rf, animpointer);
				animpointer += 0x60;
			} else {
				saveIntToFileEnd(rf, 0);
			}
		}
		//rewrite se offset
		seheaderoldpointer = fileIntPluck(bghif, (i*0x38+0x34));
		if (seheaderoldpointer != 0) {
			saveIntToFileEnd(rf, seheaderpointer);
			seheaderpointer += 0x30;
		} else {
			saveIntToFileEnd(rf, 0);
		}
	}
}

int fileReconstruct(string sourcefilename, string bgfileprefix){
	/*
	NEW FILE ORDER
	
	- Model names
	- Anim Frames
	- Anim Header
	- SE1
	- SE2
	- SE3
	- SE Header
	- BG Header
	
	*/
	ofstream rf(sourcefilename + "_newbg", ios::binary);
	ifstream srf;
	srf.open(sourcefilename, ios::binary);
	if (srf.good() == false){
		cout << "Requested source file " << sourcefilename <<" does not exist. No files made.";
		srf.close();
		return -1;
	}
	int gametype = getGameType(srf);
	uint32_t initialoffset = getBGListOffset(srf, gametype);
	uint32_t bgnumber = fileIntPluck(srf, initialoffset); //no. of background objects
	if (bgnumber > 300) {
		//incredibly high numbers of bg objects do not exist in SMB1 or 2. Those with such objects will likely be malformed
		cout << "Very high number of background objects in source file detected. (>300)" << endl << "Please check if your file has been decompressed correctly." << endl << "No files made.";
		srf.close();
		return -1;
	}
	//open initvals
	ifstream bgiif;
	bgiif.open(bgfileprefix + "_initvals", ios::binary);
	if (bgiif.good() == false){
		cout << "Requested initial value file " << bgfileprefix << "_initvals does not exist. No files made.";
		srf.close();
		bgiif.close();
		return -1;
	}
	//open header
	ifstream bghif;
	bghif.open(bgfileprefix + "_bgheader", ios::binary);
	if (bghif.good() == false){
		cout << "Requested bg header file " << bgfileprefix << "_bgheader does not exist. No files made.";
		srf.close();
		bgiif.close();
		bghif.close();
		return -1;
	}
	//open modellist
	ifstream bgmif;
	bgmif.open(bgfileprefix + "_modellist", ios::binary);
	if (bgmif.good() == false){
		cout << "Requested model list file " << bgfileprefix << "_modellist does not exist. No files made.";
		srf.close();
		bgiif.close();
		bghif.close();
		return -1;
	}
	//begin calculation of total offset
	srf.seekg (0, srf.end);
	uint32_t srflength = srf.tellg();
	uint32_t newmodeloffset = srflength;
	//check if animheader exists
	bgmif.seekg(0, bgmif.end);
	ifstream bgahif;
	ifstream bgafif;
	bool hasbganim = true;
	uint32_t newanimframesoffset = newmodeloffset + bgmif.tellg();
	uint32_t newanimheaderoffset = newanimframesoffset;
	uint32_t animheaderlength = 0;
	bgahif.open(bgfileprefix + "_animheader", ios::binary);
	if (bgahif.good() == false){
		//if no animheader file there's no animation
		hasbganim = false;
	} else {
		//if animheader exists, open animframes as well
		bgafif.open(bgfileprefix + "_animframes", ios::binary);
		if (bgafif.good() == false){
			cout << "Requested anim frames file " << bgfileprefix << "_animframes does not exist despite anim header file existing. No files made.";
			srf.close();
			bgiif.close();
			bghif.close();
			bgahif.close();
			bgafif.close();
			return -1;
		}
		newanimheaderoffset = addToOffsetLength(bgafif, newanimframesoffset);
		bgahif.seekg(0, bgahif.end);
		animheaderlength = bgahif.tellg();
	}
	//check if se header exists
	ifstream bgsehif;
	ifstream bgseoneif;
	ifstream bgsetwoif;
	ifstream bgsethreeif;
	bool hasspecial = true;
	bool hasseone = false;
	//this is so clumsy I'm sorry
	bool hassetwo = false;
	bool hassethree = false;
	uint32_t newseoneoffset = newanimheaderoffset + animheaderlength;
	uint32_t newsetwooffset = newseoneoffset;
	uint32_t newsethreeoffset = newsetwooffset;
	uint32_t newseheaderoffset = newsethreeoffset;
	uint32_t seheaderlength = 0;
	bgsehif.open(bgfileprefix + "_seheader", ios::binary);
	if (bgsehif.good() == false){
		//if no animheader file there's no animation
		hasspecial = false;
	} else {
		//check if the se files exist
		bgseoneif.open(bgfileprefix + "_se1", ios::binary);
		if (bgseoneif.good() == true){
			hasseone = true;
			newsetwooffset = addToOffsetLength(bgseoneif, newseoneoffset);
			newsethreeoffset = newsetwooffset;
			newseheaderoffset = newsethreeoffset;
		}
		bgsetwoif.open(bgfileprefix + "_se2", ios::binary);
		if (bgsetwoif.good() == true){
			hassetwo = true;
			newsethreeoffset = addToOffsetLength(bgsetwoif, newsetwooffset);
			newseheaderoffset = newsethreeoffset;
		}
		bgsethreeif.open(bgfileprefix + "_se3", ios::binary);
		if (bgsethreeif.good() == true){
			hassethree = true;
			newseheaderoffset = addToOffsetLength(bgsethreeif, newsethreeoffset);
		}
		bgsehif.seekg(0, bgsehif.end);
		seheaderlength = bgsehif.tellg();
	}
	//^ THERE ARE DEFINITELY BETTER IMPLEMENTATIONS OF THIS THING
	// only been learning this for a couple months okay
	uint32_t newbgheaderoffset = newseheaderoffset + seheaderlength;
	uint32_t remainingbytes = (newmodeloffset - initialoffset) - 0x8;
	//initial bytes of rf
	copyBytes(srf, rf, 0, initialoffset);
	//new header
	copyBytes(bgiif, rf, 0, 0x4);
	//new offset value
	saveIntToFileEnd(rf, newbgheaderoffset);
	//remaining bytes of srf
	copyBytes(srf, rf, initialoffset+0x8, remainingbytes);
	//done with srf
	srf.close();
	//add bytes for modellist
	addBytesModelList(bgmif, rf);
	if (hasbganim){
		//add bytes for animation frames
		addBytesAnimationFrames(bgafif, rf);
		//add bytes for animation header
		addBytesAnimationHeader(bgahif, rf, newanimframesoffset, gametype);
	}
	//add bytes for se types and header
	if (hasspecial){
		if (hasseone){
			addBytesSEOne(bgseoneif, rf);
		}
		if (hassetwo){
			addBytesSETwo(bgsetwoif, rf);
		}
		if (hassethree){
			addBytesSEThree(bgsethreeif, rf);
		}
		addBytesSEHeader(bgsehif, rf, newseoneoffset, newsetwooffset, newsethreeoffset);
	}
	//add main bg header
		addBytesBGHeader(bghif, rf, bgmif, newmodeloffset, newanimheaderoffset, newseheaderoffset);
	//close everything
	bgiif.close();
	bghif.close();
	bgmif.close();
	bgafif.close();
	bgahif.close();
	bgseoneif.close();
	bgsetwoif.close();
	bgsethreeif.close();
	bgsehif.close();
	rf.close();
	cout << "Added data from the " << bgfileprefix << " prefix set to the file. See " << sourcefilename << "_newbg";
	return 0;
}

/*
Main function
*/

int main (int argc, char* argv[]){
	//Extract background elements: Filename, then -e or --extract
	int successval = 1;
	if (argc >= 3) {
		string operationtype(argv[2]);
		if (argc == 3) {
			if (operationtype == "-e") {
				string filename(argv[1]);
				successval = fileExtract(filename);
			}
		} else if (argc == 4) {
			if (operationtype == "-r") {
				string sourcefilename(argv[1]);
				string bgfileprefix(argv[3]);
				successval = fileReconstruct(sourcefilename, bgfileprefix);
			}
		} else {helpText();}
	} else {helpText();}
	return successval;
}