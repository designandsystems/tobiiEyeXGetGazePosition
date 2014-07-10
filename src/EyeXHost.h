#pragma once

#include "ofMain.h"
#include "EyeX.h"

class EyeXHost {
	public:

		EyeXHost();
		virtual ~EyeXHost();
		void Init();

	private:

		bool InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext);
		bool RegisterConnectionStateChangedHandler();
		void OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState);
		void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData);
		bool RegisterEventHandler();
		void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData);
		void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior);

		TX_CONTEXTHANDLE _context;
		TX_TICKET _connectionStateChangedTicket;
		TX_TICKET _eventHandlerTicket;

		void eyeposition(TX_CONTEXTHANDLE hContext);
		void test(TX_HANDLE hGazeDataBehavior);
};
