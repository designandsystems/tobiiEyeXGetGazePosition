#pragma once
#include "EyeXHost.h"
#include <windows.h>
#include <vector>
#include <mutex>
#include <cassert>
#include <cstdint>

#pragma comment (lib, "Tobii.EyeX.Client.lib")

/*
#if INTPTR_MAX == INT64_MAX
#define WINDOW_HANDLE_FORMAT "%lld"
#else
#define WINDOW_HANDLE_FORMAT "%d"
#endif
*/

// ID of the global interactor that provides our data stream; must be unique within the application.
static const TX_STRING InteractorId = "trackGazeDataStream";

// global variables
static TX_HANDLE g_hGlobalInteractorSnapshot = TX_EMPTY_HANDLE;

EyeXHost::EyeXHost() {
}

EyeXHost::~EyeXHost()
{
	if (_context != TX_EMPTY_HANDLE)
	{
		// shut down, then release the context.
		txShutdownContext(_context, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE);
		txReleaseContext(&_context);
	}
}

//Initialize
void EyeXHost::Init()
{
	_context = TX_EMPTY_HANDLE;
	//actually there's no need to define the tickets, as long as you provide their space in the header-file
	_connectionStateChangedTicket = TX_INVALID_TICKET;
	_eventHandlerTicket = TX_INVALID_TICKET;

	// create a context and register event handlers.
	bool success;
	success = txInitializeSystem(TX_SYSTEMCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL) == TX_RESULT_OK;
	success &= txCreateContext(&_context, TX_FALSE) == TX_RESULT_OK;
	success &= InitializeGlobalInteractorSnapshot(_context);
	success &= RegisterConnectionStateChangedHandler();
	success &= RegisterEventHandler();
	success &= txEnableConnection(_context) == TX_RESULT_OK;
	if (success) {
		cout << "connection successful" << endl;
	}
	else {
		cout << "connection not successful" << endl;
	}
}

bool EyeXHost::InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext) {
	TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior   = TX_EMPTY_HANDLE;
	TX_GAZEPOINTDATAPARAMS params = { TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED };
	bool success;

	success = txCreateGlobalInteractorSnapshot(hContext, InteractorId, &g_hGlobalInteractorSnapshot, &hInteractor) == TX_RESULT_OK;
	success &= txCreateInteractorBehavior(hInteractor, &hBehavior, TX_INTERACTIONBEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK;
	success &= txSetGazePointDataBehaviorParams(hBehavior, &params) == TX_RESULT_OK;

	txReleaseObject(&hBehavior);
	txReleaseObject(&hInteractor);

	return success;
}

bool EyeXHost::RegisterConnectionStateChangedHandler() {
	auto connectionStateChangedTrampoline = [](TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam) {
		static_cast<EyeXHost*>(userParam)->OnEngineConnectionStateChanged(connectionState);
	};

	bool success = txRegisterConnectionStateChangedHandler(_context, &_connectionStateChangedTicket, connectionStateChangedTrampoline, this) == TX_RESULT_OK;
	return success;
}

void EyeXHost::OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState) {
	string connectionStateMessage;
	auto snapshotCommittedTrampoline = [](TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam) {
		static_cast<EyeXHost*>(userParam)->OnSnapshotCommitted(hAsyncData);
	};

	switch (connectionState) {
		case TX_CONNECTIONSTATE::TX_CONNECTIONSTATE_CONNECTED:
			connectionStateMessage = "connected";
			BOOL success;
			success = txCommitSnapshotAsync(g_hGlobalInteractorSnapshot, snapshotCommittedTrampoline, NULL) == TX_RESULT_OK;
			if (!success) {
				connectionStateMessage += "\n Failed to initialize the data stream.\n";
				}
			else {
				connectionStateMessage += "\n Waiting for gaze data to start streaming...\n";
				}
		break;
		case TX_CONNECTIONSTATE::TX_CONNECTIONSTATE_DISCONNECTED:			connectionStateMessage = "disconnected";		break;
		case TX_CONNECTIONSTATE::TX_CONNECTIONSTATE_TRYINGTOCONNECT:		connectionStateMessage = "trying to connect";	break;
		case TX_CONNECTIONSTATE::TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:	connectionStateMessage = "version too low";		break;
		case TX_CONNECTIONSTATE::TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:	connectionStateMessage = "version too high";	break;

		default:
			break;
		}

	cout << connectionStateMessage << endl;
}

/*
 * Callback function invoked when a snapshot has been committed.
 */
void TX_CALLCONVENTION EyeXHost::OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData) {
	// check the result code using an assertion.
	// this will catch validation errors and runtime errors in debug builds. in release builds it won't do anything.

	TX_RESULT result = TX_RESULT_UNKNOWN;
	txGetAsyncDataResultCode(hAsyncData, &result);
	assert(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
}

bool EyeXHost::RegisterEventHandler() {
	auto eventHandlerTrampoline = [](TX_CONSTHANDLE hObject, TX_USERPARAM userParam) {
		static_cast<EyeXHost*>(userParam)->HandleEvent(hObject);
	};

	bool success = txRegisterEventHandler(_context, &_eventHandlerTicket, eventHandlerTrampoline, this) == TX_RESULT_OK;
	return success;
}

void TX_CALLCONVENTION EyeXHost::HandleEvent(TX_CONSTHANDLE hAsyncData) {
	TX_HANDLE hEvent = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior = TX_EMPTY_HANDLE;

	txGetAsyncDataContent(hAsyncData, &hEvent);

	// NOTE. Uncomment the following line of code to view the event object. The same function can be used with any interaction object.
	//OutputDebugStringA(txDebugObject(hEvent));

	if (txGetEventBehavior(hEvent, &hBehavior, TX_INTERACTIONBEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK) {
		OnGazeDataEvent(hBehavior);
		txReleaseObject(&hBehavior);
	}

	// NOTE since this is a very simple application with a single interactor and a single data stream, 
	// our event handling code can be very simple too. A more complex application would typically have to 
	// check for multiple behaviors and route events based on interactor IDs.

	txReleaseObject(&hEvent);
}

/*
 * Handles an event from the Gaze Point data stream.
 */
void EyeXHost::OnGazeDataEvent(TX_HANDLE hGazeDataBehavior) {
	TX_GAZEPOINTDATAEVENTPARAMS eventParams;
	if (txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams) == TX_RESULT_OK) {
		printf("Gaze Data: (%.1f, %.1f) timestamp %.0f ms\n", eventParams.X, eventParams.Y, eventParams.Timestamp);
	}
	else {
		printf("Failed to interpret gaze data event packet.\n");
	}
}