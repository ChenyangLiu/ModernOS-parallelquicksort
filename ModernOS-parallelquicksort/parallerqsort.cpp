#include<Windows.h>
#include<cstdlib>
#include<fstream>
#include<ctime>

#include<iostream>

#define totalNum 1000000
#define dataFile "data.dat"
#define minBlock 1000

TCHAR *file = TEXT("data.dat");
HANDLE mapping;
LPSECURITY_ATTRIBUTES sa;
DWORD sysGran;
SYSTEM_INFO sysInfo;
int *b;

void genRandom()
{
	std::ofstream data;
	data.open(dataFile, std::ofstream::ios_base::out|std::ofstream::ios_base::trunc|std::ofstream::ios_base::binary);
	int range = totalNum * 10;
	srand((unsigned int)time(NULL));
	for (int i = 0; i < totalNum; i++)
	{
		int rannum;
		rannum = rand() % range;
		data.write((char*)&rannum, sizeof(int));
	}
	data.close();
}

int compare(const void* a, const void* b)
{
	return (*(int*)a - *(int*)b);
}

void exchange(int *a, int *b)
{
	int t;
	t = *b;
	*b = *a;
	*a = t;
	return;
}

struct pqsarg
{
	DWORD o;
	SIZE_T s;
	HANDLE f;
};

DWORD WINAPI pqs(LPVOID lpParam)
{
	pqsarg *Arg = (pqsarg*)lpParam;
	int range;
	int *begin;
	HANDLE fin[2];
	fin[0] = CreateEvent(sa, TRUE, FALSE, NULL);
	fin[1] = CreateEvent(sa, TRUE, FALSE, NULL);
	begin = b + Arg->o;
	if (begin == NULL)
	{
		SetEvent(Arg->f);
		return -1;
	}
	range = Arg->s / sizeof(int) - 1;
	if (range >= minBlock)
	{
		exchange(begin + (rand() % range), begin + range);
		int part;
		int x = *(begin + range);
		int i = -1;
		for (int j = 0; j < range; j++)
		{
			if (*(begin + j) < x)
			{
				i++;
				exchange(begin + i, begin + j);
			}
		}
		part = i + 1;
		exchange(begin + part, begin + range);
		pqsarg *a1, *a2;
		a1 = new(pqsarg);
		a2 = new(pqsarg);
		a1->o = Arg->o; a2->o = Arg->o + part + 1;
		a1->s = sizeof(int)*(part + 1); a2->s = sizeof(int)*(range - part);
		a1->f = fin[0]; a2->f = fin[1];
		CreateThread(NULL, 0, pqs, a1, 0, NULL);
		if (range == part)
		{
			delete(a2);
			SetEvent(fin[1]);
		}
		else
		{
			CreateThread(NULL, 0, pqs, a2, 0, NULL);
		}
		WaitForMultipleObjects(2, fin, TRUE, INFINITE);
	}
	else
	{
		qsort(begin, range + 1, sizeof(int), compare);
	}
	SetEvent(Arg->f);	
	return 0;
}

int main()
{
	genRandom();
	sa = new(SECURITY_ATTRIBUTES);
	sa->nLength = sizeof(sa);
	sa->lpSecurityDescriptor = NULL;
	sa->bInheritHandle = TRUE;
	GetSystemInfo(&sysInfo);
	sysGran = sysInfo.dwAllocationGranularity;

	HANDLE data;
	HANDLE fin;
	fin = CreateEvent(sa, TRUE, FALSE, NULL);
	data = CreateFile(file, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	mapping = CreateFileMapping(data, NULL, PAGE_READWRITE, 0, 0, NULL);
	pqsarg *arg = new(pqsarg);
	arg->o = 0;
	arg->s = sizeof(int) * totalNum;
	arg->f = fin;
	b = (int*)MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * totalNum);
	CreateThread(NULL, 0, pqs, arg, 0, NULL);
	WaitForSingleObject(fin, INFINITE);
	UnmapViewOfFile(b);

	std::ifstream d;
	d.open(dataFile, std::ifstream::ios_base::binary);
	int prev, now;
	d.read((char*)&prev, sizeof(int));
	bool isRight = TRUE;
	for (int i = 1; i < totalNum; i++)
	{
		d.read((char*)&now, sizeof(int));
		if (now < prev)
		{
			isRight = FALSE;
			break;
		}
		prev = now;
	}
	if (isRight)
		std::cout << "The result is right!" << std::endl;
	else
		std::cout << "The result is wrong!" << std::endl;

	return 0;
}