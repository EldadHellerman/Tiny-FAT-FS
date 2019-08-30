/*
 * Project: Tiny_FAT_FS
 * File: Tiny_FAT_FS.c
 * Created on: 14.12.2018
 * Author: Eldad Hellerman
 *
 */

#include "Tiny_FAT_FS.h"

#define directory_to_address(dir)		(dir<<4)
#define get_directory(fs,dir)			(*(struct directory *)&fs[directory_to_address(dir)])
#define get_entry(fs,file)				(*(struct directory_entry *)&fs[(file.page<<4) + (file.entry<<1)])
#define page_alloc(fs,page)				fs[page>>3] &= ~(0x80 >> (page & 0x07))
#define page_free(fs,page)				fs[page>>3] |=  (0x80 >> (page & 0x07))
#define clear_page(fs,page);			for(byte i=0; i<16; i++){ fs[(page<<4) + i] = 0;}

static page_pointer allocate_page(Tiny_FS fs){ //returns 0 if there no page available for allocation
	byte location = 0;
	do{ if(fs[location] != 0) break; }while(++location < 16);
	if(location == 16) return((page_pointer)0);
	//location now equals the location of a byte with a free page
	byte byte_index = fs[location];
	byte bit_index = 0x07; //finds first bit from the left which is 1, MSB is bit_index 0, LSB is bit_index 7.
	if(byte_index & 0xF0){ bit_index &= ~4; byte_index >>= 4;}
	if(byte_index & 0x0C){ bit_index &= ~2; byte_index >>= 2;}
	if(byte_index & 0x02) bit_index &= ~1;

	fs[location] &= ~(0x80 >> bit_index);
	//printf("location is %d\n",location);
	page_pointer page = (location<<3) | bit_index;
	clear_page(fs,page);
	return(page);
}

static void delete_from_page(Tiny_FS fs, page_pointer page){
	//may be problematic because it's reading data from an already erased page
	//page_pointer next_page
	while(page != 0){
		/*next_page = fs[(page<<4) + 15];
			fs[(page<<4) + 15] = 0;
			page_free(fs,page);
			page = next_page;*/
		page_free(fs,page);
		page = fs[(page<<4) + 15];
	}
}

void init_FS(Tiny_FS fs){
	fs[0] = 0x3F;							///all of the memory is free
	for(byte i=1; i<16; i++) fs[i] = 0xFF;	//(except first two pages),
	#ifdef MEMORY_SIZE_1024
		for(byte i=8; i<16; i++) 	fs[i] = 0; //for 1024 memory size, setting second half of first page to taken.
	#endif
	for(int i=16; i<32; i++)	fs[i] = 0; //root directory
	get_directory(fs,root_directory).back_pointer.first_page = 1;
	get_directory(fs,root_directory).back_pointer.address = 0;
	for(int i=32; i<1024; i++)	fs[i] = 0; //rest is set to zero for convenience
	//for(int i=32; i<1024; i++)	fs[i] = rand(); //TODO try and intialize memory to random values - it should not affect any of the operations.

	/*//adding files for testing:
	file_pointer f = create_folder(fs,root_directory,2);
	directory_pointer dir = enter_folder(fs, f);
	create_folder(fs,dir,3); create_file(fs,dir,4);
	f = create_folder(fs,dir,50);
	for(byte name=100; name<105; name++) create_file(fs,enter_folder(fs, f),name);
	for(byte i=10 ;i<=20; i++) create_file(fs,root_directory,i);*/
}

static void list_files_inner(Tiny_FS fs, directory_pointer dir, byte level, byte max_level){
	while(dir != 0){
		for(byte i=0; i<7; i++){
			struct directory_entry entry = get_directory(fs,dir).entries[i];
			if(entry.directory_pointer.address == 0) continue;
			for(byte i=0; i<level; i++) printf("    ");
			//for(byte i=0; i<level-1; i++) printf("   |"); printf("    ");
			if(entry.directory_pointer.is_folder){
				printf("%d\\\n",entry.file_number);
				//printf("%d\\ (%d)\n",entry.file_number,(entry.directory_pointer.address == 1) ? 0 : amount_of_entries(fs, entry.directory_pointer.address));
				if(entry.directory_pointer.address != 1 && level < max_level) list_files_inner(fs,entry.directory_pointer.address,level+1, max_level);
			}else{
				printf("%d\n",entry.file_number);
			}
		}
		dir = get_directory(fs,dir).end_pointer.address_of_next;
	}
}

void list_files(Tiny_FS fs, directory_pointer dir, byte max_level){
	for(byte i=0; i<30; i++) printf("-");
	printf("\n%d bytes are free (%d pages used)\n", amount_of_free_memory(fs),(1024-amount_of_free_memory(fs))/16);
	printf("\\\n");
	list_files_inner(fs,dir,1,max_level);
	for(byte i=0; i<30; i++) printf("-");
	printf("\n");
}

file_pointer create_entry(Tiny_FS fs, directory_pointer dir, byte file_number, byte is_folder){
	file_pointer result;
	while(1){
		for(result.entry=0; result.entry<7; result.entry++) if(get_directory(fs,dir).entries[result.entry].directory_pointer.address == 0) break;
		if(result.entry == 7){ //no available entries here, go to next page
			if(get_directory(fs,dir).end_pointer.address_of_next == 0) break; //there isn't a next page, continue to allocation
			else dir = get_directory(fs,dir).end_pointer.address_of_next;
		}else{ //found an available entry
			get_directory(fs,dir).entries[result.entry].file_number = file_number;
			get_directory(fs,dir).entries[result.entry].directory_pointer.is_folder = is_folder;
			get_directory(fs,dir).entries[result.entry].directory_pointer.address = 1;
			result.page = dir;
			return(result);
		}
	}
	//if here, there is no available entry and a new page should be allocated
	result.entry = 0;
	result.page = allocate_page(fs);
	if(result.page == 0) return(result); //if there is no available memory
	get_directory(fs,result.page).back_pointer.first_page = 0;
	get_directory(fs,result.page).back_pointer.address = dir;
	get_directory(fs,result.page).end_pointer.address_of_next = 0;
	//get_directory(fs,dir).back_pointer = 0; //can free up current back pointer for other uses
	get_directory(fs,dir).end_pointer.address_of_next = result.page;

	get_directory(fs,result.page).entries[0].file_number = file_number;
	get_directory(fs,result.page).entries[0].directory_pointer.is_folder = is_folder;
	get_directory(fs,result.page).entries[0].directory_pointer.address = 1;
	return(result);
}

int write(Tiny_FS fs, page_pointer page, byte byte_index, byte *buffer, int size){
	int wrote = 0;
	while(wrote < size){
		if(byte_index == 15){ //wrote an entire page (15 bytes)
			if(fs[(page<<4) + byte_index] & 0xFE){ //if there is already another page allocated
				page = fs[(page<<4) + byte_index];
			}else{
				page_pointer new_page = allocate_page(fs);
				if(new_page == 0) break;
				fs[(page<<4) + byte_index] = new_page; //set file_end_pointer to new address
				page = new_page;
			}
			byte_index = 0;
		}
		fs[(page<<4) + byte_index] = buffer[wrote];
		byte_index++;
		wrote++;
	}
	byte original_file_end_pointer = fs[(page<<4) + 15];
	if(byte_index == 15){ //wrote 15 bytes in last page
		fs[(page<<4) + byte_index] = 1; //set file_end_pointer to signal there are 15 bytes of data in that page.
	}else{
		fs[(page<<4) + 14] = byte_index; //set the 15'th byte to amount of bytes in that page.
		fs[(page<<4) + 15] = 0; //set file_end_pointer to signal there are less than 14 bytes of data in that page.
	}
	if(original_file_end_pointer & 0xFE) delete_from_page(fs, original_file_end_pointer); //delete any leftover pages
	return(wrote);
}

int write_to_file(Tiny_FS fs, file_pointer file, byte *buffer, int size){
	if(file.page == 0 || get_entry(fs,file).directory_pointer.address == 0 || get_entry(fs,file).directory_pointer.is_folder) return(0); //non existing file, or folder
	page_pointer page = get_entry(fs,file).directory_pointer.address;
	if(page == 1){
		page = allocate_page(fs);
		if(page == 0) return(0);
		get_entry(fs,file).directory_pointer.address = page;
	}
	return(write(fs, page, 0, buffer, size));
}

int append_to_file(Tiny_FS fs, file_pointer file, byte *buffer, int size){
	if(file.page == 0 || get_entry(fs,file).directory_pointer.address == 0 || get_entry(fs,file).directory_pointer.is_folder) return(0); //non existing file, or folder
	page_pointer page = get_entry(fs,file).directory_pointer.address;
	if(page == 1) return(write_to_file(fs, file, buffer, size)); //if its an empty file
	while(fs[(page<<4) + 15] & 0xFE) page = fs[(page<<4) + 15]; //find last page
	//at this point, page is last page with data, and fs[(page<<4) + 15] is either 0 or 1
	return(write(fs, page, (fs[(page<<4) + 15] == 1) ? 15 : (fs[(page<<4) + 14]), buffer, size));
}

int read_from_file(Tiny_FS fs, file_pointer file, byte *buffer, int max_size){
	if(get_entry(fs,file).directory_pointer.is_folder) return(0); //is a folder
	page_pointer page = get_entry(fs,file).directory_pointer.address;
	if(!(page&0xFE)) return(0); //is an empty file or doesnt exist
	int read = 0;
	byte byte_index = 0;
	while(read < max_size && (fs[(page<<4) + 15]&0xFE)){
		buffer[byte_index] = fs[(page<<4) + byte_index];
		read++;
		byte_index++;
		if(byte_index == 15){
			page = fs[(page<<4) + 15];
			byte_index = 0;
			buffer += 15;
		}
	}
	if(read == max_size) return(read);
	//reached the last page by here
	byte_index = 0;
	byte max = fs[(page<<4) + 15] ? 15 : fs[(page<<4) + 14];
	while(read < max_size && byte_index < max){
		buffer[byte_index] = fs[(page<<4) + byte_index];
		byte_index++;
		read++;
	}
	return(read);
}

//TODO compact the files folder after deleting the file - may save space.
void delete_file(Tiny_FS fs, file_pointer file){
	if(get_entry(fs,file).directory_pointer.is_folder) return; //if its a folder
	page_pointer page = get_entry(fs,file).directory_pointer.address;
	if(page == 0) return; //file doesn't exist
	if(!(page == 1)) delete_from_page(fs, page); //isn't an empty file
	get_entry(fs,file).directory_pointer.address = 0;
}

void delete_folder(Tiny_FS fs, file_pointer folder){
	if(!get_entry(fs,folder).directory_pointer.is_folder) return; //if its a file
	page_pointer page = get_entry(fs,folder).directory_pointer.address;
	file_pointer file_to_delete;
	if(page == 0) return; //file doesn't exist
	if(!(page == 1)){ //isn't an empty file
		while(page != 0){
			file_to_delete.page = page;
			for(file_to_delete.entry = 0; file_to_delete.entry<7; file_to_delete.entry++) delete(fs,file_to_delete);
			page_free(fs,page);
			page = fs[(page<<4) + 15];
		}
	}
	get_entry(fs,folder).directory_pointer.address = 0;
}

int amount_of_free_memory(Tiny_FS fs){
	int amount = 0;
	#ifdef MEMORY_SIZE_1024
		for(byte byte_index=0; byte_index < 8; byte_index++){
	#else
		for(byte byte_index=0; byte_index < 16; byte_index++){
	#endif
		byte mask = 0x80;
		while(mask){
			if(fs[byte_index] & mask) amount++;
			mask >>= 1;
		}
	}
	return(amount<<4);
}

int amount_of_entries(Tiny_FS fs, directory_pointer dir){
	int amount = 0;
	while(dir != 0){
		for(byte i=0; i<7; i++) if(get_directory(fs,dir).entries[i].directory_pointer.address != 0) amount++;
		dir = get_directory(fs,dir).end_pointer.address_of_next;
	}
	return(amount);
}

int file_size(Tiny_FS fs, file_pointer file){
	int size = 0;
	page_pointer page = fs[(file.page<<4) + (file.entry<<2)]; //get_directory(fs,file.page).entries[file.entry];
	if(page & 0x80) return(0); //if its a folder
	if(!(page & 0xFE)) return(0); //if file doesn't exist or it doesn't have any data
	while(fs[(page<<4) + 15] & 0xFE){ //while next page isn't 1 or 0
		size += 15;
		page = fs[(page<<4) + 15];
	}
	if(fs[(page<<4) + 15] & 0x01) size += 15; else size += fs[(page<<4) + 14]; //add the correct amount according to whether page is 1 or 0
	return(size);
}

directory_pointer enter_folder(Tiny_FS fs, file_pointer folder){
	//printf("entering a folder at page %.2X entry %.2X -> %.2X,(%d)\n",folder.page,folder.entry,fs[(folder.page<<4) + (folder.entry<<1)],(folder.page<<4) + (folder.entry<<1));
	if(!get_entry(fs,folder).directory_pointer.is_folder) return(0); //its not a folder
	if(get_entry(fs,folder).directory_pointer.address == 0) return(0); //it doesn't exist
	if(get_entry(fs,folder).directory_pointer.address == 1){
		get_entry(fs,folder).directory_pointer.address = allocate_page(fs);
		get_directory(fs,get_entry(fs,folder).directory_pointer.address).back_pointer.first_page = 1;
		get_directory(fs,get_entry(fs,folder).directory_pointer.address).back_pointer.address = get_folder_start(fs,folder.page);
	}
	//printf("returning %d\n",fs[(folder.page<<4) + (folder.entry<<1)]&0x7F);
	return(get_entry(fs,folder).directory_pointer.address);
}

file_pointer find_file(Tiny_FS fs, directory_pointer dir, byte file_number){ //file_pointer.page == 0 if didnt find the file
	file_pointer file;
	while(dir != 0){
		byte i;
		for(i=0; i<7; i++){
			//printf("saw file %d with address %d\n",get_directory(fs,dir).entries[i].file_number,get_directory(fs,dir).entries[i].directory_pointer.address);
			if(get_directory(fs,dir).entries[i].directory_pointer.address != 0 && get_directory(fs,dir).entries[i].file_number == file_number){
				file.entry = i;
				break;
			}
		}
		if(i != 7) break;
		dir = get_directory(fs,dir).end_pointer.address_of_next;
	}
	file.page = dir; //will be 0 if didn't find
	return(file);
}

directory_pointer get_folder_start(Tiny_FS fs, directory_pointer dir){
	while(!get_directory(fs,dir).back_pointer.first_page) dir = get_directory(fs,dir).back_pointer.address;
	return(dir);
}

void compact_folder(Tiny_FS fs, directory_pointer dir){
	/*
	algorithm works as follows:
	  -loop on all entries of directory:
	    -index pointer is set to next free entry
	    -file pointer is set to next available file, if there is'nt any, break
	    -file is copied to index
	  -delete all pages after the index - use delete from page.
	  -if index is first in page - delete entire page (its empty), and if its the first page, remove from file entry itself.

	a more efficient way would be to use to pointers, to the start and end of the directory (across multiple pages), and then:
	-find a free entry after start
	-copy to it files from before end
	-stop if start >= end
	*/
	dir = get_folder_start(fs, dir);
	file_pointer index = {.page = dir, .entry = 0};
	byte break_flag = 0;
	while(1){
		for(index.entry=0; index.entry<7; index.entry++){
			if(get_entry(fs,index).directory_pointer.address == 0){ //if reached a free entry, copy another entry to it
				file_pointer file = {.page = index.page, .entry = index.entry};
				//find next file:
				while(file.page != 0){
					while(file.entry < 7){
						if(get_entry(fs,file).directory_pointer.address) break;
						file.entry++;
					}
					if(file.entry < 7) break;
					file.page = get_directory(fs,file.page).end_pointer.address_of_next;
					file.entry = 0;
				}
				//printf("for file at [%d,%d] - ",index.page,index.entry);
				if(file.page == 0){
					//printf("no more available files.\n");
					break_flag = 1;
					break;
				}
				//printf("copying next file from [%d,%d]\n",file.page,file.entry);
				//and copy it:
				get_entry(fs,index).file_number = get_entry(fs,file).file_number;
				get_entry(fs,index).directory_pointer = get_entry(fs,file).directory_pointer;
				get_entry(fs,file).directory_pointer.address = 0;
				//
			}
		}
		if(break_flag || get_directory(fs,index.page).end_pointer.address_of_next == 0) break;
		index.page = get_directory(fs,index.page).end_pointer.address_of_next;
	}
	//delete left over pages:
	printf("deleting from page %d\n",(index.entry == 0) ? index.page : get_directory(fs,index.page).end_pointer.address_of_next);
	if(index.entry == 0){ //empty page
		struct back_pointer back = get_directory(fs,index.page).back_pointer;
		if(back.first_page){
			delete_from_page(fs, get_directory(fs,index.page).end_pointer.address_of_next);
			get_directory(fs,index.page).end_pointer.address_of_next = 0;
			//TODO could delete entire folder if was given a file_pointer to this function
		}else{
			delete_from_page(fs, index.page);
			get_directory(fs,back.address).end_pointer.address_of_next = 0;
		}
	}else{ //there are still entries in current page
		delete_from_page(fs, get_directory(fs,index.page).end_pointer.address_of_next);
		get_directory(fs,index.page).end_pointer.address_of_next = 0;
	}
	return;
}
