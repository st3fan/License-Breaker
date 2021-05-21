// =====================================================================================================================

//
// File        : ElfUtils.cpp
// Project     : LicenseBreaker
// Author      : Stefan Arentz, stefan.arentz@soze.com
// Version     : $Id$
// Environment : BeOS 4.5/Intel
//

// =====================================================================================================================

#include <posix/stdio.h>
#include <posix/stdlib.h>
#include <posix/string.h>
#include <posix/fcntl.h>
#include <posix/errno.h>
#include <posix/sys/types.h>
#include <posix/sys/stat.h>

#include <be/support/SupportDefs.h>

#include "ElfUtils.h"

// =====================================================================================================================

#define ELF_ST_BIND(val)                (((unsigned int)(val)) >> 4)
#define ELF_ST_TYPE(val)                ((val) & 0xF)

#define SHT_SYMTAB		2			/* Link editing symbol table */
#define SHT_STRTAB		3			/* A string table */
#define SHT_DYNSYM		11			/* Dynamic linker symbol table */

#define STB_LOCAL       0               /* Symbol not visible outside obj */
#define STB_GLOBAL      1               /* Symbol visible outside obj */
#define STB_WEAK        2               /* Like globals, lower precedence */
#define STB_LOPROC      13              /* Application-specific semantics */
#define STB_HIPROC      15              /* Application-specific semantics */

#define STT_NOTYPE      0               /* Symbol type is unspecified */
#define STT_OBJECT      1               /* Symbol is a data object */
#define STT_FUNC        2               /* Symbol is a code object */
#define STT_SECTION     3               /* Symbol associated with a section */
#define STT_FILE        3               /* Symbol gives a file name */

#define STT_LOPROC      13              /* Application-specific semantics */
#define STT_HIPROC      15              /* Application-specific semantics */

// =====================================================================================================================

typedef struct {
  unsigned char e_ident[16];            /* ELF "magic number" */
  unsigned char e_type[2];              /* Identifies object file type */
  unsigned char e_machine[2];           /* Specifies required architecture */
  unsigned char e_version[4];           /* Identifies object file version */
  unsigned char e_entry[4];             /* Entry point virtual address */
  unsigned char e_phoff[4];             /* Program header table file offset */
  unsigned char e_shoff[4];             /* Section header table file offset */
  unsigned char e_flags[4];             /* Processor-specific flags */
  unsigned char e_ehsize[2];            /* ELF header size in bytes */
  unsigned char e_phentsize[2];         /* Program header table entry size */
  unsigned char e_phnum[2];             /* Program header table entry count */
  unsigned char e_shentsize[2];         /* Section header table entry size */
  unsigned char e_shnum[2];             /* Section header table entry count */
  unsigned char e_shstrndx[2];          /* Section header string table index */
} Elf32_External_Ehdr;

typedef struct {
  unsigned char sh_name[4];             /* Section name, index in string tbl */
  unsigned char sh_type[4];             /* Type of section */
  unsigned char sh_flags[4];            /* Miscellaneous section attributes */
  unsigned char sh_addr[4];             /* Section virtual addr at execution */
  unsigned char sh_offset[4];           /* Section file offset */
  unsigned char sh_size[4];             /* Size of section in bytes */
  unsigned char sh_link[4];             /* Index of another section */
  unsigned char sh_info[4];             /* Additional section information */
  unsigned char sh_addralign[4];        /* Section alignment */
  unsigned char sh_entsize[4];          /* Entry size if section holds table */
} Elf32_External_Shdr;

typedef struct {
  unsigned char st_name[4];             /* Symbol name, index in string tbl */
  unsigned char st_value[4];            /* Value of the symbol */
  unsigned char st_size[4];             /* Associated symbol size */
  unsigned char st_info[1];             /* Type and binding attributes */
  unsigned char st_other[1];            /* No defined meaning, 0 */
  unsigned char st_shndx[2];            /* Associated section index */
} Elf32_External_Sym;

// =====================================================================================================================

//
// GetFunctionsFromElfObject
//
//	Parse an ELF32 object file to retrieve the symbol tables and the string table. Caller should release
//	the functions and stringtable.
//

int GetSymbolTable(char* filename, symbol_table_t** outSymbols, char** outMessage)
{
	Elf32_External_Ehdr		fileHeader;
	int						i, j;
	FILE*					file;
	size_t					n;
	Elf32_External_Sym*		symbolTableData[128];
	int						symbolTableSizes[128];
	int						symbolTableCount;
	off_t					offset;
	unsigned long			sectionTableOffset;
	unsigned short			sectionCount;
	char*					dynamicStrings;
	function_t*				functions = NULL;
	int						functionCount = 0;
	bool					result;
	
	result = false;
	*outSymbols = NULL;
	*outMessage = "";

	try {

		file = fopen(filename, "rb");
		if (file == NULL) {
			return 1;
		}
	
		//
		// Read the ELF header
		//
	
		n = fread(&fileHeader, sizeof(fileHeader), 1, file);
	
		sectionTableOffset = *((unsigned long*) fileHeader.e_shoff);
		sectionCount = *((unsigned short*) fileHeader.e_shnum);
	
		//printf("Section table offset: 0x%08x\n", sectionTableOffset);
		//printf("Section header count: 0x%08x\n", sectionCount);
	
		//
		// Read all the sections until we find a symbol table. We then
		// read the symbol table into memory and build a list of all the
		// function symbols.
		//
	
		symbolTableCount = 0;
	
		offset = fseek(file, sectionTableOffset, SEEK_SET);
	
		for (i = 0; i < sectionCount; i++)
		{
			Elf32_External_Shdr sectionHeader;
			unsigned int sectionType;
			unsigned long sectionOffset;
			unsigned long sectionSize;
			unsigned long sectionEntrySize;
			unsigned long sectionEntryCount;
	
			//
			// Read the section header
			//
	
			fseek(file, sectionTableOffset + (i * sizeof(Elf32_External_Shdr)), SEEK_SET);
			n = fread(&sectionHeader, sizeof(sectionHeader), 1, file);
	
			//printf("fluts1\n");
	
			//
			// Get all the fields we need
			//
	
			sectionType = *((unsigned long*) sectionHeader.sh_type);
			sectionOffset = *((unsigned long*) sectionHeader.sh_offset);
			sectionSize = *((unsigned long*) sectionHeader.sh_size);
			sectionEntrySize = *((unsigned long*) sectionHeader.sh_entsize);
	
			//printf("Section %d = type 0x%08x\n", i, sectionType);
	
			if (sectionType == SHT_SYMTAB)
			{
				//
				// Allocate a symbol table
				//
		
				sectionEntryCount = (sectionSize / sectionEntrySize);
	
				//printf("Allocating %d symbol records of size %d\n", sectionEntryCount, sizeof(Elf32_External_Sym));
				symbolTableData[symbolTableCount] = (Elf32_External_Sym*) malloc(sectionEntryCount * sizeof(Elf32_External_Sym));
				symbolTableSizes[symbolTableCount] = sectionEntryCount;
	
				//
				// Read the table into memory
				//
	
				offset = fseek(file, sectionOffset, SEEK_SET);
				n = fread(symbolTableData[symbolTableCount], sizeof(Elf32_External_Sym), sectionEntryCount, file);
	
				symbolTableCount++;
			}
			else if (sectionType == SHT_STRTAB)
			{
				//
				// Read the string table
				//
	
				dynamicStrings = (char*) malloc(sectionSize);
	
				offset = fseek(file, sectionOffset, SEEK_SET);
				n = fread(dynamicStrings, sectionSize, 1, file);
			}
		}

		//
		// If we didn't find a symbol table then bail out
		//
		
		if (symbolTableCount == 0)
		{
			throw "No symbol table found";
		}

		//
		// If we didn't find a string table then bail out
		//
		
		if (dynamicStrings == NULL)
		{
			throw "No symbols in file";
		}
		
		//
		// Find the functions that we are interested in. First we count how many there
		// are and then we allocate an array for them and copy 'm in.
		//
	
		for (i = 0; i < symbolTableCount; i++)
		{
			for (j = 0; j < symbolTableSizes[i]; j++)
			{
				unsigned long st_value = *((unsigned long*) symbolTableData[i][j].st_value);
				unsigned char st_info = symbolTableData[i][j].st_info[0];
	
				// We're only interested in functions that are in the binary. (XXX Is the value == 0 check okay?)
				if (st_value != 0 && (ELF_ST_BIND(st_info) == STB_GLOBAL) && (ELF_ST_TYPE(st_info) == STT_FUNC)) {
					functionCount++;
				}
			}
		}
	
		if (functionCount != 0)
		{
			function_t* f;
	
			functions = f = (function_t*) malloc(functionCount* sizeof(function_t));
	
			for (i = 0; i < symbolTableCount; i++)
			{
				for (j = 0; j < symbolTableSizes[i]; j++)
				{
					unsigned long st_name = *((unsigned long*) symbolTableData[i][j].st_name);
					unsigned long st_value = *((unsigned long*) symbolTableData[i][j].st_value);
					unsigned long st_size = *((unsigned long*) symbolTableData[i][j].st_size);
					unsigned char st_info = symbolTableData[i][j].st_info[0];
	
					if (st_value != 0 && (ELF_ST_BIND(st_info) == STB_GLOBAL) && (ELF_ST_TYPE(st_info) == STT_FUNC))
					{
						if (st_value != 0 && (ELF_ST_BIND(st_info) == STB_GLOBAL) && (ELF_ST_TYPE(st_info) == STT_FUNC))
						{
							f->offset = st_value;
							f->name = dynamicStrings + st_name;
							f->size = st_size;
						}
	
						f++;
					}
				}
			}
		}
	
		//
		// Store the result
		//

		*outSymbols = (symbol_table_t*) malloc(sizeof(symbol_table_t));
		if (*outSymbols == NULL) {
			throw "Memory Exhausted. Get More!";
		}
		
		(*outSymbols)->functions = functions;
		(*outSymbols)->count = functionCount;
		(*outSymbols)->strings = dynamicStrings;
		
		result = true;
	}
	
	catch (char* message) {
		*outMessage = message;
		result = false;
	}
	
	//
	// Free the symbol tables that we read
	//

	if (symbolTableCount != 0) {
		for (i = 0; i < symbolTableCount; i++) {
			free(symbolTableData[i]);
		}
	}

	(void) fclose(file);

	return result;
}

// =====================================================================================================================

#ifdef TEST
int main()
{
	char* error;
	symbol_table_t* symbols;
	
	GetSymbolTable("/boot/beos/system/kernel_intel", &symbols, &error);
	
	printf("Count = %d\n", symbols->count);

	function_t* mCurrentFunction = &(symbols->functions)[(rand() + clock()) % symbols->count];

	printf("%s\n", mCurrentFunction->name);
	
	for (int i = 0; i < symbols->count; i++)
	{
		printf("%08d %s\n", i, (symbols->functions)[i].name);
	}
}
#endif
