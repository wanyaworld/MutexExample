#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>


#define THREADCOUNT 48
#define MILLISEC_TO_RUN 30000

HANDLE ghMutex;

DWORD WINAPI SingleWrite(LPVOID);
DWORD WINAPI WriteToDatabase(LPVOID);

class Bench {
public:
	unsigned long long int cnt{};
};

volatile bool stop;
unsigned long long int global{};
Bench bench[THREADCOUNT];


int main(void)
{
	HANDLE aThread[THREADCOUNT];
	DWORD ThreadID;
	int i;

	// Create a mutex with no initial owner

	ghMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	if (ghMutex == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
		return 1;
	}

	// Create worker threads

	stop = true;

	int idx_arr[THREADCOUNT];

	for (i = 0; i < THREADCOUNT; i++)
	{
		idx_arr[i] = i;
		aThread[i] = CreateThread(
			NULL,       // default security attributes
			0,          // default stack size
			(LPTHREAD_START_ROUTINE)WriteToDatabase,
			idx_arr + i,       // no thread function arguments
			0,          // default creation flags
			&ThreadID); // receive thread identifier

		if (aThread[i] == NULL)
		{
			printf("CreateThread error: %d\n", GetLastError());
			return 1;
		}
	}

	stop = false;
	Sleep(MILLISEC_TO_RUN);
	stop = true;
	

	// Wait for all threads to terminate

	WaitForMultipleObjects(THREADCOUNT, aThread, TRUE, INFINITE);

	// Close thread and mutex handles

	for (i = 0; i < THREADCOUNT; i++)
		CloseHandle(aThread[i]);

	CloseHandle(ghMutex);

	unsigned long long int cnt{};
	for (int i = 0; i < THREADCOUNT; i++)
		cnt += bench[i].cnt;

	printf("is: %llu\n", global);
	printf("must be: %llu\n", cnt);
	if (cnt == global)
		printf("Correct !!\n");
	else
		printf("NOT Correct !!\n");
	printf("\n");

	global = 0;

	for (i = 0; i < 1; i++)
	{
		idx_arr[i] = i;
		aThread[i] = CreateThread(
			NULL,       // default security attributes
			0,          // default stack size
			(LPTHREAD_START_ROUTINE)SingleWrite,
			idx_arr + i,       // no thread function arguments
			0,          // default creation flags
			&ThreadID); // receive thread identifier

		if (aThread[i] == NULL)
		{
			printf("CreateThread error: %d\n", GetLastError());
			return 1;
		}
	}

	stop = false;
	Sleep(30000);
	stop = true;

	printf("Single write: %llu\n", global);
	

	return 0;
}

DWORD WINAPI SingleWrite(LPVOID lpParam) {
	while (stop);
	while (!stop) {
		global++;
	}

	return true;
}

DWORD WINAPI WriteToDatabase(LPVOID lpParam)
{

	DWORD dwCount = 0, dwWaitResult;
	int idx = *(int*)lpParam;
	printf("I am: %d\n", idx);

	// Request ownership of mutex.

	while (stop);

	while (!stop)
	{
		dwWaitResult = WaitForSingleObject(
			ghMutex,    // handle to mutex
			INFINITE);  // no time-out interval

		switch (dwWaitResult)
		{
			// The thread got ownership of the mutex
		case WAIT_OBJECT_0:
			__try {
				// TODO: Write to the database
				(bench[idx].cnt)++;
				global++;
			}

			__finally {
				// Release ownership of the mutex object
				if (!ReleaseMutex(ghMutex))
				{
					// Handle error.
				}
			}
			break;

			// The thread got ownership of an abandoned mutex
			// The database is in an indeterminate state
		case WAIT_ABANDONED:
			return FALSE;
		}
	}
	return TRUE;
}