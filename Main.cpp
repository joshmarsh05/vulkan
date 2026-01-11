#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#include <string>
#include "SceneManager.h"
#include "Debug.h"
#include "MMath.h"
#include <memory>


using namespace MATH;

int accol = 0;
int deaccel = 0;

void* operator new(std::size_t numBytes) {
	//std::cout << "allocating " << numBytes << " bytes of memory\n";
	accol += numBytes;
	return std::malloc(numBytes);
}

void operator delete(void* memoryLocation, std::size_t numBytes) {
	//std::cout << "freeing " << numBytes << " bytes of memory\n";
	deaccel += numBytes;
	std::free(memoryLocation);
}


/// This is a trick to get access to numBytes 
/// w/o wild typecasts. 
struct ArraySize {
	std::size_t numBytes;
};

/// I did a little hack to hide the total number of bytes
/// allocated in the array itself. 
void* operator new[](std::size_t numBytes) {
	//std::cout << "allocating an array of " << numBytes << " bytes of memory\n";
	ArraySize* array = reinterpret_cast<ArraySize*>(std::malloc(numBytes + sizeof(ArraySize)));
	std::cout << array << std::endl;
	std::cout << array + 1 << std::endl;
	if (array) array->numBytes = numBytes;
	accol += array->numBytes;
	return array + 1;
}

/// This overload doesn't work as advertised in VS22
void operator delete[](void* memoryLocation) {
	ArraySize* array = reinterpret_cast<ArraySize*>(memoryLocation) - 1;
	//std::cout << "freeing array " << array->numBytes << " bytes of memory\n";
	deaccel += array->numBytes;
	free(array);
}


int main(int argc, char* args[]) {
/***	
#pragma warning(disable : 4996) 
	if (const char* env_p = std::getenv("PATH")) {
		std::cout << "Your PATH is: " << env_p << '\n';
	}
***/
	std::string name = { "Graphics Game Engine" };
	Debug::DebugInit(name + "_Log");
	Debug::Info("Starting the GameSceneManager", __FILE__, __LINE__);
	SceneManager* gsm = new SceneManager();
	if (gsm->Initialize(name, 1280, 720) ==  true) {
		gsm->Run();
	} 
	delete gsm;
	
	std::cout << "allocated memory: " << accol << std::endl;
	std::cout << "Deallocated memory: " << deaccel << std::endl;

	_CrtDumpMemoryLeaks();
	exit(0);

}