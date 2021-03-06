#include <map>
#include <fstream>
#include <iostream>
#include <stdio.h>

#include "pin.H"
#include "Hierarchy.hh"
#include "Cache.hh"
#include "common.hh"
using namespace std;

Hierarchy* my_system;
Access element;

int start_debug;
int compiler_status;

//ofstream log_file;
ofstream output_file;
ofstream config_file;
PIN_LOCK lock;

int id;
//map<uint64_t,string> hashTable;
//map<int,string> hashTable1;
bool startInstruFlag;
map<string,string> PCinst;


VOID access(uint64_t pc , uint64_t addr, MemCmd type, int size, int id_thread)
{
	PIN_GetLock(&lock, id_thread);

	Access my_access = Access(addr, size, pc , type , id_thread);
	my_access.m_compilerHints = 0;// compiler_status;
	my_access.m_idthread = 0;
	my_system->handleAccess(my_access);		
	cpt_time++;
	PIN_ReleaseLock(&lock);
}


/* Record Instruction Fetch */
VOID RecordMemInst(VOID* pc, int size, int id_thread, int id)
{

	uint64_t convert_pc = reinterpret_cast<uint64_t>(pc);	

//	hashTable.insert(pair<uint64_t,string>(convert_pc, hashTable1[id]));
//	log_file << hashTable[id] << "\t" << std::hex << "0x" << convert_pc << std::dec << endl;

	access(0 , convert_pc , MemCmd::INST_READ , size , id_thread);
}


/* Record Data Read Fetch */
VOID RecordMemRead(VOID* pc , VOID* addr, int size, int id_thread)
{
	uint64_t convert_pc = reinterpret_cast<uint64_t>(pc);
	uint64_t convert_addr = reinterpret_cast<uint64_t>(addr);

	access(convert_pc , convert_addr , MemCmd::DATA_READ , size , id_thread);
}


/* Record Data Write Fetch */
VOID RecordMemWrite(VOID * pc, VOID * addr, int size, int id_thread)
{
	uint64_t convert_pc = reinterpret_cast<uint64_t>(pc);
	uint64_t convert_addr = reinterpret_cast<uint64_t>(addr);

	access(convert_pc , convert_addr , MemCmd::DATA_WRITE , size , id_thread);
}

VOID compilerHint(int id_thread)
{
	PIN_GetLock(&lock, id_thread);	
	if(compiler_status == 0){
		compiler_status = 1;
		cout << "[" << cpt_time << "] CACHE-PINTOOLS: Beginning of the \"hot\" spot" << endl;	
	}
	else{
		compiler_status = 0;
		cout << "[" << cpt_time << "] CACHE-PINTOOLS: Ending of the \"hot\" spot" << endl;	
	}
	PIN_ReleaseLock(&lock);
}

VOID startInstru()
{

	if(startInstruFlag)
	{	
		startInstruFlag = false;
		cout << "PINTOOLS:Stop Instru" << endl; 
	}
	else
	{
		startInstruFlag = true;
		cout << "PINTOOLS:Start Instru" << endl; 	
	}
	
}



VOID Routine(RTN rtn, VOID *v)
{           
	RTN_Open(rtn);
		
	for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)){

		string name = INS_Disassemble(ins);
		
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemInst,
		    IARG_INST_PTR,
		    IARG_UINT32,
		    INS_Size(ins),
		    IARG_THREAD_ID,
		    IARG_UINT32,
		    id,
		    IARG_END);
		id++;
		/*				
		if(INS_Opcode(ins) == XED_ICLASS_PREFETCHT1){
			INS_InsertPredicatedCall(
			ins, IPOINT_BEFORE, (AFUNPTR)startInstru,
			IARG_END);
		}*/
				/*
		if(INS_Opcode(ins) == XED_ICLASS_PREFETCHT2){
			cerr << "[0] CACHE-PINTOOLS: Instrument Instruction detected" << endl;
			INS_InsertPredicatedCall(
			ins, IPOINT_BEFORE, (AFUNPTR) compilerHint,
		        IARG_THREAD_ID,
			IARG_END);
		}*/

    		UINT32 memOperands = INS_MemoryOperandCount(ins);	    						
	
		for (UINT32 memOp = 0; memOp < memOperands; memOp++){
			if (INS_MemoryOperandIsRead(ins, memOp))
			{
				INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
				IARG_INST_PTR,
				IARG_MEMORYOP_EA, memOp,
				IARG_MEMORYREAD_SIZE,
				IARG_THREAD_ID,
				IARG_END);
			}

			if (INS_MemoryOperandIsWritten(ins, memOp))
			{
				INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
				IARG_INST_PTR,
				IARG_MEMORYOP_EA, memOp,
				IARG_MEMORYREAD_SIZE,
				IARG_THREAD_ID,
				IARG_END);
			}
		}
	}
	RTN_Close(rtn);
}


VOID Fini(INT32 code, VOID *v)
{
	cout << "EXECUTION FINISHED" << endl;
	cout << "NB Access handled " << cpt_time << endl;
	my_system->finishSimu();

	config_file.open(CONFIG_FILE);
	my_system->printConfig(config_file);
	config_file.close();

	output_file.open(OUTPUT_FILE);
	output_file << "Execution finished" << endl;
	output_file << "Printing results : " << endl;
	my_system->printResults(output_file);
	output_file.close();
	DPRINTF(DebugHierarchy, "WRITING RESULTS FINISHED\n");

}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
//	PIN_ERROR( "This Pin tool evaluate the RO detection on a LLC \n" 
//	      + KNOB_BASE::StringKnobSummary() + "\n");
	return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
	PIN_InitSymbols();
	if (PIN_Init(argc, argv)) return Usage();
	
	PIN_InitLock(&lock);
	cpt_time = 0;
	start_debug = 1;
	compiler_status = 0;
	id = 0;
	startInstruFlag = true;
	
	init_default_parameters();
	my_system = new Hierarchy("LRU" , 1);
	
	RTN_AddInstrumentFunction(Routine, 0);
	PIN_AddFiniFunction(Fini, 0);
	// Never returns
	PIN_StartProgram();

	return 0;
}
