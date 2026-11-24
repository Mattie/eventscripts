// ===================================================================
// Copyright (C) 2010 - Eventscripts Development Team
// File: es_hashtable.cpp
// Purpose: Implementation of CESHashTable.
// ===================================================================

// ===================================================================
// Includes
// ===================================================================
#include "es_hashtable.h"
#include "es_strutils.h"

// ===================================================================
// These are all global hash tables.
// ===================================================================
CESHashTable g_SayCmdTable;
CESHashTable g_ClientCmdTable;
CESHashTable g_CmdTable;
CESHashTable g_StackTable;

// ===================================================================
// Constructor.
// ===================================================================
CESHashTable::CESHashTable() : m_HashTable(65536,0,0, &CESHashTable::HashDataCompare, 
										   &CESHashTable::HashKeyFunc)
{
	// Do nothing.
}

// ===================================================================
// Comparison function.
// ===================================================================
bool CESHashTable::HashDataCompare(const HashData_t &p1, const HashData_t &p2)
{
	return (FStrEq(p1.name, p2.name));
}

// ===================================================================
// Generates a hash for data.
// ===================================================================
unsigned int CESHashTable::HashKeyFunc(const HashData_t &p)
{
	const char* str = p.name;

	unsigned int hash = HASH;
	int c;

	while( c = *str++ )
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

// ===================================================================
// Finds an element in the hash table.
// ===================================================================
void* CESHashTable::FindElement(const char* name)
{
	if( !name ) {
		return NULL;
	}

	// -----------------------------------------
	// Create a temp struct to find data.
	// -----------------------------------------
	HashData_t dataToFind;
	dataToFind.name = name;
	dataToFind.value = NULL;

	// -----------------------------------------
	// Have our hash table find it.
	// -----------------------------------------
	UtlHashHandle_t hKey = m_HashTable.Find(dataToFind);

	// -----------------------------------------
	// Make sure that the key is valid.
	// -----------------------------------------
	if( hKey != m_HashTable.InvalidHandle() )
	{
		// -----------------------------------------
		// Done!
		// -----------------------------------------
		return m_HashTable.Element(hKey).value;
	}

	// -----------------------------------------
	// If we are here, it means hKey was 
	// invalid.
	// -----------------------------------------
	return NULL;
}

// ===================================================================
// Inserts an element into the hash table.
// ===================================================================
void CESHashTable::InsertElement(const HashData_t *p)
{
	// -----------------------------------------
	// Insert the element into the hash table.
	// -----------------------------------------
	m_HashTable.Insert(*p);
}

// ===================================================================
// Removes an element from the hash table.
// ===================================================================
bool CESHashTable::RemoveElement(const char* name)
{
	if( !name ) {
		return false;
	}

	// -----------------------------------------
	// Create a temporary data struct to find
	// the hash data.
	// -----------------------------------------
	HashData_t dataToFind;
	dataToFind.name = name;
	dataToFind.value = NULL;

	// -----------------------------------------
	// Find a key to the data (if it exists).
	// -----------------------------------------
	UtlHashHandle_t hKey = m_HashTable.Find(dataToFind);

	// -----------------------------------------
	// Make sure it's valid.
	// -----------------------------------------
	if( hKey != m_HashTable.InvalidHandle() )
	{
		// -----------------------------------------
		// Remove it.
		// -----------------------------------------
		m_HashTable.Remove(hKey);

		// -----------------------------------------
		// Done!
		// -----------------------------------------
		return true;
	}

	// -----------------------------------------
	// If we're here, it means we didn't find
	// the data in the table.
	// -----------------------------------------
	return false;
}

