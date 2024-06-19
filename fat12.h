#ifndef FAT12_H
#define FAT12_H

typedef struct _FAT12{
    char disco[1474560];// 1.44MB
}fat12;

typedef struct BOOTSECTOR{
    char ignore[11];
    short sector_size;// bytes per setor
    char sector;
    short N_reserved; // numero de setores reservados
    char N_FATs;
    short Nmax_rootEntries;
    short qtd_sectors;
    char ignore1;
    short secFAT;
    short secTrack;
    short N_heads;
    int ignore2;
    int secCount;// 0 para FAT12
    short ignore3;
    char boot_signature;
    int volume_id;
    char volume_label[11];
    char type[8];
}__attribute__((packed)) boot_sector;

typedef struct ROOTDIRECTORY{
    char fileName[8];
    char extension[3];
    char attributes;
    short reserved;
    short creation_time;
    short creation_date;
    short LAD;
    short ignore;
    short LWT;// last write time
    short LWD;// last write date
    short first_cluster;
    int fileSize;
}__attribute__((packed)) root_directory;

#define ATTR_RO 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLLAB 0x08
#define ATTR_ARCH 0x20

// num maximo de entradas em um diretorio
#define MAX_FILES_SUDIR 14

typedef struct SUBDIRECTORY {
    root_directory* dir;
    char*   path;
    short parent_cluster;
    unsigned int  root_index;
    struct SUBDIRECTORY* next;
}subdirectory;

void readDISK(char* file_name, fat12* f);
void printFat12Info(boot_sector* p);
void read_BootSector(fat12* f);
void readFAT_table(fat12* f);
void read_RootDirectory(fat12* f);
void printCommands();
void cmd_ls1();
void read_Subdirectories(fat12* f);
subdirectory* new_SubDir(subdirectory* s, int r_index, char* cam);
void cmd_ls();
void grab(fat12* f, char* file_path);
void copy_data(fat12* f, root_directory r, FILE* Dest);
void cp(fat12* f, char* fsource);
void escreveDisco(FILE* DISK, char* buffer, int clusters, int first);

boot_sector* p;
short *FAT_table;
root_directory* root_vector;
subdirectory* s;

#endif