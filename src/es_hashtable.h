// ===================================================================
// Copyright (C) 2010 - Eventscripts Development Team
// File: es_hashtable.h
// Purpose: A hash table class for use with data structures.
// ===================================================================
#ifndef _ES_HASHTABLE_H
#define _ES_HASHTABLE_H

// ===================================================================
// Includes
// ===================================================================
#include <utlhash.h>

// ===================================================================
// Useful constants.
// ===================================================================
#define HASH 5381

// ===================================================================
// A single hash data element.
// ===================================================================
struct HashData_t
{
	const char* name;
	void* value;
};

// ===================================================================
// The hash table class.
// ===================================================================
class CESHashTable
{
	private:
		// -----------------------------------------
		// The hash table in which to store our
		// data.
		// -----------------------------------------
		CUtlHash<HashData_t> m_HashTable;

	public:
		// -----------------------------------------
		// @brief Constructor.
		// -----------------------------------------
		CESHashTable( void );

		// -----------------------------------------
		// @brief The hash compare function.
		// -----------------------------------------
		static bool HashDataCompare(const HashData_t &p1, 
									const HashData_t &p2);

		// -----------------------------------------
		// @brief Returns a key to the given data.
		// -----------------------------------------
		static unsigned int HashKeyFunc(const HashData_t &p);

		// -----------------------------------------
		// @brief Returns a value by name.
		// -----------------------------------------
		void* FindElement(const char* name);

		// -----------------------------------------
		// @brief Inserts an element into the hash
		//	table.
		// -----------------------------------------
		void InsertElement(const HashData_t *p);

		// -----------------------------------------
		// @brief Removes an element by name.
		// -----------------------------------------
		bool RemoveElement(const char* name);
};

// ===================================================================
// These are all global hash tables.
// ===================================================================
extern CESHashTable g_SayCmdTable;
extern CESHashTable g_ClientCmdTable;
extern CESHashTable g_CmdTable;
extern CESHashTable g_StackTable;

#endif // _ES_HASHTABLE_H

