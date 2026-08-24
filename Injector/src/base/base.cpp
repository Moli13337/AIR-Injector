#include "base.h"

#include <thread>

#include "request/request.h"
#include "folder/folder.h"
#include "update/update.h"
#include <iostream>

Base::Base()
{
	window.Init(); // initialize the window
	FolderManager::GetAirV1Folder(); // ensure the AirV1 folder exists
}

void Base::Run()
{
	while (window.Update()) // update the window
	{
		// sleep for 1ms to reduce CPU usage
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

void Base::Cleanup()
{
	window.Cleanup(); // cleanup the window
}

bool BaseUtils::IsDllUpdated()
{
	// Always return true to skip update checks - use local DLL
	return true;
}

bool BaseUtils::IsInjectorUpdated()
{
	// Always return true to skip update checks
	return true;
}

bool BaseUtils::UpdateDll(std::string oldPath)
{
	// Disabled - use local DLL
	return false;
}

bool BaseUtils::UpdateInjector()
{
	// Disabled - use local injector
	return false;
}