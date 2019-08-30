/*
 * Project: Tiny_FAT_FS
 * File: main.c
 * Created on: 14.12.2018
 * Author: Eldad Hellerman
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "Tiny_FAT_FS.h"
//#include "Tiny_FS_Browser.h"

Tiny_FS fs;

void print_fs_to_file(Tiny_FS fs);
void test_fragmented_file(void);
void test_rw_to_file(void);
void test_find_files(void);
void test_recursive_delete(void);
//void test_browser(void);
void test_compacting(void);

int main(int argc, char **args){
	printf("here");
	init_FS(fs);
	//test_fragmented_file();
	//test_rw_to_file();
	//test_find_files();
	//test_recursive_delete();
	//test_browser();
	test_compacting();
	/*
	file_pointer folder = create_folder(fs,root_directory,123);
	for(int i=0; i<10; i++) create_file(fs,root_directory,0xD0 + i);
	for(int i=0; i<10; i++) create_file(fs,enter_folder(fs,folder),i);
	folder = create_folder(fs,enter_folder(fs,folder),222);
	for(int i=0; i<10; i++) create_file(fs,enter_folder(fs,folder),0xA0 + i);
	//for(int i=0; i<100; i++) create_file(fs,enter_folder(fs,folder),'A' + i);
	//list_files(fs, root_directory);
	//delete(fs,folder);
	list_files(fs, root_directory,255);
	//enter_folder(fs,);
	 */
	print_fs_to_file(fs);
	return(EXIT_SUCCESS);
}


void print_fs_to_file(Tiny_FS fs){
	FILE *file = fopen("C:/Users/Eldad/Desktop/folders/hobbys/Programing/Projects/Tiny_FS/Debug/fs.bin","wb+");
	if(file == NULL) printf("could not open file\n");
	int wrote = fwrite(fs,1,1024,file);
	if(wrote != 1024) printf("error writing FS to file... wrote only %d\n",wrote);
	fclose(file);
}

void test_fragmented_file(){
	file_pointer dir_file = create_folder(fs,root_directory,111);
	directory_pointer folder = enter_folder(fs, dir_file);
	list_files(fs, root_directory,255);
	file_pointer file[10];
	byte *data = malloc(100); for(int i=0; i<100; i++) data[i] = 'A'+i;
	for(int i=0; i<10; i++){
		file[i] = create_file(fs,folder,i+'A');
		write_to_file(fs, file[i], data, 10);
	}
	list_files(fs, root_directory,255);

	delete_file(fs, file[0]);
	delete_file(fs, file[2]);
	delete_file(fs, file[4]);
	delete_file(fs, file[7]);
	delete_file(fs, file[9]);
	list_files(fs, root_directory,255);

	for(int i=0; i<100; i++) data[i] = i;
	file[1] = create_file(fs,folder,234);
	write_to_file(fs, file[1], data, 60);
	list_files(fs, root_directory,255);

	/*byte *data_in = calloc(60,1);
	printf("read %d bytes\n",read_from_file(fs, file[1], data_in, 60));
	printf("data_in:\n\t"); for(byte i=0; i<60; i++) printf("%.2X,",data_in[i]); printf(";\n");*/
}

void test_rw_to_file(){
	file_pointer dir_file = create_folder(fs,root_directory,222);
	directory_pointer folder = enter_folder(fs, dir_file);
	file_pointer file;
	byte size = 23;
	byte *data_out = malloc(size); for(int i=0; i<size; i++) data_out[i] = i;
	byte *data_in = calloc(size,1);
	file = create_file(fs,folder, 'A');
	write_to_file(fs, file, data_out, size);
	printf("read %d bytes\n",read_from_file(fs, file, data_in, 200));
	printf("data_in:\n\t"); for(byte i=0; i<size; i++) printf("%.2X,",data_in[i]); printf(";\n");
	list_files(fs, root_directory, 255);
	//delete_file(fs, file);
	//printf("read %d bytes\n",read_from_file(fs, file, data_in, 23));

	size = 33; data_out = malloc(size); for(int i=0; i<size; i++) data_out[i] = 0xA0+i;
	write_to_file(fs, file, data_out, size);
	printf("read %d bytes\n",read_from_file(fs, file, data_in, 200));
	printf("data_in:\n\t"); for(byte i=0; i<size; i++) printf("%.2X,",data_in[i]); printf(";\n");

	size = 3; data_out = malloc(size); for(int i=0; i<size; i++) data_out[i] = 0x50+i;
	write_to_file(fs, file, data_out, size);
	printf("read %d bytes\n",read_from_file(fs, file, data_in, 200));
	printf("data_in:\n\t"); for(byte i=0; i<size; i++) printf("%.2X,",data_in[i]); printf(";\n");

	size = 3; data_out = malloc(size); for(int i=0; i<size; i++) data_out[i] = 0x60+i;
	append_to_file(fs, file, data_out, size);
	printf("read %d bytes\n",read_from_file(fs, file, data_in, 200));
	printf("data_in:\n\t"); for(byte i=0; i<6; i++) printf("%.2X,",data_in[i]); printf(";\n");

	size = 30; data_out = malloc(size); for(int i=0; i<size; i++) data_out[i] = 0xC0+i;
	append_to_file(fs, file, data_out, size);
	printf("read %d bytes\n",read_from_file(fs, file, data_in, 200));
	printf("data_in:\n\t"); for(byte i=0; i<36; i++) printf("%.2X,",data_in[i]); printf(";\n");

	//printf("read %d bytes\n",read_from_file(fs, file, data_in, 200));
	//printf("data_in:\n\t"); for(byte i=0; i<size; i++) printf("%.2X,",data_in[i]); printf(";\n");
}

void test_find_files(){
	#define get_entry(fs,file)		(*(struct directory_entry *)&fs[(file.page<<4) + (file.entry<<1)])
	file_pointer dir = create_folder(fs,root_directory,123);
	directory_pointer folder = enter_folder(fs, dir);
	file_pointer file, fileFound;
	byte size = 25;
	byte *data_out = malloc(size); for(int i=0; i<size; i++) data_out[i] = i;
	file = create_file(fs,folder, 'A');
	write_to_file(fs, file, data_out, size);
	list_files(fs, root_directory,255);
	printf("original file at page %d entry %d, name - %d\n",file.page,file.entry,get_entry(fs,file).file_number);
	fileFound = find_file(fs, folder, 'A');
	printf("'A' found file at page %d entry %d, name - %d\n",fileFound.page,fileFound.entry,get_entry(fs,fileFound).file_number);
	/*
	fileFound = find_file(fs, root_directory, 'B');
	printf("'B' found file at page %d entry %d, name - %d\n",fileFound.page,fileFound.entry,get_entry(fs,fileFound).file_number);
	delete_file(fs, file);
	printf("delete file 'A'\n");

	fileFound = find_file(fs, root_directory, 'A');
	printf("'A' found file at page %d entry %d, name - %d\n",fileFound.page,fileFound.entry,get_entry(fs,fileFound).file_number);
	fileFound = find_file(fs, root_directory, 'B');
	printf("'B' found file at page %d entry %d, name - %d\n",fileFound.page,fileFound.entry,get_entry(fs,fileFound).file_number);
	*/
	for(int i=0; i<20; i++) create_file(fs,folder, 'B' + i);
	fileFound = find_file(fs, folder, 'J');
	printf("'J' found file at page %d entry %d, name - %d\n",fileFound.page,fileFound.entry,get_entry(fs,fileFound).file_number);
	list_files(fs, root_directory,255);
	delete_file(fs, fileFound);
	create_file(fs,folder, 100);
	delete_file(fs, find_file(fs, folder,100));
	list_files(fs, root_directory,255);
}

void test_recursive_delete(){
	list_files(fs, root_directory,255);
	file_pointer dir = create_folder(fs,root_directory,123);
	directory_pointer folder = enter_folder(fs, dir);
	byte size = 25; byte *data_out = malloc(size); for(int i=0; i<size; i++) data_out[i] = 0xE0+i;
	for(int i=0; i<5; i++) create_file(fs,folder, i);
	for(int i=5; i<10; i++) create_folder(fs,folder, i);
	write_to_file(fs, find_file(fs, folder, 0), data_out, size);
	folder = enter_folder(fs, find_file(fs,folder,7));
	for(int i=10; i<15; i++) create_file(fs,folder,i);
	for(int i=15; i<20; i++) create_folder(fs,folder,i);
	write_to_file(fs, find_file(fs, folder, 12), data_out, size);

	list_files(fs, root_directory,255);
	delete_folder(fs, find_file(fs, enter_folder(fs,find_file(fs, root_directory, 123)), 5));
	delete_folder(fs, find_file(fs, enter_folder(fs,find_file(fs, root_directory, 123)), 7));
	//delete_folder(fs, find_file(fs,root_directory,123));
	list_files(fs, root_directory,255);
}

/*void test_browser(){
	struct browser_struct b_s;
	browser b = &b_s;
	init_browser(fs,b);
	list(b);
	enter(b, 123);
	list(b);
	enter(b, 222);
	list(b);
	//list_files(b.fs, b.page, 3);
}*/

void test_compacting(){
	list_files(fs, root_directory,255);
	directory_pointer folder = enter_folder(fs, create_folder(fs,root_directory,123));
	for(int i=0; i<20; i++) create_file(fs,folder, i);
	//for(int i=0; i<5; i++) create_folder(fs,folder, 'a'+i);

	for(int i=0; i<20; i++) delete(fs, find_file(fs, folder, i));
//	delete(fs, find_file(fs, folder, 'b'));
//	delete(fs, find_file(fs, folder, 'c'));
//	delete(fs, find_file(fs, folder, 'd'));
//	delete(fs, find_file(fs, folder, 'e'));
	list_files(fs, root_directory,255);
	compact_folder(fs, folder);
	list_files(fs, root_directory,255);
}
