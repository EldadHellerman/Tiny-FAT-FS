/*
 * Project: Tiny_FAT_FS
 * File: Tiny_FAT_FS.h
 * Created on: 14.12.2018
 * Author: Eldad Hellerman
 *
 */

#ifndef TINY_FAT_FS_H_
#define TINY_FAT_FS_H_

/*
 * file and directory names are still not implemented
 *
 * The following operations should exist:
 *  *-list files and directories
 *  *-list a file's size
 *  *-list amount of entries in a directory
 *  *-list amount of free memory
 *  *-enter a directory
 *  -enter a given path
 *  *-find a file - in current dir, from a given name to a file pointer
 *  *-read a file
 *  *-write to a file
 *  *-append to a file
 *  *-create a file
 *  *-create a directory
 *  -set/change a file's name
 *  -set/change a directory's name
 *  *-delete a file
 *  *-delete a folder recursively
 *  *-'compacting a folder' - move all entries to be one after another (fill any holes left from deleting entries), and then delete any empty pages left
 *  *-finding first page of folder from one of its pages.
 *
 *  and combinations such as:
 *  -copy a file
 *  -move a file
 *  -copy a folder recursively
 *  -move a folder recursively
 *
 */

#define MEMORY_SIZE_1024

#include "stdio.h"
#define root_directory 1

typedef unsigned char byte;
typedef byte Tiny_FS[1000];
typedef byte page_pointer;
typedef page_pointer directory_pointer;
typedef struct{
	page_pointer page;	//points to the directory page the file is in (not to the page of the first entire directory structure)
	byte entry;			//entry number, 0-6
} file_pointer;


//////////////////////////////////////////////////////////////////////////////////////
///  next structures are for internal use, and should probably be moved to anther  ///
///  file, like Tiny_FS.c, or the rest moved to Tiny_FS_interface.h or something   ///
//////////////////////////////////////////////////////////////////////////////////////

struct directory_entry_pointer{
	byte address : 7;
	byte is_folder : 1;
};

struct back_pointer{
	byte address : 7;
	byte first_page : 1;
};

struct directory_page_end_pointer{
	byte address_of_next : 7;
	byte is_name : 1;
};

struct file_page_end_pointer{
	byte address_of_next : 7;
	byte is_name : 1;
};

struct directory_entry{
	struct directory_entry_pointer directory_pointer;
	byte file_number;
};

struct directory{
	struct directory_entry entries[7];
	struct back_pointer back_pointer;
	struct directory_page_end_pointer end_pointer;
};

void init_FS(Tiny_FS fs);
void list_files(Tiny_FS fs, directory_pointer dir, byte max_level);
int file_size(Tiny_FS fs, file_pointer file);

file_pointer create_entry(Tiny_FS fs, directory_pointer dir, byte file_number, byte is_folder);	//returns pointer to page in which file is.
#define create_file(fs, dir, file_number)		create_entry(fs, dir, file_number, 0)
#define create_folder(fs, dir, file_number)		create_entry(fs, dir, file_number, 1)

directory_pointer enter_folder(Tiny_FS fs, file_pointer folder);

int write_to_file(Tiny_FS fs, file_pointer file, byte *buffer, int size); //returns amount of bytes written (maybe memory run out)
int append_to_file(Tiny_FS fs, file_pointer file, byte *buffer, int size); //returns amount of bytes appended (maybe memory run out)
int read_from_file(Tiny_FS fs, file_pointer file, byte *buffer, int max_size); //returns amount of bytes read (maybe reached end of file)
void delete_file(Tiny_FS fs, file_pointer file);
void delete_folder(Tiny_FS fs, file_pointer folder);
#define delete(fs,file)	if((*(struct directory *)&fs[file.page<<4]).entries[file.entry].directory_pointer.is_folder) delete_folder(fs, file); else delete_file(fs, file)

int amount_of_free_memory(Tiny_FS fs);
int amount_of_entries(Tiny_FS fs, directory_pointer dir);

file_pointer find_file(Tiny_FS fs, directory_pointer dir, byte file_number);
directory_pointer get_folder_start(Tiny_FS fs, directory_pointer dir);
void compact_folder(Tiny_FS fs, directory_pointer dir);

#endif /* TINY_FAT_FS_H_ */
