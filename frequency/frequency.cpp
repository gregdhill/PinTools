#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include "pin.H"

using namespace std;

#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

ofstream OutFile;
static UINT64 icount = 0;
static map <INS, int> instructions;
static bool detailed = false;
ADDRINT main_begin;
ADDRINT main_end;

bool _compare(const pair<INS,int> &a,const pair<INS,int> &b)
{
	return a.second>b.second;
}

bool compare_(const pair<OPCODE,int> &a,const pair<OPCODE,int> &b)
{
	return a.second>b.second;
}

void _count(INS ins) 
{
	// Ignore not linked library.
	ADDRINT ip = INS_Address(ins);
	if (ip < main_end)
		return;

	instructions[ins] += 1;
	icount++; 
}

void Instruction(INS ins, VOID *v)
{
	INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)_count, IARG_ADDRINT, ins, IARG_END);
}

void ImageLoad(IMG Img, void *v)
{
	if(IMG_IsMainExecutable(Img))
	{
		main_begin = IMG_LowAddress(Img);
		main_end = IMG_HighAddress(Img);
	}
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "frequency.out", "specify output file name");

KNOB<string> KnobInt(KNOB_MODE_WRITEONCE, "pintool",
    "v", "0", "verbosity");

VOID Fini(INT32 code, VOID *v)
{
	OutFile.setf(ios::showbase);
	OutFile <<  "===============================================" << endl;
	OutFile <<  "Total Count: " << icount << endl;
	OutFile <<  "===============================================" << endl << endl;

	map <INS, int>::iterator instruction;
	map <OPCODE, int> operators;
	map <OPCODE, int>::iterator op;
	vector<pair<INS, int> > ins_frequency;
	vector<pair<OPCODE, int> > op_frequency;

	for (instruction = instructions.begin(); instruction != instructions.end(); instruction++)
	{
		if (detailed==true) ins_frequency.push_back(*instruction);
		else operators[INS_Opcode(instruction->first)] += instruction->second;
	}

	if (detailed==true) 
	{
		OutFile << setw(50) << left << "Instruction" << setw(20) << "Count" 
			<< setw(20) << "%" << endl << endl;
		sort(ins_frequency.begin(), ins_frequency.end(), _compare);
		vector<pair<INS, int> >::iterator freq;
		for (freq = ins_frequency.begin(); freq != ins_frequency.end(); freq++)
		{
			OutFile << setw(50) << left << INS_Disassemble(freq->first)
				<< setw(20) << SSTR(freq->second) 
				<< setw(14) << SSTR((((double)freq->second / icount) * 100)) << endl;
		}
	}

	else 
	{
		vector<pair<OPCODE, int> > op_frequency;
		for (op = operators.begin(); op != operators.end(); op++)
		{
			op_frequency.push_back(*op);
		}

		OutFile << setw(20) << left << "Operator" << setw(20) << "Count" 
			<< setw(20) << "%" << endl << endl;
		sort(op_frequency.begin(), op_frequency.end(), compare_);
		vector<pair<OPCODE, int> >::iterator freq;
		for (freq = op_frequency.begin(); freq != op_frequency.end(); freq++)
		{
			OutFile << setw(20) << left << OPCODE_StringShort(freq->first)
				<< setw(20) << SSTR(freq->second) 
				<< setw(14) << SSTR((((double)freq->second / icount) * 100)) << endl;
		}

	}

	OutFile.close();
}

int main(int argc, char * argv[])
{
	PIN_Init(argc, argv);
	OutFile.open(KnobOutputFile.Value().c_str());
	detailed = atoi(KnobInt.Value().c_str());
	IMG_AddInstrumentFunction(ImageLoad, 0);
	INS_AddInstrumentFunction(Instruction, 0);
	PIN_AddFiniFunction(Fini, 0);
	PIN_StartProgram();
	return 0;
}
