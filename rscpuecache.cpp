//Name: Ryan Jaskulski
//File: rscpuecache.cpp
//Date: 3/31/23

/*
 * Description: rscpuecache is a simulator for rscpu that now includes a direct mapped write
 * through/write allocate cache.
 * instructions.
 *
 *
 */

#define Z 1
#define C 2
#define V 4
#define N 8

#define m 256
#define invalidTag 255


#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
using namespace std;

unsigned char DR,TR,IR,flag=0; //Registers
unsigned short PC,R,AR,AC=0;
bool is_executing=true; //Control boolean for the fetch decode loop
uint8_t M[65536]; //Simulated memory
const int MAX_UCHAR = 255;
const int MAX_SCHAR = 127;
const int MIN_UCHAR = 0;
const int MIN_SCHAR = -128;

const int MAX_USHORT = 65535;
const int MIN_USHORT = 0;
const int MAX_SSHORT = 32767;
const int MIN_SSHORT = -32768;

typedef struct {
      unsigned char tag;
      unsigned char line[4];
} cacheEntry;

cacheEntry theCache[m];
int cacheHits, cacheMisses;

//Fill in all addresses in memory to 0; prevents weird errors that uninitialized
//variables cause in c++
void initMemory()
{
      for(int i = 0; i < (sizeof(M)/sizeof(M[0])); i++)
      {
            M[i] = 0;
      }
}

void initCache()
{
      cout << "Initalizing the cache..." <<  endl;
      for(int i = 0; i < m; i++)
      {
            theCache[i].tag = invalidTag;
            theCache[i].line[0] = 0;
            theCache[i].line[1] = 0;
            theCache[i].line[2] = 0;
            theCache[i].line[3] = 0;
      }
      cacheHits = 0;
      cacheMisses = 0;
}

void printCacheInfo()
{

      int iLine = ((AR & 1020) >> 2);
      cout << "AR being sought from the cache = " << AR << endl;
      cout << "The main memory block number = " << ((AR & 1020) >> 2) <<  endl;
      cout << "The cache line associated with AR = " << iLine <<  endl;
      cout << "entry " << iLine << " tag = " << (int)theCache[iLine].tag << " data = " << (int)theCache[iLine].line[0] << " " << (int)theCache[iLine].line[1] << " " << (int)theCache[iLine].line[2] << " " << (int)theCache[iLine].line[3] << endl;
}

bool cacheHit()
{
      int iLine = ((AR & 1020) >> 2);

      if (theCache[iLine].tag == ((AR & 64512) >> 10))
      {
            cacheHits++;
            return true;
      }
      return false; //For now
}


//On cache miss, bring in the missing data into cache from memory.
void cacheMiss()
{
      cout << "Cache Miss" << endl;
      cacheMisses++;
      //Read in the memory blocks from main memory.
      int iLine = ((AR & 1020) >> 2);
      int memBlock = (AR & 65532);

      theCache[iLine].tag = ((AR & 64512) >> 10);

      theCache[iLine].line[0] = M[memBlock];
      theCache[iLine].line[1] = M[memBlock+1];
      theCache[iLine].line[2] = M[memBlock+2];
      theCache[iLine].line[3] = M[memBlock+3];

      printCacheInfo();
}

//Unified function for reading in from memory/cache.
void readMemory()
{
      printCacheInfo();
      if(!cacheHit())
      {
            cacheMiss();
      }
      else
      {
            cout << "Cache Hit" << endl;
      }

      //Read in the data from memory.

      int iLine = ((AR & 1020) >> 2);

      DR = theCache[iLine].line[(AR & 3)];

      printCacheInfo();

}

//unified function for writing to cache/memory.
void writeMemory()
{
      printCacheInfo();
      int iLine = ((AR & 1020) >> 2);
      int memBlock = (AR & 65532);

      theCache[iLine].tag = ((AR & 64512) >> 10);

      theCache[iLine].line[0] = M[memBlock];
      theCache[iLine].line[1] = M[memBlock+1];
      theCache[iLine].line[2] = M[memBlock+2];
      theCache[iLine].line[3] = M[memBlock+3];

      printCacheInfo();

      //Update the data in the cache
      theCache[iLine].line[(AR & 3)] = DR;
      cout << "Cache entry after being written to..." << endl;
      cout << "entry " << iLine << " tag = " << (int)theCache[iLine].tag << " data = " << (int)theCache[iLine].line[0] << " " << (int)theCache[iLine].line[1] << " " << (int)theCache[iLine].line[2] << " " << (int)theCache[iLine].line[3] << endl;
      //Update the data in memory
      M[AR] = DR;

      //Write to main memory and to the cache (write through)
}

//Preemptively read in all bytes and decode them from hexadecimals in string form into integers
void dumpSourceToMemory(string programFile)
{

      ifstream sourceFile(programFile);
      string ln = " "; //Line feed
      int i = 0;
      bool is_number = true;
      while(getline(sourceFile, ln))
      {
            if(ln != "")
            {

                  M[i] = stoi(ln,nullptr,16);
                  i++;
            }
      }
      cout << "End of file reached... BEGIN EXECUTION." << endl;
}

unsigned short formAddress(int high, int low)
{

      stringstream hex_convert;

      hex_convert << hex << high << low;

      string full_hex_address;

      hex_convert >> full_hex_address;

      return stoi(full_hex_address, nullptr, 16);
}


void reassembleAC16(unsigned char low = 0, unsigned char high = (AC & 65280)) //high should always be its optional substituted value
                                                                          //so, DO NOT EVER FILL IN
                                                                          //THIS ARGUMENT!!!
{
      //Temporary storage of the final AC value
      unsigned short AC_TEMP = 0;

      //Fetch high order bits into the low half
      AC_TEMP = (AC_TEMP | high);

      //Shift high bits into the high order half
      AC_TEMP = (AC_TEMP << 8);

      //Fetch low order bits into the low order half
      AC_TEMP = (AC_TEMP | low);


      //DONE, so set AC to the final result
      AC = AC_TEMP;
}


void reassembleR16(unsigned char low = 0, unsigned char high = (R & 65280)) //high should always be its optional substituted value
                                                                          //so, DO NOT EVER FILL IN
                                                                          //THIS ARGUMENT!!!
{
      //Temporary storage of the final AC value
      unsigned short R_TEMP = 0;

      //Fetch high order bits into the low half
      R_TEMP = (R_TEMP | high);

      //Shift high bits into the high order half
      R_TEMP = (R_TEMP << 8);

      //Fetch low order bits into the low order half
      R_TEMP = (R_TEMP | low);


      //DONE, so set AC to the final result
      R = R_TEMP;
}

void toggleZeroFlag(unsigned char reg)
{
  if((signed char)reg == 0)
      {
            flag |= Z;
      }
      else
      {
            flag &= ~Z;
      }
}

void toggleZeroFlag(unsigned short reg)
{
  if((signed short)reg == 0)
      {
            flag |= Z;
      }
      else
      {
            flag &= ~Z;
      }
}

void toggleNegativeFlag(unsigned char reg)
{
  if ((signed char)reg < 0)
      {
            flag |= N;
      }
      else
      {
            flag &= ~N;
      }
}

void toggleNegativeFlag(unsigned short reg)
{
  if ((signed short)reg < 0)
      {
            flag |= N;
      }
      else
      {
            flag &= ~N;
      }
}

//TODO: Write in bit functions to set/clear flags

//Literally does nothing :D
void nop()
{
      cout << "NOP instruction" << endl;
      //Do nothing
      return;
}

//HALT INSTRUCTION

//Stops execution of the program
void halt()
{
      cout << "HALT instruction; STOP EXECUTION" << endl;
      //Set the executing flag to 0 to stop the fetch decode loop
      is_executing = false;
      return;
}


//Clears the AC register and sets the zero flag
void clac()
{

      unsigned char AC8 = (AC & 255);
      cout << "CLAC instruction" << endl;
      AC8 = 0;
      flag &=0;
      flag |= Z;


      //Clear the low order bits and insert result of AC8.
      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "CLAC1: AC8 = " << (int)AC8 << " AC = " << (signed short)AC << " flag = " << (int)flag << endl;

}

void clac16()
{
      cout << "CLAC16 instruction" << endl;
      AC = 0;
      flag &=0;
      flag |= Z;
      cout << "CLAC16_1: AC = " << (signed short)AC << "AC8 = " << (int)(AC & 255) << " flag = " << (int)flag << endl;
}

//Increments the AC register, and sets appropriate flags
void inac16()
{
      cout << "INAC16 instruction" << endl;
      const unsigned char AC_TEMP=AC; //Copy of the AC before the operation is performed on AC.
      AC++;

      //Check the original AC and compare it to the result to check for overflow/carry
      //TODO?: Split this into a seperate function??
      if((signed short)AC_TEMP > 0 && (signed short)AC < 0)
      {
            flag |= V;
      }
      else
	{
	  flag &= ~V;
	}
      if((unsigned short)AC_TEMP > 0 && (unsigned short)AC == 0)
      {
            flag |= C;
      }
      else
	{
	  flag &= ~C;
	}
      toggleZeroFlag(AC);
      toggleNegativeFlag(AC);
      cout << "INAC16_1: AC = " << (signed short)AC << " AC8 = " << (int)(AC & 255) <<  " flag = " << (int)flag << endl;
}

void inac()
{
      unsigned char AC8 = (AC & 255);
      cout << "INAC instruction" << endl;
      const unsigned char AC8_TEMP=AC8; //Copy of the AC before the operation is performed on AC.
      AC8++;

      //Check the original AC and compare it to the result to check for overflow/carry
      //TODO?: Split this into a seperate function??
      if((signed char)AC8_TEMP > 0 && (signed char)AC8 < 0)
      {
            flag |= V;
      }
      else
	{
	  flag &= ~V;
	}
      if(AC8_TEMP > 0 && AC8 == 0)
      {
            flag |= C;
      }
      else
	{
	  flag &= ~C;
	}
      toggleZeroFlag(AC8);
      toggleNegativeFlag(AC8);


      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "INAC1: AC8 = " << (int)AC8 << " AC = " << (signed short)AC<< " flag = " << (int)flag << endl;

}


//Moves the data in AC to the R register
void mvac()
{
      unsigned char AC8 = (AC & 255);
      unsigned char R8 = (R & 255);
      cout << "MVAC instruction" << endl;
      R8 = AC8;


      R &= (unsigned short)65280;
      R |= R8;
      cout << "MVAC1: R8 = " << (int)R8 << "R = " << (signed short)R << endl;
}

void mvac16()
{
      cout << "MVAC16 instruction" << endl;
      R = AC;
      cout << "MVAC16_1: R = " << (signed short)R << " R8 = " << (int)(R & 255) << endl;
}

//Moves data in the R register to AC
void movr()
{
      unsigned char AC8 = (AC & 255);
      unsigned char R8 = (R & 255);
      cout << "MOVR instruction" << endl;
      AC8 = R8;


      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "MOVR1: AC8 = " << (int)AC8 << "AC = " << (signed short)AC << endl;
}

void movr16()
{
      cout << "MOVR16 instruction" << endl;
      AC = R;
      cout << "MOVR16_1: AC = " << (signed short)AC << "AC8 = " << (int)(AC & 255) << endl;
}

//Loads data from memory into the AC register
void ldac()
{
     unsigned char AC8 = (AC & 255);
     cout << "LDAC instruction" << endl;
     readMemory();
     PC++;
     AR++;
     cout << "LDAC1: DR = " << (int)DR << " PC = " << (int)PC << " AR = " << (int)AR << endl;

     TR = DR;
     readMemory();
     PC++;
     cout << "LDAC2: TR = " << (int)TR << " DR = " << (int)DR << " AR = " << (int)AR << " PC = " << (int)PC << endl;

     AR = TR;
     AR <<= 8;
     AR |= DR;
     cout << "LDAC3: AR = " << (int)AR << endl;

     readMemory();
     cout << "LDAC4: DR = " << (int)DR << endl;

     AC8 = DR;
     AC &= (unsigned short)65280;
     AC |= AC8;
     cout << "LDAC5: AC8 = " << (int)AC8 << " AC = " << (signed short)AC << endl;


}


void ldac16()
{

      //FIRST HALF: Load the first 8 bits into the high order of AC.
      cout << "LDAC16 instruction" << endl;
      unsigned char AC8 = (AC & 255);

      readMemory();
      PC++;
      AR++;
      cout << "LDAC16_1: DR = " << (int)DR << " PC = " << (int)PC << " AR = " << (int)AR << endl;

      TR = DR;
      readMemory();
      PC++;
      cout << "LDAC16_2: TR = " << (int)TR << " DR = " << (int)DR << " AR = " << (int)AR << " PC = " << (int)PC << endl;

      AR = TR;
      AR <<= 8;
      AR |= DR;
      cout << "LDAC16_3: AR = " << (int)AR << endl;

      readMemory();
      cout << "LDAC16_4: DR = " << (int)DR << endl;

      AC = DR;
      cout << "LDAC16_5: AC = " << (signed short)AC << endl;

      AC <<= 8;

      //SECOND HALF: Load the last 8 bits in the low order of AC.

      AR++;
      readMemory();
      cout << "LDAC16_6: DR = " << (int)DR << " AR = " << (int)AR << endl;

      AC8 = DR;
      AC |= AC8;
      cout << "LDAC16_7: AC8 = " << (int)AC8 << " AC = " << (signed short)AC <<  endl;
}

//Stores the data in AC register into memory
void stac()
{
      cout << "STAC instruction" <<  endl;
      unsigned char AC8 = (AC & 255);
      readMemory();
      PC++;
      AR++;
      cout << "STAC1: DR = " << (int)DR << " PC = " << (int)PC << " AR = " << (int)AR <<  endl;

      TR = DR;
      readMemory();
      PC++;
      cout << "STAC2: TR = " << (int)TR << " DR = " << (int)DR << " AR = " << (int)AR << " PC = " << (int)PC << endl;

      AR = TR;
      AR <<= 8;
      AR |= DR;
      cout << "STAC3: AR = " << (int)AR << endl;

      DR = AC8;
      cout << "STAC4: DR = " << (int)DR << endl;


      cout << "STAC5 before store: AR = "<< (int)AR << " M[" << (int)AR << "] = " << (int)M[AR] <<  endl;
      writeMemory();
      cout << "STAC5 after store: AR = "<< (int)AR <<" M[" << (int)AR << "] = " << (int)M[AR] <<  endl;

}


void stac16()
{
      //STORE THE HIGH ORDER BITS
      cout << "STAC16 instruction" <<  endl;
      unsigned char AC8 = (AC & 65280);
      readMemory();
      PC++;
      AR++;
      cout << "STAC16_1: DR = " << (int)DR << " PC = " << (int)PC << " AR = " << (int)AR <<  endl;

      TR = DR;
      readMemory();
      PC++;
      cout << "STAC16_2: TR = " << (int)TR << " DR = " << (int)DR << " AR = " << (int)AR << " PC = " << (int)PC << endl;

      AR = TR;
      AR <<= 8;
      AR |= DR;
      cout << "STAC16_3: AR = " << (int)AR << endl;

      DR = AC8;
      cout << "STAC16_4: DR = " << (int)DR << endl;


      cout << "STAC16_5 before store of high order: AR = "<< (int)AR << " M[" << (int)AR << "] = " << (int)M[AR] <<  endl;
      writeMemory();
      cout << "STAC16_5 after store of high order: AR = "<< (int)AR <<" M[" << (int)AR << "] = " << (int)M[AR] <<  endl;

      AC8 = (AC & 255);
      DR = AC8;
      cout << "STAC16_6: AC8 = " << (int)AC8 << " DR = " << (int)DR << endl;


      cout << "STAC16_7 before store of low order: AR = "<< (int)(AR+1) << " M[" << (int)(AR+1) << "] = " << (int)M[AR] <<  endl;
      AR++;
      writeMemory();
      cout << "STAC16_7 after store of low order: AR = "<< (int)AR <<" M[" << (int)AR << "] = " << (int)M[AR] <<  endl;
}

//Stores a given piece of data into AC register
void mvi()
{

     unsigned char AC8 = (AC & 255);
     cout << "MVI instruction" << endl;
     readMemory();
     PC++;
     AR++;
     cout << "MVI1: DR = " << (int)DR <<" PC = " << PC << " AR = " << (int)AR << endl;
     AC8 = DR;

     AC &= (unsigned short)65280;
     AC |= AC8;

     cout << "MVI2: AC8 = " << (int)AC8 << " AC = " << (signed short)AC << endl;
}

void mvi16()
{
     cout << "MVI16 instruction" << endl;

     unsigned char AC8 = 0;
     //HIGH ORDER BIT
     readMemory();
     PC++;
     AR++;
     cout << "MVI16_1: DR = " << (int)DR <<" PC = " << PC << " AR = " << (int)AR << endl;
     AC = DR;
     cout << "MVI16_2: AC = " << (int)AC << endl;

     AC <<= 8;
     cout << "MVI16_3: AC = " << (signed short)AC << endl;

     //READ IN LOW ORDER BIT
     readMemory();
     PC++;
     AR++;
     cout << "MVI16_4: DR = " << (int)DR <<" PC = " << PC << " AR = " << (int)AR << endl;
     AC8 = DR;
     cout << "MVI16_5: AC8 = " << (int)AC8 << endl;

     AC |= AC8;
     cout << "MVI16_6: AC = " << (signed short)AC << endl;

}

//and is a c++ keyword so the function is appended with _op
void and_op()
{
      unsigned char AC8 = (AC & 255);
      unsigned char R8 = (R & 255);
      //AND OPERATION
      cout << "AND instruction" << endl;
      AC8 = AC8 & R8;

      toggleZeroFlag(AC8);
      toggleNegativeFlag(AC8);


      // reassembleAC16(AC8);
      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "AND1 AC8 = " << (int)AC8 << " AC = " << (signed short)AC <<" flags = " << (int)flag << endl;
}


void and_op16()
{
      //AND OPERATION
      cout << "AND instruction" << endl;
      AC = AC & R;

      toggleZeroFlag(AC);
      toggleNegativeFlag(AC);

      cout << "AND16_1 AC = " << (signed short)AC << " AC8 = " << (int)(AC & 255) << " flags = " << (int)flag << endl;

}

void or_op()
{
      unsigned char AC8 = (AC & 255);
      unsigned char R8 = (R & 255);
      //OR OPERATION
      cout << "OR instruction" << endl;
      AC8 = AC8 | R8;

      toggleZeroFlag(AC8);
      toggleNegativeFlag(AC8);

      // reassembleAC16(AC8);
      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "OR1 AC8 = " << (int)AC8 << " AC = " << (signed short)AC << " flags = " << (int)flag << endl;

}

void or_op16()
{
      //OR OPERATION
      cout << "OR16 instruction" << endl;
      AC = AC | R;

      toggleZeroFlag(AC);
      toggleNegativeFlag(AC);

      cout << "OR16_1 AC = " << (signed short)AC << " AC8 = " << (int)(AC & 255) << " flags = " << (int)flag << endl;

}


void xor_op()
{
      unsigned char AC8 = (AC & 255);
      unsigned char R8 = (R & 255);
      //XOR OPERATION
      cout << "XOR instruction" << endl;
      AC8 = AC8 ^ R8;

      toggleZeroFlag(AC8);
      toggleNegativeFlag(AC8);


      // reassembleAC16(AC8);
      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "XOR1 AC8 = " << (int)AC8 << " AC = " << (signed short)AC << " flags = " << (int)flag << endl;
}

void xor_op16()
{
      //XOR OPERATION
      cout << "XOR16 instruction" << endl;
      AC = AC ^ R;

      toggleZeroFlag(AC);
      toggleNegativeFlag(AC);

      cout << "XOR16_1 AC = " << (signed short)AC << " AC8 = " << (int)(AC & 255) << " flags = " << (int)flag << endl;
}

void not_op()
{
      unsigned char AC8 = (AC & 255);
      unsigned char R8 = (R & 255);
      //NOT OPERATION
      cout << "NOT instruction" << endl;
      AC8 = ~AC8;

      toggleZeroFlag(AC8);
      toggleNegativeFlag(AC8);


      // reassembleAC16(AC8);
      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "NOT1 AC8 = " << (int)AC8 << " AC = " << (signed short)AC << " flags = " << (int)flag << endl;
}


void not_op16()
{
      //NOT OPERATION
      cout << "NOT16 instruction" << endl;
      AC = ~AC;

      toggleZeroFlag(AC);
      toggleNegativeFlag(AC);

      cout << "NOT16_1 AC = " << (signed short)AC << " AC8 = " << (int)(AC & 255) << " flags = " << (int)flag << endl;
}

void jump(string inst_name = "JUMP")
{
      //JUMP OPERATION
      // When calling this function the program will put the data value into the program counter.
      // This will essentially allow for the program to jump to any instruction.
      cout << "JUMP instruction" <<  endl;
      readMemory();
      AR++;
      cout << inst_name << "1: DR = " << (int)DR << " PC = " << (int)PC << " AR = " << (int)AR <<  endl;

      TR = DR;
      readMemory();
      cout << inst_name <<"2: TR = " << (int)TR << " DR = " << (int)DR << " AR = " << (int)AR << " PC = " << (int)PC << endl;

      AR = TR;
      AR <<= 8;
      AR |= DR;
      cout << inst_name <<"3: AR = " << (int)AR << endl;

      cout << inst_name <<"4 before PC overwrite: AR = "<< (int)AR << "PC = " << (int)PC <<  endl;
      PC = AR;
      cout << inst_name <<"4 after PC overwrite: AR = "<< (int)AR << "PC = " << (int)PC <<  endl;
}

void jmpz()
{
      cout << "JMPZ instruction" << endl;
      if((flag & (unsigned char)Z) == Z)
      {
            jump("JMPZ");
      }
      else
      {
            PC++;
            cout << "JMPZ1 PC = " << (int)PC << endl;
            PC++;
            cout << "JMPZ2 PC = " << (int)PC << endl;
      }
}

void jpnz()
{
      cout << "JPNZ instruction" << endl;
      if((flag & (unsigned char)Z) == 0)
      {
            jump("JPNZ");
      }
      else
      {
            PC++;
            cout << "JPNZ1 PC = " << (int)PC << endl;
            PC++;
            cout << "JPNZ2 PC = " << (int)PC << endl;
      }
}

void jmpc()
{
      cout << "JMPC instruction" << endl;
      if((flag & (unsigned char)C) == C)
      {
            jump("JMPC");
      }
      else
      {
            PC++;
            cout << "JMPC1 PC = " << (int)PC << endl;
            PC++;
            cout << "JMPC2 PC = " << (int)PC << endl;
      }
}

void jv()
{
      cout << "JV instruction" << endl;
      if((flag & (unsigned char)V) == V)
      {
            jump("JV");
      }
      else
      {
            PC++;
            cout << "JV1 PC = " << (int)PC << endl;
            PC++;
            cout << "JV2 PC = " << (int)PC << endl;
      }
}


void jn()
{
      cout << "JN instruction" << endl;
      if((flag & (unsigned char)N) == N)
      {
            jump("JN");
      }
      else
      {
            PC++;
            cout << "JN1 PC = " << (int)PC << endl;
            PC++;
            cout << "JN2 PC = " << (int)PC << endl;
      }
}

void add()
{
      cout << "ADD instruction" << endl;
      unsigned char AC8 = (AC & 255);
      unsigned char R8 = (R & 255);
      const unsigned char AC8_TEMP=AC8; //Copy of the AC before the operation is performed on AC.
      AC8 = AC8 + R8;

      toggleZeroFlag(AC8);
      toggleNegativeFlag(AC8);
      //Toggle Carry and Overflow flag HERE.

      if((signed char)AC8_TEMP >= 0)
      {
            if((signed char)R8 > ((signed char)MAX_SCHAR - (signed char)AC8_TEMP))
            {
                  flag |= V;
            }
            else
            {
                  flag &= ~V;
            }
      }
      else
	{
            if((signed char)R8 < ((signed char)MIN_SCHAR - (signed char)AC8_TEMP))
            {
	            flag |= V;
            }
            else
            {
	            flag &= ~V;
            }
	}
      if(R8 > (255 - AC8_TEMP) || AC8 < AC8_TEMP)
      {
            flag |= C;
      }
      else
	{
	  flag &= ~C;
	}


      // reassembleAC16(AC8);
      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "ADD1 AC = " << (int)AC8 << " AC = " << (signed short)AC << " flags = " << (int)flag << endl;
}


void add16()
{
      cout << "ADD16 instruction" << endl;
      const unsigned short AC_TEMP=AC; //Copy of the AC before the operation is performed on AC.
      AC = AC + R;

      toggleZeroFlag(AC);
      toggleNegativeFlag(AC);
      //Toggle Carry and Overflow flag HERE.

      if((signed short)AC_TEMP >= 0)
      {
            if((signed short)R > ((signed short)MAX_SSHORT - (signed short)AC_TEMP))
            {
                  flag |= V;
            }
            else
            {
                  flag &= ~V;
            }
      }
      else
	{
            if((signed short)R < ((signed short)MIN_SSHORT - (signed short)AC_TEMP))
            {
	            flag |= V;
            }
            else
            {
	            flag &= ~V;
            }
	}
      if((unsigned short)R > ((unsigned short)65535 - (unsigned short)AC_TEMP) || (unsigned short)AC < (unsigned short)AC_TEMP)
      {
            flag |= C;
      }
      else
	{
	  flag &= ~C;
	}

      cout << "ADD16_1 AC = " << (signed short)AC << " AC8 = " << (int)(AC & 255) << " flags = " << (int)flag << endl;
}

void sub16()
{
      cout << "SUB16 instruction" << endl;
      const unsigned short AC_TEMP=AC; //Copy of the AC before the operation is performed on AC.
      AC = AC - R;

      toggleZeroFlag(AC);
      toggleNegativeFlag(AC);
      //Toggle Carry and Overflow flag HERE.


      if((signed short)AC_TEMP >= 0)
      {
            if((signed short)R < ((signed short)AC_TEMP - (signed short)MAX_SSHORT))
            {
                  flag |= V;
            }
            else
            {
                  flag &= ~V;
            }
      }
      else
	{
            if((signed short)R > ((signed short)AC_TEMP - (signed short)MIN_SSHORT))
            {
	            flag |= V;
            }
            else
            {
	            flag &= ~V;
            }
	}

      if((unsigned short)R > (65535 - (unsigned short)AC_TEMP) || (unsigned short)AC < (unsigned short)AC_TEMP)
      {
            flag |= C;
      }
      else
	{
	  flag &= ~C;
	}
      cout << "SUB16_1 AC = " << (signed short)AC << " AC8 = " << (int)(AC & 255) << " flags = " << (int)flag << endl;
}

void sub()
{
      cout << "SUB instruction" << endl;
      unsigned char AC8 = (AC & 255);
      unsigned char R8 = (R & 255);
      const unsigned char AC8_TEMP=AC8; //Copy of the AC before the operation is performed on AC.
      AC8 = AC8 - R8;

      toggleZeroFlag(AC8);
      toggleNegativeFlag(AC8);
      //Toggle Carry and Overflow flag HERE.


      if((signed char)AC8_TEMP >= 0)
      {
            if((signed char)R8 < ((signed char)AC8_TEMP - (signed char)MAX_SCHAR))
            {
                  flag |= V;
            }
            else
            {
                  flag &= ~V;
            }
      }
      else
	{
            if((signed char)R8 > ((signed char)AC8_TEMP - (signed char)MIN_SCHAR))
            {
	            flag |= V;
            }
            else
            {
	            flag &= ~V;
            }
	}

      if(R8 > (255 - AC8_TEMP) || AC8 < AC8_TEMP)
      {
            flag |= C;
      }
      else
	{
	  flag &= ~C;
	}


      // reassembleAC16(AC8);
      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "SUB1 AC8 = " << (int)AC8 << " AC = " << (signed short)AC << " flags = " << (int)flag << endl;
}

//Rotate left
void rl()
{
      cout << "rl instruction" << endl;
      unsigned char AC8 = (AC & 255);

      const unsigned char AC8_TEMP=AC8; //Copy of the AC before the operation is performed on AC.
      AC8 = (AC8 << 1) | (AC8 >> (8 - 1));

      toggleZeroFlag(AC8);
      toggleNegativeFlag(AC8);

      //TODO: Set/reset overflow/carry flags

      if (((signed char)AC8 > (signed char)AC8_TEMP && (signed char)AC8_TEMP < 0 ) || ((signed char)AC8 < (signed char)AC8_TEMP && (signed char)AC8_TEMP > 0))
      {
            flag |= V;
      }
      else
      {
            flag &= ~V;
      }

      if(AC8 < AC8_TEMP)
      {
            flag |= C;
      }
      else
      {
            flag &= ~C;
      }


      // reassembleAC16(AC8);
      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "RL1 AC8 = " << (int)AC8 << " AC = " << (signed short)AC << " flags = " << (int)flag << endl;
}

void rl16()
{
      cout << "rl16 instruction" << endl;

      const unsigned short AC_TEMP=AC; //Copy of the AC before the operation is performed on AC.
      AC = (AC << 1) | (AC >> (16 - 1));

      toggleZeroFlag(AC);
      toggleNegativeFlag(AC);

      //TODO: Set/reset overflow/carry flags

      if (((signed short)AC > (signed short)AC_TEMP && (signed short)AC_TEMP < 0 ) || ((signed short)AC < (signed short)AC_TEMP && (signed short)AC_TEMP > 0))
      {
            flag |= V;
      }
      else
      {
            flag &= ~V;
      }

      if((unsigned short)AC < (unsigned short)AC_TEMP)
      {
            flag |= C;
      }
      else
      {
            flag &= ~C;
      }
      cout << "RL16_1 AC = " << (signed short)AC << "AC8 = " << (int)(AC & 255) << " flags = " << (int)flag << endl;
}


//Rotate right
void rr()
{
      cout << "rr instruction" << endl;
      unsigned char AC8 = (AC & 255);

      const unsigned char AC8_TEMP=AC8; //Copy of the AC before the operation is performed on AC.
      AC8 = (AC8 >> 1) |(AC8 << (8 - 1));

      toggleZeroFlag(AC8);
      toggleNegativeFlag(AC8);

      //TODO: Set/reset overflow/carry flags

      //Overflow
      if (((signed char)AC8 > (signed char)AC8_TEMP && (signed char)AC8_TEMP <= 0 ) || ((signed char)AC8 < (signed char)AC8_TEMP && (signed char)AC8 <= 0))

      {
            flag |= V;
      }
      else
      {
            flag &= ~V;
      }

      //Carry
      if(AC8 > AC8_TEMP)
      {
            flag |= C;
      }
      else
      {
            flag &= ~C;
      }

     // reassembleAC16(AC8);
     AC &= (unsigned short)65280;
     AC |= AC8;

     cout << "RR1 AC8 = " << (int)AC8 << " AC = " << (signed short)AC << " flags = " << (int)flag << endl;

}


void rr16()
{
      cout << "rr16 instruction" << endl;

      const unsigned short AC_TEMP=AC; //Copy of the AC before the operation is performed on AC.
      AC = (AC >> 1) |(AC << (16 - 1));

      toggleZeroFlag(AC);
      toggleNegativeFlag(AC);

      //TODO: Set/reset overflow/carry flags

      //Overflow
      if (((signed short)AC > (signed short)AC_TEMP && (signed short)AC_TEMP <= 0 ) || ((signed short)AC < (signed short)AC_TEMP && (signed short)AC <= 0))

      {
            flag |= V;
      }
      else
      {
            flag &= ~V;
      }

      //Carry
      if((unsigned short)AC > (unsigned short)AC_TEMP)
      {
            flag |= C;
      }
      else
      {
            flag &= ~C;
      }
      cout << "RR16_1 AC = " << (signed short)AC << " AC8 = " << (int)(AC & 255) << " flags = " << (int)flag << endl;

}

//Logical Shift right
void lsr()
{
      cout << "lsr instruction" << endl;

      unsigned char AC8 = (AC & 255);
      const unsigned char AC8_TEMP=AC8; //Copy of the AC before the operation is performed on AC.
      AC8 = AC8 >> 1;

      toggleZeroFlag(AC8);
      toggleNegativeFlag(AC8);

      //TODO: Set/reset overflow/carry flags

      //Overflow
      if ((signed char)AC8_TEMP > 0 && (signed char)AC8 < 0 || (signed char)AC8_TEMP < 0 && (signed char)AC8 > 0)
      {
            flag |= V;
      }
      else
      {
            flag &= ~V;
      }

      //Carry
      if (AC8_TEMP % 2 == 1)
      {
            flag |= C;
      }
      else
      {
            flag &= ~C;
      }


      // reassembleAC16(AC8);
      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "LSR1 AC8 = " << (int)AC8 << " AC = " << (signed short)AC << " flags = " << (int)flag << endl;
}


void lsr16()
{
      cout << "lsr16 instruction" << endl;

      const unsigned short AC_TEMP=AC; //Copy of the AC before the operation is performed on AC.
      AC = AC >> 1;

      toggleZeroFlag(AC);
      toggleNegativeFlag(AC);

      //TODO: Set/reset overflow/carry flags

      //Overflow
      if ((signed short)AC_TEMP > 0 && (signed short)AC < 0 || (signed short)AC_TEMP < 0 && (signed short)AC > 0)
      {
            flag |= V;
      }
      else
      {
            flag &= ~V;
      }

      //Carry
      if ((unsigned short)AC_TEMP % 2 == 1)
      {
            flag |= C;
      }
      else
      {
            flag &= ~C;
      }
      cout << "LSR16_1 AC = " << (signed short)AC << " AC8 = " << (int)(AC & 255) << " flags = " << (int)flag << endl;

}

//Logical shift left
void lsl()
{
      cout << "lsl instruction" << endl;

      unsigned char AC8 = (AC & 255);
      const unsigned char AC8_TEMP=AC8; //Copy of the AC before the operation is performed on AC.
      AC8 = AC8 << 1;

      toggleZeroFlag(AC8);
      toggleNegativeFlag(AC8);

      //TODO: Set/reset overflow/carry flags

      //Overflow
      if ((signed char)AC8_TEMP > 0 && (signed char)AC8 <= 0 || (signed char)AC8_TEMP < 0 && (signed char)AC8 >= 0)

      {
            flag |= V;
      }
      else
      {
            flag &= ~V;
      }

      //Carry
      if (AC8 < AC8_TEMP)
      {
            flag |= C;
      }
      else
      {
            flag &= ~C;
      }


      // reassembleAC16(AC8);
      AC &= (unsigned short)65280;
      AC |= AC8;
      cout << "RSR1 AC8 = " << (int)AC8 << " AC = " << (signed short)AC << " flags = " << (int)flag << endl;
}


void lsl16()
{
      cout << "lsl16 instruction" << endl;

      const unsigned short AC_TEMP=AC; //Copy of the AC before the operation is performed on AC.
      AC = AC << 1;

      toggleZeroFlag(AC);
      toggleNegativeFlag(AC);

      //TODO: Set/reset overflow/carry flags

      //Overflow
      if ((signed short)AC_TEMP > 0 && (signed short)AC <= 0 || (signed short)AC_TEMP < 0 && (signed short)AC >= 0)

      {
            flag |= V;
      }
      else
      {
            flag &= ~V;
      }

      //Carry
      if ((unsigned short)AC < (unsigned short)AC_TEMP)
      {
            flag |= C;
      }
      else
      {
            flag &= ~C;
      }
      cout << "RSR16_1 AC = " << (signed short)AC << " AC8 = " << (int)(AC & 255) << " flags = " << (int)flag << endl;
}

//Function that decodes and executes the instruction given
void decodeInstruction(int code)
{
      switch(code)
      {
            //Define the backwards compatible registers here so they can be printed out at the end
            //of the function


            //These are the original 8 bit versions of the functions.
            //NOP
            case 0:
                  nop();
                  break;
            case 1:
                  ldac();
                  break;
            case 2:
                  stac();
                  break;
            //MVAC
            case 3:
                  mvac();
                  break;
            //MOVR
            case 4:
                  movr();
                  break;
            case 5:
                  jump();
                  break;
            case 6:
                  jmpz();
                  break;
            case 7:
                  jpnz();
                  break;
            case 8:
                  add();
                  break;
            case 9:
                  sub();
                  break;
            //INAC
            case 10:
                  inac();
                  break;
            //CLAC
            case 11:
                  clac();
                  break;
            case 12:
                  and_op();
                  break;
            case 13:
                  or_op();
                  break;
            case 14:
                  xor_op();
                  break;
            case 15:
                  not_op();
                  break;
            case 16:
                  jmpc();
                  break;
            case 17:
                  jv();
                  break;
            case 18:
                  rl();
                  break;
            case 19:
                  rr();
                  break;
            case 20:
                  lsl();
                  break;
            case 21:
                  lsr();
                  break;
            case 22:
                  mvi();
                  break;
            case 23:
                  jn();
                  break;
            //BEGIN 16 BIT VERSIONS

            //These are the 16-bit version of the cpu instruction.
            case 129: //LDAC
                  ldac16();
                  break;
            case 130: //STAC
                  stac16();
                  break;
            case 131: //MVAC
                  mvac16();
                  break;
            case 132: //MOVR
                  movr16();
                  break;
            case 136: //ADD
                  add16();
                  break;
            case 137: //SUB
                  sub16();
                  break;
            case 138: //INAC
                  inac16();
                  break;
            case 139: //CLAC
                  clac16();
                  break;
            case 140: //AND
                  and_op16();
                  break;
            case 141: //OR
                  or_op16();
                  break;
            case 142: //XOR
                  xor_op16();
                  break;
            case 143: //NOT
                  not_op16();
                  break;
            case 146: //RL
                  rl16();
                  break;
            case 147: //RR
                  rr16();
                  break;
            case 148: //LSL
                  lsl16();
                  break;
            case 149: //LSR
                  lsr16();
                  break;
            case 150: //MVI
                  mvi16();
                  break;

            //HALT
            case 255:
                  halt();
                  break;

            //Stop the execution of the program immediately if an unknown opcode is read.
            default:
                  cout << "BAD INSTRUCTION!!! STOP EXECUTION!!!" << endl;
                  halt();
                  break;
      }

      unsigned char AC8 = (AC & 255);
      unsigned char R8 = (R & 255);
      cout << "Instruction execution complete: AC = " << (int)AC << " AC8 = " << (int)AC8 << " R = " << (int)R << " R8 = " << (int)R8 << " flag = " << (int)flag << " AR = " << (int)AR << " PC = " << (int)PC << " DR = " << (int)DR << endl;
      cout << endl << "number of hits = " << cacheHits << endl;
      cout << "number of misses = " << cacheMisses << endl;
}


void fetchAndDecode()
{
      //Open source file

      //Fetch decode loop
      while(is_executing)
      {
            AR = PC;
            cout << "fetch 1: AR = " << (int)AR << " and PC = " << (int)PC << endl;

            readMemory();
            PC++;
            cout << "fetch 2: DR = " << (int)DR << " and PC = " << (int)PC << endl;

            IR = DR;
            AR = PC;
            cout << "fetch 3: IR = " << (int)IR << " and AR = " << (int)AR << endl;
            decodeInstruction(IR); //Weird to decode before fetch, but the first instruction is skipped if decode is last
      }
}


int main()
{
      //Print out the header(s) as required by the directions in the homework
      cout << "Name: Ryan Jaskulski\nDescription: rscpue with 1KB direct-mapped, write through, write allocate cache.\n\n";

      string programFile;
      cout << "Enter the name of the file:  ";
      cin >> programFile;
      cout << "The program located at " << programFile << " has been properly opened; Dumping source to memory" << endl;

      initMemory(); //Initalize all memory addresses to 0 so that nothing weird happens with uninitalized valuesw
      cout << "Memory has been initalized" << endl;
      initCache();
      cout << "Dumping source to memory" << endl;
      dumpSourceToMemory(programFile);
      cout << "BEGIN" << endl;
      fetchAndDecode();
}
